#pragma once

#include <QMap>
#include <QObject>
#include <QString>

/* 命令执行器：基于白名单的安全命令执行，支持打开网站和应用程序 */
class CommandExecutor : public QObject
{
    Q_OBJECT

public:
    explicit CommandExecutor(QObject *parent = nullptr);

    struct Command
    {
        enum class Type { Unknown, OpenUrl, OpenApp, Search };
        Type type = Type::Unknown;
        QString value;
    };

    bool parseCommand(const QString &text, Command &out);

    bool executeCommand(const Command &cmd);

    bool executeCommandWithConfirm(const Command &cmd, QWidget *parent = nullptr);

    void loadAppWhitelist();

signals:
    void commandExecuted(const QString &result);

private:
    bool openUrl(const QString &urlStr);

    bool openApp(const QString &appName);

    bool searchWeb(const QString &query);

    QMap<QString, QString> m_urlShortcuts;
};
