#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "historypanel.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"
#include "core/AiProvider.h"
#include "core/VitsEngine.h"
#include "utils/DragHelper.h"
#include "utils/ScrollHelper.h"

#include <QDir>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTextCursor>

ChatDialog::ChatDialog(QWidget *parent)
    : QWidget(parent), ui(new Ui::ChatDialog)
{
    ui->setupUi(this);
    initWindow();

    /* 初始化 AI 提供者 */
    m_ai = new AiProvider(this);
    m_ai->setStreamEnabled(true);

    /* 初始化 VITS 语音引擎 */
    m_vits = new VitsEngine(this);

    reloadAiConfig();
    loadContext();

    /* 流式 chunk 到达：实时解析并显示中文，按句触发语音合成 */
    connect(m_ai, &AiProvider::replyChunkReceived, this, [this](const QString &chunk) {
        m_streamRaw += chunk;

        /* 解析格式：心情|中文|日语，按 "|" 分隔 */
        const int sep1 = m_streamRaw.indexOf('|');
        if (sep1 < 0)
            return;
        const int sep2 = m_streamRaw.indexOf('|', sep1 + 1);
        const int chineseEnd = (sep2 < 0) ? m_streamRaw.size() : sep2;
        const QString chinese = m_streamRaw.mid(sep1 + 1, chineseEnd - sep1 - 1);

        /* 更新显示 */
        if (!chinese.isEmpty() && chinese != m_streamChinese)
        {
            m_streamChinese = chinese;
            ui->textEdit->setText(m_streamChinese);
        }

        /* 检测到日语部分后，按句子切分触发语音合成 */
        if (m_vitsEnabled && m_vitsSentenceSplit && sep2 >= 0)
        {
            const QString japanese = m_streamRaw.mid(sep2 + 1);
            int end = findSentenceEnd(japanese, m_vitsCursor);
            while (end >= 0)
            {
                QString sentence = japanese.mid(m_vitsCursor, end - m_vitsCursor + 1).trimmed();
                m_vitsCursor = end + 1;
                if (!sentence.isEmpty())
                    m_vits->enqueueAndPlay(sentence);
                end = findSentenceEnd(japanese, m_vitsCursor);
            }
        }
    });

    /* 完整回复到达：最终解析、补漏语音、更新立绘、保存历史 */
    connect(m_ai, &AiProvider::replyReceived, this, [this](const QString &reply) {
        const QString full = m_streamRaw.isEmpty() ? reply : m_streamRaw;
        const QString mood = full.section('|', 0, 0).trimmed();
        const QString chinese = full.section('|', 1, 1).trimmed();
        const QString japanese = full.section('|', 2, 2).trimmed();

        ui->textEdit->setText(chinese);
        ui->btnNext->show();

        /* 补漏：如果最后一段没有句末标点，在结束时补一次合成 */
        if (m_vitsEnabled && m_vitsSentenceSplit)
        {
            const QString remain = japanese.mid(qMax(0, m_vitsCursor)).trimmed();
            if (!remain.isEmpty())
                m_vits->enqueueAndPlay(remain);
        }
        else if (m_vitsEnabled && !japanese.isEmpty())
        {
            m_vits->enqueueAndPlay(japanese);
        }

        emit requestSetTachie(mood);

        /* 保存对话历史 */
        if (!m_lastInput.isEmpty())
        {
            appendHistory(QStringLiteral("用户：") + m_lastInput);
            m_lastInput.clear();
        }
        appendHistory(QStringLiteral("角色：") + chinese);

        m_streamRaw.clear();
        m_streamChinese.clear();
        m_vitsCursor = 0;
    });

    /* 错误处理 */
    connect(m_ai, &AiProvider::errorOccurred, this, [this](const QString &err) {
        ui->btnNext->show();
        ui->textEdit->setText(err);
        m_lastInput.clear();
        m_streamRaw.clear();
        m_streamChinese.clear();
        m_vitsCursor = 0;
    });
}

ChatDialog::~ChatDialog() { delete ui; }

