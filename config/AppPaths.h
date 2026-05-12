#pragma once

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

/* 应用程序所有可迁移的全局配置（API Key 等） */
inline const QString GlobalConfigPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("AkiDesk/config.json");

/* 不可迁移的本地配置（窗口位置、大小等） */
inline const QString LocalConfigPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("AkiDesk/config.ini");

/* 角色资产根目录（只读资源：立绘、角色Prompt） */
inline const QString CharacterAssetsPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("AkiDesk/Character/Assets");

/* 角色用户配置根目录（运行时配置、上下文历史） */
inline const QString CharacterConfigPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("AkiDesk/Character/Config");

/* 动画插件目录 */
inline const QString AnimePluginPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("AkiDesk/Plugin/Anime");

/* 读取当前选中的角色名称（从本地 ini 配置） */
inline QString CurrentCharacterName()
{
    QSettings settings(LocalConfigPath, QSettings::IniFormat);
    return settings.value("character/Selected", "").toString();
}

/* 获取当前角色的立绘图片目录，角色未选择时返回空 */
inline QString CurrentCharacterTachiePath()
{
    const QString name = CurrentCharacterName();
    if (name.isEmpty())
        return {};
    const QString path = QDir(CharacterAssetsPath).filePath(name + "/Tachie");
    return QDir(path).exists() ? path : QString();
}

/* 获取当前角色的资产配置文件路径（角色 Prompt、动画绑定等） */
inline QString CurrentCharacterAssetConfig()
{
    const QString name = CurrentCharacterName();
    if (name.isEmpty())
        return {};
    return QDir(CharacterAssetsPath).filePath(name + "/config.json");
}

/* 获取当前角色的用户运行配置路径（立绘大小、模型选择等） */
inline QString CurrentCharacterUserConfig()
{
    const QString name = CurrentCharacterName();
    if (name.isEmpty())
        return {};
    return QDir(CharacterConfigPath).filePath(name + "/config.json");
}

/* 获取当前角色的对话上下文路径 */
inline QString CurrentCharacterContextPath()
{
    const QString name = CurrentCharacterName();
    if (name.isEmpty())
        return {};
    return QDir(CharacterConfigPath).filePath(name + "/context.json");
}
