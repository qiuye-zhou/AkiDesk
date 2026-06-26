#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "historypanel.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"
#include "core/AiProvider.h"
#include "core/SpeechRecognizer.h"
#include "core/VitsEngine.h"
#include "ui/settingswindow/pages/page_llm.h"
#include "utils/DragHelper.h"
#include "utils/ScrollHelper.h"

#include <QAudioSource>
#include <QBuffer>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QJsonArray>
#include <QMediaDevices>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTextCursor>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif
#endif

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

    /* 初始化语音识别引擎 */
    m_stt = new SpeechRecognizer(this);

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

    /* 语音识别完成：先在输入框显示识别结果，短暂停留后自动发送 */
    connect(m_stt, &SpeechRecognizer::recognized, this, [this](const QString &text) {
        ui->btnVoice->setEnabled(true);
        QString trimmed = text.trimmed();
        if (!trimmed.isEmpty())
        {
            ui->textEdit->setText(trimmed);
            QTimer::singleShot(800, this, [this, trimmed]() {
                if (!ui->textEdit->isEnabled())
                    return;
                sendMessage(trimmed);
            });
        }
    });

    /* 语音识别错误 */
    connect(m_stt, &SpeechRecognizer::errorOccurred, this, [this](const QString &err) {
        ui->btnVoice->setEnabled(true);
        ui->textEdit->setEnabled(true);
        ui->textEdit->setText(err);
        ui->btnVoice->setStyleSheet("");
    });

    /* 语音按钮：长按录音，松开识别 */
    connect(ui->btnVoice, &QPushButton::pressed, this, &ChatDialog::on_btnVoice_pressed);
    connect(ui->btnVoice, &QPushButton::released, this, &ChatDialog::on_btnVoice_released);
}

ChatDialog::~ChatDialog()
{
    stopPendingState();
    if (m_historyPanel)
    {
        m_historyPanel->close();
        delete m_historyPanel;
    }
    delete ui;
}

void ChatDialog::closeEvent(QCloseEvent *event)
{
    if (m_historyPanel)
        m_historyPanel->close();
    QWidget::closeEvent(event);
}

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
    for (int i = 1; i < 5; ++i)
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

#ifdef Q_OS_WIN
    QTimer::singleShot(0, this, &ChatDialog::removeBorder);
#endif

    ui->btnNext->hide();
    ui->scrollBar->hide();
    new ScrollHelper(ui->textEdit, ui->scrollBar, 5, this);
    new DragHelper(this);

    /* 同步 scrollBar 与 textEdit 的滚动 */
    connect(ui->scrollBar, &QScrollBar::valueChanged,
            ui->textEdit->verticalScrollBar(), &QScrollBar::setValue);
    connect(ui->textEdit->verticalScrollBar(), &QScrollBar::rangeChanged,
            this, [this](int min, int max) {
        ui->scrollBar->setRange(min, max);
        ui->scrollBar->setPageStep(ui->textEdit->verticalScrollBar()->pageStep());
        ui->scrollBar->setVisible(max > min);
    });
}

void ChatDialog::removeBorder()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    DWORD cornerPref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                          &cornerPref, sizeof(cornerPref));
    COLORREF borderColor = 0xFFFFFFFE;
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR,
                          &borderColor, sizeof(borderColor));
#endif
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
        while (m_context.size() > 40)
            m_context.removeFirst();
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

    if (m_ai)
        m_ai->cancelAll();

    if (m_vits)
        m_vits->stopAll();

    if (m_stt)
        m_stt->cancel();

    if (m_recording)
    {
        m_recording = false;
        if (m_audioSource)
        {
            m_audioSource->stop();
            m_audioSource->deleteLater();
            m_audioSource = nullptr;
        }
        if (m_audioBuffer)
        {
            m_audioBuffer->close();
            m_audioBuffer->deleteLater();
            m_audioBuffer = nullptr;
        }
    }
}

