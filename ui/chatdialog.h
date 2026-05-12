#pragma once

#include <QStringList>
#include <QWidget>

class AiProvider;
class VitsEngine;
class HistoryPanel;

namespace Ui { class ChatDialog; }

/* 对话窗口：用户输入、AI 流式对话、上下文管理、语音播放 */
class ChatDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

public slots:
    void toggleVisible();
    void reloadAiConfig();

signals:
    /* AI 返回的心情用于切换立绘 */
    void requestSetTachie(const QString &moodName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_btnNext_clicked();

private:
    void initWindow();
    void loadContext();
    void saveContext() const;
    void appendHistory(const QString &line);
    QString buildMessageWithContext(const QString &input) const;
    void stopPendingState();
    void showHistoryPanel();
    void hideHistoryPanel();

    /* 从流式回复文本中寻找下一个句子结束位置 */
    static int findSentenceEnd(const QString &text, int from);

    Ui::ChatDialog *ui = nullptr;
    HistoryPanel *m_historyPanel = nullptr;

    AiProvider *m_ai = nullptr;
    VitsEngine *m_vits = nullptr;

    /* 对话上下文历史 */
    QStringList m_context;

    /* 流式回复状态 */
    QString m_lastInput;
    QString m_streamRaw;
    QString m_streamChinese;
    bool m_vitsEnabled = false;
    bool m_vitsSentenceSplit = true;
    int m_vitsCursor = 0;

    bool m_historyVisible = false;
    QPoint m_lastPos;
};
