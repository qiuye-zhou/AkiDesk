#include "VitsEngine.h"

#include <QAudioOutput>
#include <QDir>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QUrl>

VitsEngine::VitsEngine(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audioOutput);

    /* 当前音频播放完毕后，尝试播放队列中的下一段 */
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
                if (state == QMediaPlayer::StoppedState)
                {
                    if (m_currentFile)
                    {
                        m_currentFile->deleteLater();
                        m_currentFile = nullptr;
                    }
                    startNextPlayback();
                }
            });
}

void VitsEngine::setApiUrl(const QString &url) { m_apiUrl = url; }
void VitsEngine::setModel(const QString &model) { m_model = model; }
void VitsEngine::setSpeaker(const QString &speaker) { m_speaker = speaker; }

void VitsEngine::enqueueAndPlay(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;
    m_pendingTexts.append(text.trimmed());
    startNextSynthesis();
}

void VitsEngine::stopAll()
{
    m_pendingTexts.clear();
    m_synthesizing = false;
    m_player->stop();
    for (auto *f : m_readyFiles)
        f->deleteLater();
    m_readyFiles.clear();
    if (m_currentFile)
    {
        m_currentFile->deleteLater();
        m_currentFile = nullptr;
    }
}

bool VitsEngine::isIdle() const
{
    return !m_synthesizing && m_pendingTexts.isEmpty() && m_readyFiles.isEmpty();
}

/* 发起下一个合成请求 */
void VitsEngine::startNextSynthesis()
{
    if (m_synthesizing || m_pendingTexts.isEmpty())
        return;

    m_synthesizing = true;
    const QString text = m_pendingTexts.takeFirst();

    /* 构建请求 URL：/voice/{text}?id={speaker}&model={model} */
    QString url = QString("%1/voice/%2?model=%3&id=%4")
                      .arg(m_apiUrl)
                      .arg(QString(QUrl::toPercentEncoding(text)))
                      .arg(QString(QUrl::toPercentEncoding(m_model)))
                      .arg(QString(QUrl::toPercentEncoding(m_speaker)));

    QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_synthesizing = false;
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray audioData = reply->readAll();
            if (!audioData.isEmpty())
            {
                auto *tmp = new QTemporaryFile(
                    QDir::tempPath() + "/vits_XXXXXX.mp3", this);
                if (tmp->open())
                {
                    tmp->write(audioData);
                    tmp->flush();
                    m_readyFiles.append(tmp);
                    startNextPlayback();
                }
                else
                {
                    delete tmp;
                }
            }
        }
        reply->deleteLater();
        /* 合成完成后立即尝试发起下一个合成请求 */
        startNextSynthesis();
    });
}

/* 播放就绪队列中的下一段音频 */
void VitsEngine::startNextPlayback()
{
    if (m_player->playbackState() != QMediaPlayer::StoppedState)
        return;
    if (m_readyFiles.isEmpty())
        return;

    m_currentFile = m_readyFiles.takeFirst();
    m_player->setSource(QUrl::fromLocalFile(m_currentFile->fileName()));
    m_player->play();
}