/* 按下回车发送消息 */
void ChatDialog::keyPressEvent(QKeyEvent *event)
{
    if (!m_pressedKeys.contains(event->key()))
        m_pressedKeys.append(event->key());
    QWidget::keyPressEvent(event);
}

void ChatDialog::keyReleaseEvent(QKeyEvent *event)
{
    m_pressedKeys.removeAll(event->key());
    if (event->key() == Qt::Key_Return && !m_pressedKeys.contains(Qt::Key_Shift)
        && ui->textEdit->isEnabled())
    {
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
        sendMessage(userInput);
    }
    QWidget::keyReleaseEvent(event);
}

/* 发送用户消息给 AI（供键盘输入和语音识别共用） */
void ChatDialog::sendMessage(const QString &text)
{
    if (text.isEmpty())
        return;

    const QString charName = CurrentCharacterName();
    ui->labelName->setText(charName.isEmpty() ? QStringLiteral("角色") : charName);
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
    m_lastInput = text;
    JsonConfig charCfg(CurrentCharacterUserConfig());
    m_vitsEnabled = charCfg.value("vitsEnable").toBool();
    JsonConfig globalCfg(GlobalConfigPath);
    m_vitsSentenceSplit = globalCfg.value("vits/SentenceSplit", true).toBool();
    m_streamRaw.clear();
    m_streamChinese.clear();
    m_vitsCursor = 0;
    if (m_vits)
        m_vits->stopAll();

    m_ai->chat(buildMessageWithContext(text));
    ui->textEdit->setText("……");
}

/* 点击"继续"按钮：恢复输入状态 */
void ChatDialog::on_btnNext_clicked()
{
    ui->labelName->setText("你");
    ui->textEdit->clear();
    ui->textEdit->setEnabled(true);
    ui->btnNext->hide();
}

void ChatDialog::on_btnVoice_pressed()
{
    if (m_recording || !ui->textEdit->isEnabled())
        return;

    /* 查找用户配置的麦克风设备 */
    JsonConfig cfg(GlobalConfigPath);
    QString deviceId = cfg.value("stt/deviceId").toString();
    QAudioDevice device;
    if (!deviceId.isEmpty())
    {
        for (const QAudioDevice &dev : QMediaDevices::audioInputs())
        {
            if (dev.id() == deviceId)
            {
                device = dev;
                break;
            }
        }
    }
    if (device.isNull())
        device = QMediaDevices::defaultAudioInput();
    if (device.isNull())
    {
        ui->textEdit->setText("未检测到麦克风设备");
        return;
    }

    /* 配置录音格式：16000Hz, 单声道, 16bit PCM */
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_audioBuffer = new QBuffer(this);
    m_audioBuffer->open(QBuffer::WriteOnly);

    m_audioSource = new QAudioSource(device, format, this);
    m_audioSource->start(m_audioBuffer);
    m_recording = true;

    /* 按钮变红表示录音中 */
    ui->btnVoice->setStyleSheet(
        "QPushButton { background-color: #FF3B30; color: #FFFFFF; border: none;"
        " border-radius: 5px; font-size: 16px; }");
    ui->textEdit->setText("正在录音…");
}

void ChatDialog::on_btnVoice_released()
{
    if (!m_recording)
        return;
    m_recording = false;

    /* 停止录音 */
    if (m_audioSource)
    {
        m_audioSource->stop();
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
    }

    QByteArray pcmData;
    if (m_audioBuffer)
    {
        m_audioBuffer->close();
        pcmData = m_audioBuffer->data();
        m_audioBuffer->deleteLater();
        m_audioBuffer = nullptr;
    }

    /* 恢复按钮样式 */
    ui->btnVoice->setStyleSheet("");

    if (pcmData.isEmpty())
    {
        ui->textEdit->clear();
        return;
    }

    ui->textEdit->setText("识别中…");
    ui->btnVoice->setEnabled(false);
    m_stt->recognize(pcmData);
}

void ChatDialog::toggleVisible() { setVisible(!isVisible()); }

void ChatDialog::setVisible(bool visible)
{
    if (!visible && m_historyPanel)
        m_historyPanel->hide();
    QWidget::setVisible(visible);
}

void ChatDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &ChatDialog::removeBorder);
}

void ChatDialog::on_btnHistory_clicked()
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
                const QString cn = CurrentCharacterName();
                ui->labelName->setText(cn.isEmpty() ? QStringLiteral("角色") : cn);
                ui->textEdit->setEnabled(false);
                ui->textEdit->setText(line.mid(3));
                ui->btnNext->show();
            }
            if (m_historyVisible)
                on_btnHistory_clicked();
        });
        connect(m_historyPanel, &HistoryPanel::deleteIndex, this, [this](int idx) {
            if (idx < 0 || idx >= m_context.size())
                return;
            m_context.removeAt(idx);
            saveContext();
            if (m_historyVisible)
            {
                m_historyPanel->clear();
                for (int i = 0; i < m_context.size(); ++i)
                {
                    const QString &line = m_context[i];
                    if (line.startsWith("用户："))
                        m_historyPanel->addItem(i, "你", line.mid(3));
                    else if (line.startsWith("角色："))
                    {
                        const QString cn = CurrentCharacterName();
                        m_historyPanel->addItem(i, cn.isEmpty() ? QStringLiteral("角色") : cn, line.mid(3));
                    }
                    else
                        m_historyPanel->addItem(i, "记录", line);
                }
            }
        });
    }

    m_historyPanel->clear();
    for (int i = 0; i < m_context.size(); ++i)
    {
        const QString &line = m_context[i];
        if (line.startsWith("用户："))
            m_historyPanel->addItem(i, "你", line.mid(3));
        else if (line.startsWith("角色："))
        {
            const QString cn = CurrentCharacterName();
            m_historyPanel->addItem(i, cn.isEmpty() ? QStringLiteral("角色") : cn, line.mid(3));
        }
        else
            m_historyPanel->addItem(i, "记录", line);
    }

    m_historyPanel->resize(width(), m_historyPanel->height());
    m_historyPanel->move(x(), y() - m_historyPanel->height());

    if (!m_historyVisible)
    {
        m_historyPanel->show();
        m_historyPanel->raise();
        m_historyVisible = true;

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
    else
    {
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
}

void ChatDialog::reloadAiConfig()
{
    JsonConfig charCfg(CurrentCharacterUserConfig());
    QString server = charCfg.value("serverSelect").toString();
    QString baseUrl, apiKey;
    if (PageLLM::findProvider(server, baseUrl, apiKey))
    {
        m_ai->setApiUrl(baseUrl);
        m_ai->setApiKey(apiKey);
    }

    m_ai->setModel(charCfg.value("modelSelect").toString());

    JsonConfig globalCfg(GlobalConfigPath);

    /* 加载语音识别配置 */
    m_stt->setApiKey(globalCfg.value("stt/apiKey").toString());
    m_stt->setSecretKey(globalCfg.value("stt/secretKey").toString());

    /* 先读取 VITS 启用状态再使用 */
    m_vitsEnabled = charCfg.value("vitsEnable").toBool();
    if (m_vitsEnabled)
    {
        m_vits->setApiUrl(globalCfg.value("vits/ApiUrl").toString());
        QString mas = charCfg.value("vitsMasSelect").toString();
        m_vits->setModel(mas.section(" - ", 0, 0).trimmed());
        m_vits->setSpeaker(mas.section(" - ", 1, 1).trimmed());
    }

    loadContext();
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

int ChatDialog::findSentenceEnd(const QString &text, int from)
{
    for (int i = qMax(0, from); i < text.size(); ++i)
    {
        const QChar ch = text.at(i);
        if (ch == '.' || ch == '!' || ch == '?' || ch == '\n' ||
            ch == QChar(0x3002) ||
            ch == QChar(0xFF01) ||
            ch == QChar(0xFF1F) ||
            ch == QChar(0x3001) ||
            ch == QChar(0xFF1B) ||
            ch == ';')
            return i;
    }
    return -1;
}
