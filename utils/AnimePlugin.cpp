#include "AnimePlugin.h"
#include "config/JsonConfig.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

namespace
{
/* 解析单个动画步骤 */
bool parseStep(const QJsonObject &obj, PluginStep &out, QString &err)
{
    const QString type = obj.value("type").toString().trimmed();

    if (type == "move")
    {
        out.type = PluginStep::Type::Move;
        out.durationSec = obj.value("duration").toDouble(-1.0);
        out.x = obj.value("x").toDouble(0.0);
        out.y = obj.value("y").toDouble(0.0);
        if (out.durationSec <= 0.0)
        {
            err = "move 的 duration 必须大于 0";
            return false;
        }
        return true;
    }
    if (type == "opacity")
    {
        out.type = PluginStep::Type::Opacity;
        out.durationSec = obj.value("duration").toDouble(-1.0);
        out.from = obj.value("from").toDouble(-1.0);
        out.to = obj.value("to").toDouble(-1.0);
        if (out.durationSec <= 0.0)
        {
            err = "opacity 的 duration 必须大于 0";
            return false;
        }
        if (out.from < 0.0 || out.from > 1.0 || out.to < 0.0 || out.to > 1.0)
        {
            err = "opacity 的 from/to 必须在 0~1 之间";
            return false;
        }
        return true;
    }
    if (type == "scale")
    {
        out.type = PluginStep::Type::Scale;
        out.durationSec = obj.value("duration").toDouble(-1.0);
        out.scaleFrom = obj.value("from").toDouble(-1.0);
        out.scaleTo = obj.value("to").toDouble(-1.0);
        if (out.durationSec <= 0.0)
        {
            err = "scale 的 duration 必须大于 0";
            return false;
        }
        if (out.scaleFrom <= 0.0 || out.scaleTo <= 0.0)
        {
            err = "scale 的 from/to 必须大于 0";
            return false;
        }
        return true;
    }

    err = QString("不支持的步骤类型: %1").arg(type);
    return false;
}

/* 解析完整动画定义 */
bool parseAnimation(const QJsonObject &obj, PluginAnimation &out, QString &err)
{
    out.name = obj.value("name").toString().trimmed();
    if (out.name.isEmpty())
    {
        err = "动画缺少 name";
        return false;
    }

    const QJsonArray steps = obj.value("steps").toArray();
    if (steps.isEmpty())
    {
        err = QString("动画 %1 的 steps 不能为空").arg(out.name);
        return false;
    }

    for (int i = 0; i < steps.size(); ++i)
    {
        if (!steps[i].isObject())
        {
            err = QString("动画 %1 的第 %2 个步骤不是对象").arg(out.name).arg(i + 1);
            return false;
        }
        PluginStep step;
        if (!parseStep(steps[i].toObject(), step, err))
        {
            err = QString("动画 %1 的第 %2 步: %3").arg(out.name).arg(i + 1).arg(err);
            return false;
        }
        out.steps.append(step);
    }
    return true;
}
} // namespace

/* 从 JSON 文件加载一个完整的动画插件 */
bool loadPluginFromFile(const QString &filePath,
                        PluginDefinition &outPlugin,
                        QString &outError)
{
    JsonConfig cfg(filePath);
    if (!cfg.load())
    {
        outError = QString("无法读取插件文件: %1").arg(filePath);
        return false;
    }

    outPlugin = PluginDefinition();
    outPlugin.filePath = filePath;
    outPlugin.name = cfg.value("name").toString().trimmed();
    outPlugin.version = cfg.value("version").toString().trimmed();
    outPlugin.author = cfg.value("author").toString().trimmed();
    outPlugin.link = cfg.value("link").toString().trimmed();

    if (outPlugin.name.isEmpty() || outPlugin.version.isEmpty() ||
        outPlugin.author.isEmpty() || outPlugin.link.isEmpty())
    {
        outError = "插件必须包含 name/version/author/link";
        return false;
    }

    const QJsonArray animations = cfg.value("animations").toArray();
    if (animations.isEmpty())
    {
        outError = "animations 不能为空";
        return false;
    }

    QSet<QString> names;
    for (int i = 0; i < animations.size(); ++i)
    {
        if (!animations[i].isObject())
        {
            outError = QString("第 %1 个动画不是对象").arg(i + 1);
            return false;
        }
        PluginAnimation anim;
        if (!parseAnimation(animations[i].toObject(), anim, outError))
            return false;
        if (names.contains(anim.name))
        {
            outError = QString("动画名重复: %1").arg(anim.name);
            return false;
        }
        names.insert(anim.name);
        outPlugin.animations.append(anim);
    }
    return true;
}
