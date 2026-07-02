#include "CommandExecutor.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>
#include <QWidget>

CommandExecutor::CommandExecutor(QObject *parent)
    : QObject(parent)
{
    loadAppWhitelist();
}

void CommandExecutor::loadAppWhitelist()
{
    m_appWhitelist.clear();
    m_urlShortcuts.clear();

    JsonConfig cfg(GlobalConfigPath);

    QJsonArray apps = cfg.value("commands/apps").toArray();
    for (const QJsonValue &v : apps)
    {
        QJsonObject obj = v.toObject();
        QString name = obj.value("name").toString().trimmed().toLower();
        QString path = obj.value("path").toString().trimmed();
        if (!name.isEmpty() && !path.isEmpty())
        {
            m_appWhitelist[name] = path;
        }
    }

    if (m_appWhitelist.isEmpty())
    {
#ifdef Q_OS_WIN
        m_appWhitelist["vscode"] = "code";
        m_appWhitelist["vs code"] = "code";
        m_appWhitelist["visual studio code"] = "code";
        m_appWhitelist["notepad"] = "notepad";
        m_appWhitelist["计算器"] = "calc";
        m_appWhitelist["calculator"] = "calc";
        m_appWhitelist["画图"] = "mspaint";
        m_appWhitelist["paint"] = "mspaint";
        m_appWhitelist["资源管理器"] = "explorer";
        m_appWhitelist["explorer"] = "explorer";
        m_appWhitelist["终端"] = "cmd";
        m_appWhitelist["terminal"] = "cmd";
        m_appWhitelist["powershell"] = "powershell";
        m_appWhitelist["chrome"] = "chrome";
        m_appWhitelist["google chrome"] = "chrome";
        m_appWhitelist["edge"] = "msedge";
        m_appWhitelist["steam"] = "steam";
        m_appWhitelist["微信"] = "wechat";
        m_appWhitelist["qq"] = "qq";
#else
        m_appWhitelist["vscode"] = "code";
        m_appWhitelist["vs code"] = "code";
        m_appWhitelist["notepad"] = "gedit";
        m_appWhitelist["terminal"] = "gnome-terminal";
        m_appWhitelist["chrome"] = "google-chrome";
        m_appWhitelist["firefox"] = "firefox";
#endif
    }

    QJsonArray urls = cfg.value("commands/urls").toArray();
    for (const QJsonValue &v : urls)
    {
        QJsonObject obj = v.toObject();
        QString name = obj.value("name").toString().trimmed().toLower();
        QString url = obj.value("url").toString().trimmed();
        if (!name.isEmpty() && !url.isEmpty())
        {
            m_urlShortcuts[name] = url;
        }
    }

    if (m_urlShortcuts.isEmpty())
    {
        m_urlShortcuts["b站"] = "https://www.bilibili.com";
        m_urlShortcuts["bilibili"] = "https://www.bilibili.com";
        m_urlShortcuts["哔哩哔哩"] = "https://www.bilibili.com";
        m_urlShortcuts["百度"] = "https://www.baidu.com";
        m_urlShortcuts["baidu"] = "https://www.baidu.com";
        m_urlShortcuts["谷歌"] = "https://www.google.com";
        m_urlShortcuts["google"] = "https://www.google.com";
        m_urlShortcuts["github"] = "https://github.com";
        m_urlShortcuts["gitlab"] = "https://gitlab.com";
        m_urlShortcuts["知乎"] = "https://www.zhihu.com";
        m_urlShortcuts["zhihu"] = "https://www.zhihu.com";
        m_urlShortcuts["淘宝"] = "https://www.taobao.com";
        m_urlShortcuts["taobao"] = "https://www.taobao.com";
        m_urlShortcuts["京东"] = "https://www.jd.com";
        m_urlShortcuts["jd"] = "https://www.jd.com";
        m_urlShortcuts["抖音"] = "https://www.douyin.com";
        m_urlShortcuts["douyin"] = "https://www.douyin.com";
        m_urlShortcuts["youtube"] = "https://www.youtube.com";
        m_urlShortcuts["twitter"] = "https://twitter.com";
        m_urlShortcuts["x"] = "https://twitter.com";
        m_urlShortcuts["微博"] = "https://weibo.com";
        m_urlShortcuts["weibo"] = "https://weibo.com";
    }
}

