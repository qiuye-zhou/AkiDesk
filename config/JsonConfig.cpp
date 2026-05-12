#include "JsonConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

JsonConfig::JsonConfig(const QString &filePath)
    : m_filePath(filePath)
{
    load();
}

bool JsonConfig::load()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    m_root = doc.object();
    return true;
}

bool JsonConfig::save()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    file.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
    return true;
}

/* 按 "/" 分隔 key，逐层嵌套读写 JSON 对象 */
QStringList JsonConfig::splitKey(const QString &key)
{
    return key.split('/', Qt::SkipEmptyParts);
}

QJsonValue JsonConfig::value(const QString &key,
                             const QJsonValue &defaultValue) const
{
    const QStringList parts = splitKey(key);
    QJsonObject current = m_root;
    for (int i = 0; i < parts.size() - 1; ++i)
    {
        if (!current.contains(parts[i]) || !current[parts[i]].isObject())
            return defaultValue;
        current = current[parts[i]].toObject();
    }
    const QString lastKey = parts.last();
    return current.contains(lastKey) ? current[lastKey] : defaultValue;
}

void JsonConfig::setValue(const QString &key, const QJsonValue &value)
{
    const QStringList parts = splitKey(key);
    if (parts.isEmpty())
        return;

    /* 构建完整的嵌套结构 */
    QJsonValue current = value;
    for (int i = parts.size() - 1; i >= 0; --i)
    {
        QJsonObject obj;
        obj[parts[i]] = current;
        current = obj;
    }

    /* 合并到根对象 */
    mergeObjects(m_root, current.toObject());
    save();
}

void JsonConfig::mergeObjects(QJsonObject &target, const QJsonObject &source)
{
    for (auto it = source.begin(); it != source.end(); ++it)
    {
        const QString &key = it.key();
        const QJsonValue &srcValue = it.value();

        if (srcValue.isObject() && target.contains(key) && target[key].isObject())
        {
            QJsonObject targetObj = target[key].toObject();
            mergeObjects(targetObj, srcValue.toObject());
            target[key] = targetObj;
        }
        else
        {
            target[key] = srcValue;
        }
    }
}
