#include "ui/chatdialog.h"
#include "ui/characterwindow.h"
#include "ui/settingswindow.h"

#include "config/AppPaths.h"
#include "app_version.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QStandardPaths>
#include <QSystemTrayIcon>

/* 递归复制 Qt 资源目录到本地文件系统（首次运行时部署默认配置） */
static void copyResourceDir(const QString &resPrefix, const QString &destDir)
{
    QDir dest(destDir);
    if (!dest.exists())
        QDir().mkpath(dest);

    QDir resDir(resPrefix);
    const QFileInfoList entries = resDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries)
    {
        if (fi.isDir())
        {
            /* 递归处理子目录 */
            copyResourceDir(fi.filePath(), dest.filePath(fi.fileName()));
        }
        else
        {
            /* 复制单个文件（仅在目标不存在时） */
            const QString destPath = dest.filePath(fi.fileName());
            if (!QFile::exists(destPath))
                QFile::copy(fi.filePath(), destPath);
        }
    }
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    const QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString projectDir = QDir(userDataDir).filePath("AkiDesk");

    /* 首次运行时，将内置的默认配置部署到用户文档目录 */
    const QString iniPath = LocalConfigPath;
    if (!QFile::exists(iniPath))
    {
        /* 部署默认角色资产 */
        copyResourceDir(":/defaults/Character/Assets", QDir(projectDir).filePath("Character/Assets"));
        /* 部署默认角色用户配置 */
        copyResourceDir(":/defaults/Character/Config", QDir(projectDir).filePath("Character/Config"));
        /* 部署默认动画插件 */
        copyResourceDir(":/defaults/Plugin/Anime", QDir(projectDir).filePath("Plugin/Anime"));
        /* 部署默认本地配置 */
        QFile::copy(":/defaults/config.ini", iniPath);
    }

    /* 确保所有数据目录存在 */
    QStringList dirs = {
        projectDir,
        QDir(projectDir).filePath("Character/Assets"),
        QDir(projectDir).filePath("Character/Config"),
        QDir(projectDir).filePath("Plugin/Anime")
    };
    for (const QString &d : dirs)
        QDir().mkpath(d);

    /* 创建主窗口 */
    ChatDialog chatWin;
    chatWin.show();

    CharacterWindow charWin;
    charWin.show();

    SettingsWindow *settingsWin = nullptr;

    /* 窗口间信号连接 */
    /* 右键立绘 → 切换对话框显隐 */
    QObject::connect(&charWin, &CharacterWindow::requestToggleChat,
                     &chatWin, &ChatDialog::toggleVisible);
    /* AI 返回心情 → 切换立绘图片 */
    QObject::connect(&chatWin, &ChatDialog::requestSetTachie,
                     &charWin, &CharacterWindow::setTachieImage);

    /* 系统托盘 */
    QSystemTrayIcon tray;
    tray.setIcon(QIcon(":/assets/logo.png"));
    tray.setToolTip(APP_NAME);
    tray.show();

    QMenu trayMenu;
    QAction *actSettings = trayMenu.addAction("设置");
    QAction *actQuit = trayMenu.addAction("退出");
    tray.setContextMenu(&trayMenu);

    /* 懒加载设置窗口：首次点击时才创建 */
    auto showSettings = [&]() {
        if (!settingsWin)
        {
            settingsWin = new SettingsWindow(&chatWin, &charWin);
            /* 设置窗口的信号连接 */
            QObject::connect(settingsWin, &SettingsWindow::requestReloadAi,
                             &chatWin, &ChatDialog::reloadAiConfig);
            QObject::connect(settingsWin, &SettingsWindow::requestReloadCharImage,
                             &charWin, &CharacterWindow::setTachieImage);
            QObject::connect(settingsWin, &SettingsWindow::requestSetTachieSize,
                             &charWin, &CharacterWindow::setTachieSize);
            QObject::connect(settingsWin, &SettingsWindow::requestResetTachieLoc,
                             &charWin, &CharacterWindow::resetPosition);
        }
        settingsWin->show();
        settingsWin->raise();
        settingsWin->activateWindow();
    };

    /* 左键单击托盘图标也打开设置 */
    QObject::connect(&tray, &QSystemTrayIcon::activated,
                     [&](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger ||
                             reason == QSystemTrayIcon::DoubleClick)
                             showSettings();
                     });
    QObject::connect(actSettings, &QAction::triggered, showSettings);
    QObject::connect(actQuit, &QAction::triggered, &app, &QApplication::quit);

    return app.exec();
}
