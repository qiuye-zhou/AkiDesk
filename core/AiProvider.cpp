#include "AiProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

AiProvider::AiProvider(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

void AiProvider::setServiceType(ServiceType type)
{
    m_serviceType = type;
    /* 重置默认 URL */
    switch (type)
    {
    case OpenAI:
        m_apiUrl = "https://api.openai.com";
        break;
    case DeepSeek:
        m_apiUrl = "https://api.deepseek.com/v1";
        break;
    case Custom:
        break;
    }
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
    connect(reply, &QNetworkReply::finished, this, &AiProvider::handleModelsReply);
}

void AiProvider::handleModelsReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
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

/* 发起对话请求 */
void AiProvider::chat(const QString &userMessage)
{
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

    QNetworkReply *reply = m_network->post(request, QJsonDocument(body).toJson());
    m_streamBuffers[reply] = QByteArray();
    m_streamReplies[reply] = QString();

    if (m_streamEnabled)
    {
        connect(reply, &QNetworkReply::readyRead, this, [this, reply]()
                { handleStreamReadyRead(reply); });
        connect(reply, &QNetworkReply::finished, this, [this, reply]()
                { finalizeStreamReply(reply); });
    }
    else
    {
        connect(reply, &QNetworkReply::finished, this, [this, reply]()
                {
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
                    reply->deleteLater(); });
    }
}

/* 处理 SSE 流式数据：data: {...}\n\n 格式 */
void AiProvider::handleStreamReadyRead(QNetworkReply *reply)
{
    m_streamBuffers[reply].append(reply->readAll());

    while (true)
    {
        int newlinePos = m_streamBuffers[reply].indexOf("\n");
        if (newlinePos < 0)
            break;

        QByteArray line = m_streamBuffers[reply].left(newlinePos).trimmed();
        m_streamBuffers[reply].remove(0, newlinePos + 1);

        if (line.isEmpty())
            continue;

        /* SSE 格式：data: {...} */
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
        if (!m_streamBuffers[reply].isEmpty())
            handleStreamReadyRead(reply);
        emit replyReceived(m_streamReplies.value(reply));
    }
    m_streamBuffers.remove(reply);
    m_streamReplies.remove(reply);
    reply->deleteLater();
}
