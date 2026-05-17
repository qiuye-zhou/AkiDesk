#pragma once

#include <QPoint>
#include <QShowEvent>
#include <QStringList>
#include <QWidget>

class AiProvider;
class VitsEngine;
class SpeechRecognizer;
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

public slots:
    void toggleVisible();
    void reloadAiConfig();

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
    void appendHistory(const QString &line);
    QString buildMessageWithContext(const QString &input) const;
    void stopPendingState();
    void sendMessage(const QString &text);

    static int findSentenceEnd(const QString &text, int from);

    Ui::ChatDialog *ui = nullptr;
    HistoryPanel *m_historyPanel = nullptr;

    AiProvider *m_ai = nullptr;
    VitsEngine *m_vits = nullptr;
    SpeechRecognizer *m_stt = nullptr;

    QAudioSource *m_audioSource = nullptr;
    QBuffer *m_audioBuffer = nullptr;
    bool m_recording = false;

    QStringList m_context;

    QString m_lastInput;
    QString m_streamRaw;
    QString m_streamChinese;
    bool m_vitsEnabled = false;
    bool m_vitsSentenceSplit = true;
    int m_vitsCursor = 0;

    bool m_historyVisible = false;
    QPoint m_lastPos;
    QList<int> m_pressedKeys;
};
