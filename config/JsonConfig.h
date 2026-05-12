#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>

/* 轻量级 JSON 配置读写工具，支持路径式 key（如 "llm/DeepSeek/ApiKey"） */
class JsonConfig
{
public:
    explicit JsonConfig(const QString &filePath);

    bool load();
    bool save();

    void setValue(const QString &key, const QJsonValue &value);
    QJsonValue value(const QString &key,
                     const QJsonValue &defaultValue = QJsonValue()) const;

private:
    QString m_filePath;
    QJsonObject m_root;

    static QStringList splitKey(const QString &key);
    static void mergeObjects(QJsonObject &target, const QJsonObject &source);
};
