#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QMediaPlayer;
class QAudioOutput;
class QTemporaryFile;
class QNetworkReply;

/* VITS 语音合成引擎：按句合成 + 双队列串行播放 */
class VitsEngine : public QObject
{
    Q_OBJECT

public:
    explicit VitsEngine(QObject *parent = nullptr);
    ~VitsEngine();

    /* 设置合成参数 */
    void setApiUrl(const QString &url);
    void setSpeaker(const QString &speaker);

    /* 将文本追加到待合成队列，自动按队列顺序合成和播放 */
    void enqueueAndPlay(const QString &text);

    /* 停止所有合成和播放，清空队列 */
    void stopAll();

    bool isIdle() const;

signals:
    void playbackFinished();
    void errorOccurred(const QString &error);

private:
    void startNextSynthesis();
    void startNextPlayback();

    QNetworkAccessManager *m_network = nullptr;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    QString m_apiUrl;
    QString m_speaker;

    QStringList m_pendingTexts;
    QList<QTemporaryFile *> m_readyFiles;
    QTemporaryFile *m_currentFile = nullptr;
    QNetworkReply *m_currentReply = nullptr;
    bool m_synthesizing = false;
    bool m_stopped = false;
};
