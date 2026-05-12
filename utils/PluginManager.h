#pragma once

#include "AnimePlugin.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

/* 动画插件管理器：扫描、加载、索引所有动画插件 */
class PluginManager
{
public:
    /* 重新扫描插件目录并加载所有插件 */
    bool reload();

    const QList<PluginDefinition> &plugins() const;
    const QStringList &animationKeys() const;
    const QStringList &errors() const;

    /* 根据唯一键（插件名_动画名）查找动画 */
    bool findAnimation(const QString &uniqueKey,
                       PluginDefinition &outPlugin,
                       PluginAnimation &outAnimation) const;

private:
    struct Index
    {
        int pluginIdx = -1;
        int animIdx = -1;
    };

    QHash<QString, Index> m_keyIndex;
    QList<PluginDefinition> m_plugins;
    QStringList m_animationKeys;
    QStringList m_errors;
};
