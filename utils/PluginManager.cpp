#include "PluginManager.h"
#include "config/AppPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

bool PluginManager::reload()
{
    m_plugins.clear();
    m_animationKeys.clear();
    m_keyIndex.clear();
    m_errors.clear();

    QDir dir(AnimePluginPath);
    const QFileInfoList files =
        dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);

    QSet<QString> usedNames;
    for (const QFileInfo &fi : files)
    {
        PluginDefinition plugin;
        QString error;
        if (!loadPluginFromFile(fi.filePath(), plugin, error))
        {
            m_errors.append(QString("[%1] %2").arg(fi.fileName(), error));
            continue;
        }
        if (usedNames.contains(plugin.name))
        {
            m_errors.append(QString("[%1] 插件名重复: %2").arg(fi.fileName(), plugin.name));
            continue;
        }
        usedNames.insert(plugin.name);

        int pi = m_plugins.size();
        m_plugins.append(plugin);

        /* 为该插件下的所有动画建立全局唯一键索引 */
        for (int i = 0; i < plugin.animations.size(); ++i)
        {
            QString key = plugin.animations[i].buildKey(plugin.name);
            if (m_keyIndex.contains(key))
            {
                m_errors.append(QString("动画键冲突: %1").arg(key));
                continue;
            }
            m_animationKeys.append(key);
            m_keyIndex.insert(key, {pi, i});
        }
    }
    return !m_plugins.isEmpty();
}

const QList<PluginDefinition> &PluginManager::plugins() const { return m_plugins; }
const QStringList &PluginManager::animationKeys() const { return m_animationKeys; }
const QStringList &PluginManager::errors() const { return m_errors; }

bool PluginManager::findAnimation(const QString &uniqueKey,
                                  PluginDefinition &outPlugin,
                                  PluginAnimation &outAnimation) const
{
    if (uniqueKey.isEmpty())
        return false;

    auto it = m_keyIndex.constFind(uniqueKey);
    if (it == m_keyIndex.constEnd())
        return false;

    const Index &idx = it.value();
    if (idx.pluginIdx < 0 || idx.pluginIdx >= m_plugins.size())
        return false;

    const PluginDefinition &p = m_plugins.at(idx.pluginIdx);
    if (idx.animIdx < 0 || idx.animIdx >= p.animations.size())
        return false;

    outPlugin = p;
    outAnimation = p.animations.at(idx.animIdx);
    return true;
}