bool CommandExecutor::parseCommand(const QString &text, Command &out)
{
    QString trimmed = text.trimmed();
    out = Command();

    QRegularExpression cmdRegex(R"(COMMAND:\s*(\w+)\s*:\s*(.+))", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = cmdRegex.match(trimmed);
    if (match.hasMatch())
    {
        QString typeStr = match.captured(1).toLower();
        QString value = match.captured(2).trimmed();

        if (typeStr == "openurl" || typeStr == "url")
        {
            out.type = Command::Type::OpenUrl;
            out.value = value;
            return true;
        }
        else if (typeStr == "openapp" || typeStr == "app")
        {
            out.type = Command::Type::OpenApp;
            out.value = value.toLower();
            return true;
        }
        else if (typeStr == "search")
        {
            out.type = Command::Type::Search;
            out.value = value;
            return true;
        }
    }

    return false;
}

bool CommandExecutor::executeCommand(const Command &cmd)
{
    switch (cmd.type)
    {
    case Command::Type::OpenUrl:
        return openUrl(cmd.value);
    case Command::Type::OpenApp:
        return openApp(cmd.value);
    case Command::Type::Search:
        return searchWeb(cmd.value);
    default:
        break;
    }
    return false;
}

bool CommandExecutor::executeCommandWithConfirm(const Command &cmd, QWidget *parent)
{
    QString title;
    QString msg;

    switch (cmd.type)
    {
    case Command::Type::OpenUrl:
        title = "打开网站";
        msg = QString("确定要打开网站吗？\n\n%1").arg(cmd.value);
        break;
    case Command::Type::OpenApp:
        title = "打开应用";
        msg = QString("确定要打开应用吗？\n\n%1").arg(cmd.value);
        break;
    case Command::Type::Search:
        title = "网页搜索";
        msg = QString("确定要搜索吗？\n\n%1").arg(cmd.value);
        break;
    default:
        return false;
    }

    int ret = QMessageBox::question(parent, title, msg,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return false;

    bool success = executeCommand(cmd);
    if (success)
    {
        emit commandExecuted(QString("已成功执行命令：%1").arg(cmd.value));
    }
    else
    {
        emit commandExecuted(QString("执行命令失败：%1").arg(cmd.value));
    }
    return success;
}

bool CommandExecutor::openUrl(const QString &urlStr)
{
    QString input = urlStr.trimmed();
    QString inputLower = input.toLower();

    if (m_urlShortcuts.contains(inputLower))
    {
        return QDesktopServices::openUrl(QUrl(m_urlShortcuts[inputLower]));
    }

    QString finalUrlStr = input;
    if (!finalUrlStr.startsWith("http://", Qt::CaseInsensitive) && 
        !finalUrlStr.startsWith("https://", Qt::CaseInsensitive))
    {
        finalUrlStr = "https://" + finalUrlStr;
    }

    QUrl finalUrl(finalUrlStr);
    if (!finalUrl.isValid())
        return false;

    QString scheme = finalUrl.scheme().toLower();
    if (scheme != "http" && scheme != "https")
        return false;

    QString host = finalUrl.host().toLower();
    if (host.isEmpty())
        return false;

    return QDesktopServices::openUrl(finalUrl);
}

bool CommandExecutor::openApp(const QString &appName)
{
    QString lowerName = appName.toLower();

    if (m_appWhitelist.contains(lowerName))
    {
        QString path = m_appWhitelist[lowerName];
        return QProcess::startDetached(path);
    }

    for (const QString &key : m_appWhitelist.keys())
    {
        if (lowerName.length() <= key.length() && key.contains(lowerName))
        {
            QString path = m_appWhitelist[key];
            return QProcess::startDetached(path);
        }
    }

    return false;
}

bool CommandExecutor::searchWeb(const QString &query)
{
    QString encoded = QUrl::toPercentEncoding(query);
    QString url = QString("https://www.baidu.com/s?wd=%1").arg(encoded);
    return QDesktopServices::openUrl(QUrl(url));
}