/* 绘制圆角白色背景 + 阴影 */
void ChatDialog::paintEvent(QPaintEvent *)
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QRectF rect(5, 5, width() - 10, height() - 10);
    path.addRoundedRect(rect, 15, 15);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(Qt::white));

    QColor shadow(0, 0, 0, 50);
    for (int i = 0; i < 5; ++i)
    {
        QPainterPath sp;
        sp.setFillRule(Qt::WindingFill);
        QRectF sr(5 - i, 5 - i, width() - (5 - i) * 2, height() - (5 - i) * 2);
        sp.addRoundedRect(sr, 15, 15);
        shadow.setAlpha(50 - qSqrt(i) * 22);
        painter.setPen(shadow);
        painter.drawPath(sp);
    }
}

/* 初始化无边框透明窗口 */
void ChatDialog::initWindow()
{
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint;
#ifdef Q_OS_LINUX
    flags |= Qt::X11BypassWindowManagerHint;
#endif
    setWindowFlags(flags);
    setWindowOpacity(0.95);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->btnNext->hide();
    ui->scrollBar->hide();
    new ScrollHelper(ui->textEdit, ui->scrollBar, 5, this);
    new DragHelper(this);
    ui->textEdit->installEventFilter(this);
    ui->textEdit->viewport()->installEventFilter(this);
}

/* 从本地文件加载对话上下文历史 */
void ChatDialog::loadContext()
{
    m_context.clear();
    const QString path = CurrentCharacterContextPath();
    if (path.isEmpty())
        return;
    JsonConfig cfg(path);
    const QJsonArray arr = cfg.value("history", QJsonArray()).toArray();
    for (const QJsonValue &v : arr)
        m_context.append(v.toString());
}

/* 保存对话上下文到本地文件 */
void ChatDialog::saveContext() const
{
    const QString path = CurrentCharacterContextPath();
    if (path.isEmpty())
        return;
    QJsonArray arr;
    for (const QString &line : m_context)
        arr.append(line);
    JsonConfig cfg(path);
    cfg.setValue("history", arr);
}

void ChatDialog::appendHistory(const QString &line)
{
    if (!line.isEmpty())
    {
        m_context.append(line);
        saveContext();
    }
}

/* 构建带上下文的用户消息 */
QString ChatDialog::buildMessageWithContext(const QString &input) const
{
    if (m_context.isEmpty())
        return input;
    return "以下是你和用户最近的对话，请延续上下文并保持人设一致：\n" +
           m_context.join("\n") + "\n\n用户当前输入：" + input;
}

/* 清理残留的流式和播放状态 */
void ChatDialog::stopPendingState()
{
    m_lastInput.clear();
    m_streamRaw.clear();
    m_streamChinese.clear();
    m_vitsEnabled = false;
    m_vitsCursor = 0;
    if (m_vits)
        m_vits->stopAll();
}

