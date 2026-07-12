#include "CommandExecutor.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"

#include <QDesktopServices>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>

CommandExecutor::CommandExecutor(QObject *parent)
    : QObject(parent)
{
    loadUrlShortcuts();
}

void CommandExecutor::loadUrlShortcuts()
{
    m_urlShortcuts.clear();

    JsonConfig cfg(GlobalConfigPath);

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
    bool success = executeCommand(cmd);
    if (success)
    {
        emit commandExecuted(QString("已成功执行命令：%1").arg(cmd.value));
    }
    else
    {
        emit commandExecuted(QString("执行命令失败：%1").arg(cmd.value));
        QString msg;
        if (cmd.type == Command::Type::OpenApp)
            msg = QString("无法打开应用 \"%1\"，请检查桌面上或开始菜单中是否有该应用的快捷方式。").arg(cmd.value);
        else if (cmd.type == Command::Type::OpenUrl)
            msg = QString("无法打开网址 \"%1\"。").arg(cmd.value);
        else if (cmd.type == Command::Type::Search)
            msg = QString("无法执行搜索 \"%1\"。").arg(cmd.value);
        else
            msg = QString("执行命令失败：%1").arg(cmd.value);

        QMessageBox msgBox;
        msgBox.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("执行失败");
        msgBox.setText(msg);
        msgBox.setStyleSheet("");
        msgBox.exec();
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
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString programsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString commonProgramsPath = QString(qgetenv("PROGRAMDATA")) + "/Microsoft/Windows/Start Menu/Programs";

    QStringList shortcutSearchPaths;
    shortcutSearchPaths << desktopPath << programsPath << commonProgramsPath;

    struct MatchResult
    {
        QString path;
        QString name;
        int score;
    };
    QList<MatchResult> matches;

    for (const QString &searchPath : shortcutSearchPaths)
    {
        QDirIterator it(searchPath, QStringList() << "*.lnk", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QString filePath = it.next();
            QFileInfo fileInfo(filePath);
            QString baseName = fileInfo.baseName().toLower();

            if (baseName.contains("卸载") || baseName.contains("uninstall") ||
                baseName.contains("remove") || baseName.contains("删除"))
            {
                continue;
            }

            int score = 0;
            if (baseName == appName)
            {
                score = 3;
            }
            else if (baseName.contains(appName))
            {
                score = 2;
            }
            else
            {
                bool allCharsFound = true;
                for (const QChar &c : appName)
                {
                    if (!baseName.contains(c))
                    {
                        allCharsFound = false;
                        break;
                    }
                }
                if (allCharsFound)
                {
                    score = 1;
                }
            }

            if (score > 0)
            {
                matches.append({filePath, baseName, score});
            }
        }
    }

    if (!matches.isEmpty())
    {
        std::sort(matches.begin(), matches.end(), [](const MatchResult &a, const MatchResult &b) {
            return a.score > b.score;
        });
        return QDesktopServices::openUrl(QUrl::fromLocalFile(matches.first().path));
    }

    return false;
}

bool CommandExecutor::searchWeb(const QString &query)
{
    QString encoded = QUrl::toPercentEncoding(query);
    QString url = QString("https://www.baidu.com/s?wd=%1").arg(encoded);
    return QDesktopServices::openUrl(QUrl(url));
}
