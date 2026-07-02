#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

/* OpenAI 兼容 API 封装，支持流式 SSE 和非流式两种模式 */
class AiProvider : public QObject
{
    Q_OBJECT

public:
    struct ModelInfo {
        QString id;
        QString created;
        QString ownedBy;
    };

    explicit AiProvider(QObject *parent = nullptr);

    void setApiKey(const QString &apiKey);
    void setApiUrl(const QString &url);
    void setModel(const QString &model);
    void setStreamEnabled(bool enabled);
    void setSystemPrompt(const QString &prompt);

    QString currentModel() const { return m_model; }
    QString currentApiUrl() const { return m_apiUrl; }

    /* 拉取可用模型列表 */
    void fetchModels();

    /* 发起对话请求 */
    void chat(const QString &userMessage);

    /* 取消所有正在进行的请求 */
    void cancelAll();

signals:
    /* 非流式完整回复 */
    void replyReceived(const QString &reply);
    /* 流式每收到一个 chunk 触发 */
    void replyChunkReceived(const QString &chunk);
    /* 请求出错 */
    void errorOccurred(const QString &error);
    /* 模型列表返回 */
    void modelsReceived(const QList<ModelInfo> &models);

private:
    void handleModelsReply(QNetworkReply *reply);
    void handleStreamReadyRead(QNetworkReply *reply);
    void finalizeStreamReply(QNetworkReply *reply);
    void abortCurrentRequest();

    QNetworkAccessManager *m_network = nullptr;
    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
    bool m_streamEnabled = true;
    QString m_systemPrompt;

    QNetworkReply *m_activeReply = nullptr;
    QHash<QNetworkReply *, QByteArray> m_streamBuffers;
    QHash<QNetworkReply *, QString> m_streamReplies;
};