/* 按下回车发送消息 */
void ChatDialog::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return && !(event->modifiers() & Qt::ShiftModifier))
    {
        /* 获取用户输入 */
        QTextCursor cursor = ui->textEdit->textCursor();
        if (cursor.hasSelection())
            cursor.clearSelection();
        cursor.deletePreviousChar();
        const QString userInput = ui->textEdit->toPlainText().trimmed();
        if (userInput.isEmpty())
        {
            ui->textEdit->clear();
            return;
        }

        ui->labelName->setText("她");
        ui->textEdit->setEnabled(false);
        ui->btnNext->hide();

        /* 读取当前角色的立绘表情列表，用于构建 system prompt */
        QDir dir(CurrentCharacterTachiePath());
        QStringList filters;
        filters << "*.png" << "*.jpg" << "*.jpeg";
        QStringList tachieNames;
        for (const QString &f : dir.entryList(filters, QDir::Files))
            tachieNames << f.section('.', 0, 0);

        /* 读取角色设定 prompt */
        JsonConfig assetCfg(CurrentCharacterAssetConfig());
        QString charPrompt = assetCfg.value("prompt").toString().trimmed();

        /* 构建 system prompt */
        QString sysPrompt;
        if (!charPrompt.isEmpty())
            sysPrompt += "角色设定：" + charPrompt + "\n请始终保持该设定进行回复。\n\n";
        sysPrompt += "你是一个桌宠 AI，输出内容必须严格按照以下格式：\n"
                     "心情|中文|日语\n\n"
                     "要求：\n"
                     "1. 心情必须从以下列表中选择：" + tachieNames.join(", ") + "\n"
                     "2. 中文是桌宠此刻想表达的内容\n"
                     "3. 日语是中文内容的对应翻译\n"
                     "4. 输出中不能有多余内容或解释，严格用\"|\"分隔\n\n"
                     "示例输出：\n"
                     "快乐|今天的天气真好呀！|今日はいい天気ですね！\n"
                     "生气|为什么一直打扰我！|なんでずっと邪魔するの！";
        m_ai->setSystemPrompt(sysPrompt);

        /* 初始化本轮对话状态 */
        m_lastInput = userInput;
        JsonConfig charCfg(CurrentCharacterUserConfig());
        m_vitsEnabled = charCfg.value("vitsEnable").toBool();
        JsonConfig globalCfg(GlobalConfigPath);
        m_vitsSentenceSplit = globalCfg.value("vits/SentenceSplit", true).toBool();
        m_streamRaw.clear();
        m_streamChinese.clear();
        m_vitsCursor = 0;
        if (m_vits)
            m_vits->stopAll();

        m_ai->chat(buildMessageWithContext(userInput));
        ui->textEdit->setText("……");
    }
    QWidget::keyReleaseEvent(event);
}

/* 点击"继续"按钮：恢复输入状态 */
void ChatDialog::on_btnNext_clicked()
{
    ui->labelName->setText("你");
    ui->textEdit->clear();
    ui->textEdit->setEnabled(true);
    ui->btnNext->hide();
}

void ChatDialog::toggleVisible() { setVisible(!isVisible()); }

/* 重新加载 AI 配置（切换角色或修改设置后调用） */
void ChatDialog::reloadAiConfig()
{
    JsonConfig charCfg(CurrentCharacterUserConfig());
    QString server = charCfg.value("serverSelect").toString();
    if (server == "OpenAI")
        m_ai->setServiceType(AiProvider::OpenAI);
    else
        m_ai->setServiceType(AiProvider::DeepSeek);

    m_ai->setModel(charCfg.value("modelSelect").toString());

    JsonConfig globalCfg(GlobalConfigPath);
    m_ai->setApiKey(globalCfg.value("llm/" + server + "/ApiKey").toString());

    if (m_vitsEnabled)
    {
        m_vits->setApiUrl(globalCfg.value("vits/ApiUrl").toString());
        QString mas = charCfg.value("vitsMasSelect").toString();
        m_vits->setModel(mas.section(" - ", 0, 0).trimmed());
        m_vits->setSpeaker(mas.section(" - ", 2, 2).trimmed());
    }

    loadContext();
}

