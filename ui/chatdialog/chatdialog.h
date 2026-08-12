#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QPoint>
#include <QRect>
#include <QScopedPointer>
#include <QShowEvent>
#include <QStringList>
#include <QWidget>

class AiProvider;
class VitsEngine;
class SpeechRecognizer;
class CommandExecutor;
class HistoryPanel;
class QAudioSource;
class QBuffer;

namespace Ui { class ChatDialog; }

class ChatDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

    void setVisible(bool visible) override;

    /* 启动时根据当前时间自动发送一次招呼 */
    void sendGreeting();

public slots:
    void toggleVisible();
    void reloadAiConfig();
    /* 根据立绘图片在屏幕上的全局矩形，更新对话框位置（按比例偏移，固定相对位置） */
    void updatePositionRelativeToTachie(const QRect &globalTachieRect);

signals:
    void requestSetTachie(const QString &moodName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_btnNext_clicked();
    void on_btnHistory_clicked();
    void on_btnVoice_pressed();
    void on_btnVoice_released();

private:
    void initWindow();
    void removeBorder();
    void loadContext();
    void saveContext() const;
    void appendHistory(const QString &role, const QString &content);
    QJsonArray buildMessagesArray(const QString &input) const;
    void stopPendingState();
    void sendMessage(const QString &text);
    void refreshCachedAssets();
    QString buildSystemPrompt() const;

    static int findSentenceEnd(const QString &text, int from);
    static int estimateTokenCount(const QString &text);

    Ui::ChatDialog *ui = nullptr;
    QScopedPointer<HistoryPanel> m_historyPanel;

    AiProvider *m_ai = nullptr;
    VitsEngine *m_vits = nullptr;
    SpeechRecognizer *m_stt = nullptr;
    CommandExecutor *m_commandExecutor = nullptr;

    QAudioSource *m_audioSource = nullptr;
    QBuffer *m_audioBuffer = nullptr;
    bool m_recording = false;

    QJsonArray m_context;
    int m_maxHistoryRecords = 10;

    QString m_lastInput;
    QString m_streamRaw;
    QString m_streamChinese;
    bool m_vitsEnabled = false;
    bool m_vitsSentenceSplit = true;
    int m_vitsCursor = 0;

    /* 当前回合是否为启动招呼（不保存触发提示到用户历史） */
    bool m_greetingPending = false;

    bool m_historyVisible = false;
    QPoint m_lastPos;
    QList<int> m_pressedKeys;

    QStringList m_cachedTachieNames;
    QString m_cachedCharPrompt;
    QString m_cachedCharName;

    /* 对话框相对立绘的位置比例（从全局配置读取） */
    double m_dialogGapRatio = 0.05;
    double m_dialogOffsetYRatio = 0.08;
    QRect m_lastTachieRect;
};
