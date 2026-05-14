#pragma once

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <functional>

class QNetworkAccessManager;

/* 百度语音识别引擎：获取 access_token + RAW PCM 识别 */
class SpeechRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechRecognizer(QObject *parent = nullptr);

    void setApiKey(const QString &key);
    void setSecretKey(const QString &key);

    /* 提交 PCM 音频数据进行识别（16000Hz, 单声道, 16bit） */
    void recognize(const QByteArray &pcmData);

signals:
    void recognized(const QString &text);
    void errorOccurred(const QString &msg);

private:
    void ensureAccessToken(std::function<void(bool)> callback);
    void doRecognize(const QByteArray &pcmData);

    QNetworkAccessManager *m_network;
    QString m_apiKey;
    QString m_secretKey;

    QString m_accessToken;
    QDateTime m_tokenExpiry;
};