/* 打开历史面板 */
void ChatDialog::showHistoryPanel()
{
    if (!m_historyPanel)
    {
        m_historyPanel = new HistoryPanel(this);
        connect(m_historyPanel, &HistoryPanel::jumpToIndex, this, [this](int idx) {
            if (idx < 0 || idx >= m_context.size())
                return;
            stopPendingState();
            m_context = m_context.mid(0, idx + 1);
            saveContext();

            const QString line = m_context.at(idx);
            if (line.startsWith("用户："))
            {
                ui->labelName->setText("你");
                ui->textEdit->setEnabled(true);
                ui->textEdit->setText(line.mid(3));
                ui->btnNext->hide();
            }
            else if (line.startsWith("角色："))
            {
                ui->labelName->setText("她");
                ui->textEdit->setEnabled(false);
                ui->textEdit->setText(line.mid(3));
                ui->btnNext->show();
            }
            if (m_historyVisible)
                hideHistoryPanel();
        });
    }

    m_historyPanel->clear();
    for (int i = 0; i < m_context.size(); ++i)
    {
        const QString &line = m_context[i];
        if (line.startsWith("用户："))
            m_historyPanel->addItem(i, "你", line.mid(3));
        else if (line.startsWith("角色："))
            m_historyPanel->addItem(i, "她", line.mid(3));
        else
            m_historyPanel->addItem(i, "记录", line);
    }

    m_historyPanel->move(x(), y() - m_historyPanel->height());
    m_historyPanel->show();
    m_historyPanel->raise();
    m_historyVisible = true;

    /* 弹出动画：从下方滑入 + 淡入 */
    auto *opacity = new QGraphicsOpacityEffect(m_historyPanel);
    m_historyPanel->setGraphicsEffect(opacity);
    opacity->setOpacity(0.0);

    QRect start = m_historyPanel->geometry();
    QRect end = start;
    start.moveTop(start.top() + 20);
    m_historyPanel->setGeometry(start);

    auto *fadeAnim = new QPropertyAnimation(opacity, "opacity");
    fadeAnim->setDuration(150);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    auto *moveAnim = new QPropertyAnimation(m_historyPanel, "geometry");
    moveAnim->setDuration(150);
    moveAnim->setStartValue(start);
    moveAnim->setEndValue(end);

    auto *group = new QParallelAnimationGroup(m_historyPanel);
    group->addAnimation(fadeAnim);
    group->addAnimation(moveAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void ChatDialog::hideHistoryPanel()
{
    if (!m_historyPanel || !m_historyVisible)
        return;
    m_historyVisible = false;

    auto *opacity = qobject_cast<QGraphicsOpacityEffect *>(m_historyPanel->graphicsEffect());
    if (!opacity)
    {
        opacity = new QGraphicsOpacityEffect(m_historyPanel);
        m_historyPanel->setGraphicsEffect(opacity);
    }

    QRect startR = m_historyPanel->geometry();
    QRect endR = startR;
    endR.moveTop(endR.top() + 20);

    auto *fadeAnim = new QPropertyAnimation(opacity, "opacity");
    fadeAnim->setDuration(150);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    auto *moveAnim = new QPropertyAnimation(m_historyPanel, "geometry");
    moveAnim->setDuration(150);
    moveAnim->setStartValue(startR);
    moveAnim->setEndValue(endR);

    auto *group = new QParallelAnimationGroup(m_historyPanel);
    group->addAnimation(fadeAnim);
    group->addAnimation(moveAnim);
    connect(group, &QParallelAnimationGroup::finished, m_historyPanel, &QWidget::hide);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

/* 滚轮上滑打开历史面板，下滑关闭 */
void ChatDialog::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0 && !m_historyVisible)
        showHistoryPanel();
    else if (event->angleDelta().y() < 0 && m_historyVisible)
        hideHistoryPanel();
    QWidget::wheelEvent(event);
}

/* 对话框移动时，历史面板跟随移动 */
void ChatDialog::moveEvent(QMoveEvent *event)
{
    if (m_historyPanel && m_historyPanel->isVisible())
    {
        QPoint offset = event->pos() - m_lastPos;
        m_historyPanel->move(m_historyPanel->pos() + offset);
    }
    m_lastPos = event->pos();
    QWidget::moveEvent(event);
}

/* 拦截文本编辑框的滚轮事件，转为历史面板控制 */
bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == ui->textEdit || watched == ui->textEdit->viewport()) &&
        event->type() == QEvent::Wheel)
    {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->angleDelta().y() > 0 && !m_historyVisible)
            showHistoryPanel();
        else if (we->angleDelta().y() < 0 && m_historyVisible)
            hideHistoryPanel();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

/* 寻找中/英/日文句末标点位置 */
int ChatDialog::findSentenceEnd(const QString &text, int from)
{
    for (int i = qMax(0, from); i < text.size(); ++i)
    {
        const QChar ch = text.at(i);
        if (ch == '.' || ch == '!' || ch == '?' || ch == '\n' ||
            ch == QChar(0x3002) || /* 。 */
            ch == QChar(0xFF01) || /* ！ */
            ch == QChar(0xFF1F) || /* ？ */
            ch == QChar(0x3001) || /* 、 */
            ch == QChar(0xFF1B) || /* ； */
            ch == ';')
            return i;
    }
    return -1;
}
