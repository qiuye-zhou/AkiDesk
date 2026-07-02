#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

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
