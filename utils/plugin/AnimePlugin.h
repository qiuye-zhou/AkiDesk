#pragma once

#include <QList>
#include <QString>
#include <QStringList>

/* 动画步骤类型枚举 */
struct PluginStep
{
    enum class Type { Move, Opacity, Scale };

    Type type = Type::Move;
    double durationSec = 0.0;

    /* Move: 相对位移像素 */
    double x = 0.0;
    double y = 0.0;

    /* Opacity: 透明度 0.0~1.0 */
    double from = 1.0;
    double to = 1.0;

    /* Scale: 缩放倍率（1.0 = 100%） */
    double scaleFrom = 1.0;
    double scaleTo = 1.0;
};

/* 由多个步骤组成的完整动画 */
struct PluginAnimation
{
    QString name;
    QList<PluginStep> steps;

    /* 生成全局唯一键：插件名_动画名 */
    QString buildKey(const QString &pluginName) const
    {
        return pluginName + "_" + name;
    }
};

/* 动画插件定义（一个 JSON 文件对应一个插件） */
struct PluginDefinition
{
    QString filePath;
    QString name;
    QString version;
    QString author;
    QString link;
    QList<PluginAnimation> animations;
};

/* 从 JSON 文件加载动画插件 */
bool loadPluginFromFile(const QString &filePath,
                        PluginDefinition &outPlugin,
                        QString &outError);
