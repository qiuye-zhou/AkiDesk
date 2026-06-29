#include "SpeechRecognizer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>
#include <QPointer>

namespace {
constexpr int kTimeoutMs = 15000;
}

SpeechRecognizer::SpeechRecognizer(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void SpeechRecognizer::setApiKey(const QString &key)
{
    if (m_apiKey != key)
    {
        m_apiKey = key;
        m_accessToken.clear();
    }
}

void SpeechRecognizer::setSecretKey(const QString &key)
{
    if (m_secretKey != key)
    {
        m_secretKey = key;
        m_accessToken.clear();
    }
}

/* 确保 access_token 有效，过期前自动刷新 */
void SpeechRecognizer::ensureAccessToken(std::function<void(bool)> callback)
{
    if (!m_accessToken.isEmpty() && QDateTime::currentDateTime() < m_tokenExpiry)
    {
        callback(true);
        return;
    }

    QNetworkRequest request(QUrl("https://aip.baidubce.com/oauth/2.0/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("grant_type", "client_credentials");
    params.addQueryItem("client_id", m_apiKey);
    params.addQueryItem("client_secret", m_secretKey);

    QNetworkReply *reply = m_network->post(request, params.query(QUrl::FullyEncoded).toUtf8());

    QPointer<QNetworkReply> replyPtr(reply);
    QTimer::singleShot(kTimeoutMs, this, [replyPtr]() {
        if (replyPtr && replyPtr->isRunning())
        {
            replyPtr->abort();
        }
    });

    QPointer<SpeechRecognizer> self(this);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, self]() {
        if (!self)
        {
            reply->deleteLater();
            return;
        }

        if (reply->error() != QNetworkReply::NoError)
        {
            emit errorOccurred("获取 access_token 失败: " + reply->errorString());
            reply->deleteLater();
            callback(false);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        if (obj.contains("error"))
        {
            emit errorOccurred("获取 access_token 失败: " + obj.value("error_description").toString());
            reply->deleteLater();
            callback(false);
            return;
        }

        m_accessToken = obj.value("access_token").toString();
        int expires = obj.value("expires_in").toInt(2592000);
        m_tokenExpiry = QDateTime::currentDateTime().addSecs(expires - 3600);

        reply->deleteLater();
        callback(true);
    });
}

void SpeechRecognizer::recognize(const QByteArray &pcmData)
{
    if (m_apiKey.isEmpty() || m_secretKey.isEmpty())
    {
        emit errorOccurred("请先在设置中配置百度语音识别 API Key 和 Secret Key");
        return;
    }
    if (pcmData.isEmpty())
    {
        emit errorOccurred("录音数据为空");
        return;
    }

    ensureAccessToken([this, pcmData](bool ok) {
        if (ok)
            doRecognize(pcmData);
    });
}

void SpeechRecognizer::doRecognize(const QByteArray &pcmData)
{
    QString url = QString("https://vop.baidu.com/server_api"
                          "?dev_pid=1537&cuid=AkiDesk&token=%1")
                      .arg(m_accessToken);

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "audio/pcm;rate=16000");
    request.setHeader(QNetworkRequest::ContentLengthHeader, pcmData.size());

    m_currentReply = m_network->post(request, pcmData);
    QNetworkReply *reply = m_currentReply;

    QPointer<SpeechRecognizer> self(this);
    QPointer<QNetworkReply> replyPtr(reply);
    QTimer::singleShot(kTimeoutMs, this, [self, replyPtr]() {
        if (!self || !replyPtr)
            return;
        if (replyPtr->isRunning())
        {
            emit self->errorOccurred("语音识别请求超时");
            replyPtr->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, self]() {
        if (!self)
            return;
        if (m_currentReply == reply)
            m_currentReply = nullptr;

        if (reply->error() != QNetworkReply::NoError)
        {
            emit errorOccurred("语音识别请求失败: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        int errNo = obj.value("err_no").toInt();
        if (errNo != 0)
        {
            emit errorOccurred("语音识别错误: " + obj.value("err_msg").toString());
            reply->deleteLater();
            return;
        }

        QJsonArray result = obj.value("result").toArray();
        if (result.isEmpty())
        {
            emit errorOccurred("未识别到内容");
            reply->deleteLater();
            return;
        }

        emit recognized(result.at(0).toString());
        reply->deleteLater();
    });
}

void SpeechRecognizer::cancel()
{
    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;
    if (reply)
    {
        reply->abort();
        reply->deleteLater();
    }
}
