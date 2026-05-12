#include "characterwindow.h"
#include "ui_characterwindow.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"
#include "utils/DragHelper.h"

#include <QBitmap>
#include <QDir>
#include <QEasingCurve>
#include <QFileInfo>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QVariantAnimation>
#include <memory>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#endif

CharacterWindow::CharacterWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::CharacterWindow)
{
    ui->setupUi(this);
    /* 将立绘 label 从布局中移出，使用绝对定位以兼容动画 */
    if (ui->mainLayout)
        ui->mainLayout->removeWidget(ui->labelTachie);
    ui->labelTachie->setParent(this);

    setAttribute(Qt::WA_TranslucentBackground);
    Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;
#ifdef Q_OS_LINUX
    flags |= Qt::X11BypassWindowManagerHint;
#endif
    setWindowFlags(flags);

    auto *drag = new DragHelper(this);
    connect(drag, &DragHelper::dragFinished, this, &CharacterWindow::savePosition);

    /* 延迟加载默认立绘 */
    QTimer::singleShot(0, this, [this]() { setTachieImage("default"); });
    m_pluginManager.reload();
}

CharacterWindow::~CharacterWindow()
{
    if (m_activeAnim)
    {
        m_activeAnim->stop();
        delete m_activeAnim;
    }
    delete ui;
}

#ifdef Q_OS_LINUX
void CharacterWindow::applyInputShape(const QRegion &region)
{
    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return;
    Window wid = static_cast<Window>(winId());
    const int count = region.rectCount();
    if (count <= 0)
    {
        XShapeCombineRectangles(display, wid, ShapeInput, 0, 0, nullptr, 0, ShapeSet, YXBanded);
        XCloseDisplay(display);
        return;
    }
    QVector<XRectangle> rects(count);
    int i = 0;
    for (const QRect &r : region)
    {
        rects[i++] = {static_cast<short>(r.x()), static_cast<short>(r.y()),
                      static_cast<unsigned short>(r.width()),
                      static_cast<unsigned short>(r.height())};
    }
    XShapeCombineRectangles(display, wid, ShapeInput, 0, 0, rects.data(), count, ShapeSet, YXBanded);
    XCloseDisplay(display);
}

void CharacterWindow::applyInputShapeFromImage()
{
    if (m_scaledImage.isNull())
        return;
    QRegion region(QBitmap::fromImage(m_scaledImage.createAlphaMask()));
    region.translate(m_scaledTopLeft);
    applyInputShape(region);
}
#endif

/* 根据名称加载立绘图片并显示 */
void CharacterWindow::setTachieImage(const QString &name)
{
    const QString dirPath = CurrentCharacterTachiePath();
    if (dirPath.isEmpty())
        return;

    const QString normalizedName = name.trimmed();
    QPixmap loaded;
    bool ok = false;

    /* 尝试加载不同后缀 */
    QStringList candidates;
    if (QFileInfo(normalizedName).suffix().isEmpty())
        candidates << (normalizedName + ".png") << (normalizedName + ".jpg") << (normalizedName + ".jpeg");
    else
        candidates << normalizedName;

    QDir dir(dirPath);
    for (const QString &c : candidates)
    {
        const QString fp = dir.filePath(c);
        if (QFileInfo::exists(fp) && loaded.load(fp))
        {
            ok = true;
            break;
        }
    }
    /* 大小写不敏感兜底 */
    if (!ok)
    {
        for (const QString &f : dir.entryList({"*.png", "*.jpg", "*.jpeg"}, QDir::Files))
        {
            if (QFileInfo(f).completeBaseName().compare(normalizedName, Qt::CaseInsensitive) != 0)
                continue;
            if (loaded.load(dir.filePath(f)))
            {
                ok = true;
                break;
            }
        }
    }

    if (ok)
        m_currentPixmap = loaded;
    else if (m_currentPixmap.isNull())
        return;

    /* 读取用户配置的缩放百分比 */
    JsonConfig cfg(CurrentCharacterUserConfig());
    setTachieSize(cfg.value("tachieSize", "100").toString().toInt());
    restorePosition();

    /* 触发该表情对应的动画 */
    tryPlayAnimation(QFileInfo(normalizedName).completeBaseName());
}

