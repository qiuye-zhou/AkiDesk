#include "AiProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QTimer>
#include <QPointer>

namespace {
constexpr int kTimeoutMs = 60000;
constexpr int kMaxParseIterations = 1000;
}

AiProvider::AiProvider(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

void AiProvider::setApiKey(const QString &apiKey) { m_apiKey = apiKey; }
void AiProvider::setApiUrl(const QString &url) { m_apiUrl = url; }
void AiProvider::setModel(const QString &model) { m_model = model; }
void AiProvider::setStreamEnabled(bool enabled) { m_streamEnabled = enabled; }
void AiProvider::setSystemPrompt(const QString &prompt) { m_systemPrompt = prompt; }

/* 请求可用模型列表 */
void AiProvider::fetchModels()
{
    QNetworkRequest request(QUrl(m_apiUrl + "/models"));
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    QNetworkReply *reply = m_network->get(request);

    QPointer<QNetworkReply> replyPtr(reply);
    QTimer::singleShot(kTimeoutMs, this, [replyPtr]() {
        if (replyPtr && replyPtr->isRunning())
        {
            replyPtr->abort();
        }
    });

    QPointer<AiProvider> self(this);
    connect(reply, &QNetworkReply::finished, this, [reply, self]() {
        if (!self)
        {
            reply->deleteLater();
            return;
        }
        self->handleModelsReply(reply);
    });
}

void AiProvider::handleModelsReply(QNetworkReply *reply)
{
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit errorOccurred(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
    {
        emit errorOccurred("模型列表响应格式错误");
        return;
    }

    QList<ModelInfo> models;
    const QJsonArray data = doc.object().value("data").toArray();
    for (const QJsonValue &val : data)
    {
        QJsonObject obj = val.toObject();
        ModelInfo info;
        info.id = obj.value("id").toString();
        info.created = obj.value("created").toString();
        info.ownedBy = obj.value("owned_by").toString();
        models.append(info);
    }
    emit modelsReceived(models);
}

void AiProvider::abortCurrentRequest()
{
    QNetworkReply *reply = m_activeReply;
    m_activeReply = nullptr;
    if (reply)
    {
        reply->abort();
        reply->deleteLater();
    }
    m_streamBuffers.clear();
    m_streamReplies.clear();
}

/* 发起对话请求 */
void AiProvider::chat(const QString &userMessage)
{
    abortCurrentRequest();

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = m_streamEnabled;

    QJsonArray messages;
    if (!m_systemPrompt.isEmpty())
    {
        QJsonObject sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = m_systemPrompt;
        messages.append(sysMsg);
    }
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messages.append(userMsg);
    body["messages"] = messages;

    QNetworkRequest request(QUrl(m_apiUrl + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    m_activeReply = m_network->post(request, QJsonDocument(body).toJson());
    QNetworkReply *reply = m_activeReply;
    m_streamBuffers[reply] = QByteArray();
    m_streamReplies[reply] = QString();

    QPointer<AiProvider> self(this);
    QPointer<QNetworkReply> replyPtr(reply);
    QTimer::singleShot(kTimeoutMs, this, [self, replyPtr]() {
        if (!self || !replyPtr)
            return;
        if (replyPtr->isRunning())
        {
            emit self->errorOccurred("请求超时");
            replyPtr->abort();
        }
    });
    if (m_streamEnabled)
    {
        connect(reply, &QNetworkReply::readyRead, this, [this, reply, self]()
                {
                    if (!self)
                        return;
                    handleStreamReadyRead(reply);
                });
        connect(reply, &QNetworkReply::finished, this, [this, reply, self]()
                {
                    if (!self)
                        return;
                    if (self->m_activeReply == reply)
                        self->m_activeReply = nullptr;
                    finalizeStreamReply(reply);
                });
    }
    else
    {
        connect(reply, &QNetworkReply::finished, this, [this, reply, self]()
                {
                    if (!self)
                        return;
                    if (self->m_activeReply == reply)
                        self->m_activeReply = nullptr;
                    if (reply->error() != QNetworkReply::NoError)
                    {
                        emit errorOccurred(reply->errorString());
                    }
                    else
                    {
                        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                        QJsonArray choices = doc.object()["choices"].toArray();
                        if (!choices.isEmpty())
                        {
                            QString content = choices[0].toObject()["message"].toObject()["content"].toString();
                            emit replyReceived(content);
                        }
                    }
                    m_streamBuffers.remove(reply);
                    m_streamReplies.remove(reply);
                    reply->deleteLater();
                });
    }
}

/* 处理 SSE 流式数据：data: {...}\n\n 格式 */
void AiProvider::handleStreamReadyRead(QNetworkReply *reply)
{
    if (!m_streamBuffers.contains(reply))
        return;

    m_streamBuffers[reply].append(reply->readAll());

    int iterations = 0;
    while (iterations++ < kMaxParseIterations)
    {
        int newlinePos = m_streamBuffers[reply].indexOf("\n");
        if (newlinePos < 0)
            break;

        QByteArray line = m_streamBuffers[reply].left(newlinePos).trimmed();
        m_streamBuffers[reply].remove(0, newlinePos + 1);

        if (line.isEmpty())
            continue;

        if (!line.startsWith("data:"))
            continue;
        QByteArray jsonPart = line.mid(5).trimmed();
        if (jsonPart == "[DONE]")
            continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            continue;

        QJsonArray choices = doc.object()["choices"].toArray();
        if (choices.isEmpty())
            continue;

        QString delta = choices[0].toObject()["delta"].toObject()["content"].toString();
        if (!delta.isEmpty())
        {
            m_streamReplies[reply] += delta;
            emit replyChunkReceived(delta);
        }
    }

    if (iterations >= kMaxParseIterations)
    {
        emit errorOccurred("流式数据解析次数超限");
    }
}

/* 流式结束时发出完整回复信号 */
void AiProvider::finalizeStreamReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::RemoteHostClosedError)
    {
        emit errorOccurred(reply->errorString());
    }
    else
    {
        /* 处理缓冲区中的剩余数据 */
        if (m_streamBuffers.contains(reply) && !m_streamBuffers[reply].isEmpty())
            handleStreamReadyRead(reply);
        emit replyReceived(m_streamReplies.value(reply));
    }
    m_streamBuffers.remove(reply);
    m_streamReplies.remove(reply);
    reply->deleteLater();
}

void AiProvider::cancelAll()
{
    abortCurrentRequest();
}