/* 设置立绘缩放并居中渲染（预留 2 倍画布供缩放动画使用） */
void CharacterWindow::setTachieSize(int sizePercent)
{
    constexpr double kCanvasScale = 2.0;
    if (m_currentPixmap.isNull())
        return;
    const int safe = (sizePercent <= 0) ? 100 : sizePercent;
    QPixmap scaled = m_currentPixmap.scaled(
        m_currentPixmap.size() * (safe / 100.0), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const int cw = qMax(1, qRound(scaled.width() * kCanvasScale));
    const int ch = qMax(1, qRound(scaled.height() * kCanvasScale));
    const int ix = (cw - scaled.width()) / 2;
    const int iy = (ch - scaled.height()) / 2;

    resize(cw, ch);
    ui->labelTachie->setPixmap(scaled);
    ui->labelTachie->setGeometry(ix, iy, scaled.width(), scaled.height());
    m_scaledImage = scaled.toImage();
    m_scaledTopLeft = QPoint(ix, iy);

#ifdef Q_OS_LINUX
    applyInputShapeFromImage();
#else
    /* Windows: 使用图片 alpha 通道作为点击穿透区域 */
    QRegion region(QBitmap::fromImage(m_scaledImage.createAlphaMask()));
    region.translate(m_scaledTopLeft);
    ui->labelTachie->setMask(region);
#endif
}

/* 右键菜单触发：请求切换对话框显隐 */
void CharacterWindow::contextMenuEvent(QContextMenuEvent *)
{
    emit requestToggleChat();
}

/* 鼠标穿透：将未命中图片的点击传递到底层窗口 */
void CharacterWindow::mousePressEvent(QMouseEvent *event) { event->ignore(); }
void CharacterWindow::mouseReleaseEvent(QMouseEvent *event) { event->ignore(); }

/* 从 ini 恢复立绘窗口位置 */
void CharacterWindow::restorePosition()
{
    if (m_positionRestored)
        return;
    QSettings settings(LocalConfigPath, QSettings::IniFormat);
    int x = settings.value("tachie/x", -1).toInt();
    int y = settings.value("tachie/y", -1).toInt();
    if (x >= 0 && y >= 0)
        move(x, y);
    m_positionRestored = true;
}

/* 保存当前立绘窗口位置到 ini */
void CharacterWindow::savePosition()
{
    QSettings settings(LocalConfigPath, QSettings::IniFormat);
    settings.setValue("tachie/x", pos().x());
    settings.setValue("tachie/y", pos().y());
}

void CharacterWindow::resetPosition()
{
    move(200, 200);
    savePosition();
}

/* 根据表情名查找并播放对应的动画插件 */
void CharacterWindow::tryPlayAnimation(const QString &actionName)
{
    const QString charName = CurrentCharacterName();
    if (charName.isEmpty())
        return;

    /* 从角色资产配置中读取 动作名 → 动画唯一键 的映射 */
    JsonConfig assetCfg(CurrentCharacterAssetConfig());
    QJsonObject animMap = assetCfg.value("tachieAnimations", QJsonObject()).toObject();
    QString uniqueKey = animMap.value(actionName).toString().trimmed();
    if (uniqueKey.isEmpty())
        return;

    m_pluginManager.reload();
    PluginDefinition plugin;
    PluginAnimation animation;
    if (!m_pluginManager.findAnimation(uniqueKey, plugin, animation))
        return;

    /* 停止之前的动画 */
    if (m_activeAnim)
    {
        m_activeAnim->stop();
        m_activeAnim->deleteLater();
        m_activeAnim = nullptr;
    }

    auto *seq = new QSequentialAnimationGroup(this);

    struct ScaleState { QRect baseRect; bool init = false; };
    auto scaleState = std::make_shared<ScaleState>();

    for (const PluginStep &step : animation.steps)
    {
        const int ms = qMax(1, static_cast<int>(step.durationSec * 1000.0));

        if (step.type == PluginStep::Type::Move)
        {
            struct MoveState { QPoint base; bool init = false; };
            auto ms_ = std::make_shared<MoveState>();
            auto *anim = new QVariantAnimation(seq);
            anim->setDuration(ms);
            anim->setEasingCurve(QEasingCurve::Linear);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            connect(anim, &QVariantAnimation::valueChanged, this,
                    [this, ms_, step](const QVariant &v) {
                        if (!ms_->init)
                        {
                            ms_->base = pos();
                            ms_->init = true;
                        }
                        double p = v.toDouble();
                        move(ms_->base + QPoint(qRound(step.x * p), qRound(step.y * p)));
                    });
            seq->addAnimation(anim);
        }
        else if (step.type == PluginStep::Type::Opacity)
        {
            auto *anim = new QPropertyAnimation(this, "windowOpacity", seq);
            anim->setDuration(ms);
            anim->setStartValue(step.from);
            anim->setEndValue(step.to);
            seq->addAnimation(anim);
        }
        else if (step.type == PluginStep::Type::Scale)
        {
            auto *anim = new QVariantAnimation(seq);
            anim->setDuration(ms);
            anim->setEasingCurve(QEasingCurve::Linear);
            anim->setStartValue(step.scaleFrom);
            anim->setEndValue(step.scaleTo);

            auto applyFrame = [this, scaleState](double factor) {
                const double safe = qBound(0.05, factor, 2.0);
                const int w = qMax(1, qRound(scaleState->baseRect.width() * safe));
                const int h = qMax(1, qRound(scaleState->baseRect.height() * safe));
                const QPointF center(width() / 2.0, height() / 2.0);
                const int x = qRound(center.x() - w / 2.0);
                const int y = qRound(center.y() - h / 2.0);
                if (!m_currentPixmap.isNull())
                {
                    QPixmap sp = m_currentPixmap.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    ui->labelTachie->setPixmap(sp);
                    ui->labelTachie->setGeometry(x, y, w, h);
                    m_scaledImage = sp.toImage();
                    m_scaledTopLeft = QPoint(x, y);
                }
            };

            connect(anim, &QVariantAnimation::valueChanged, this,
                    [scaleState, applyFrame, this](const QVariant &v) {
                        if (!scaleState->init)
                        {
                            scaleState->baseRect = ui->labelTachie->geometry();
                            scaleState->init = true;
                        }
                        applyFrame(v.toDouble());
                    });
            connect(anim, &QVariantAnimation::finished, this,
                    [scaleState, applyFrame, step]() {
                        if (!scaleState->init)
                            return;
                        applyFrame(step.scaleTo);
                    });
            seq->addAnimation(anim);
        }
    }

    if (seq->animationCount() <= 0)
    {
        seq->deleteLater();
        return;
    }
    m_activeAnim = seq;
    connect(seq, &QSequentialAnimationGroup::finished, this,
            [this]() { m_activeAnim = nullptr; });
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}
