#include "ui/chatdialog/chatdialog.h"
#include "ui/characterwindow/characterwindow.h"
#include "ui/settingswindow/settingswindow.h"

#include "config/AppPaths.h"
#include "app_version.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QResource>
#include <QStandardPaths>
#include <QSystemTrayIcon>

/* 复制单个资源文件到目标位置 */
static void copyResourceFile(const QString &resPath, const QString &destPath)
{
    if (!QFile::exists(destPath))
    {
        QFile::copy(resPath, destPath);
        QFile::setPermissions(destPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner |
            QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }
}

/* 部署默认配置文件 */
static void deployDefaultConfig(const QString &projectDir)
{
    /* 确保目录存在 */
    QDir().mkpath(QDir(projectDir).filePath("Character/Atri/Tachie"));
    QDir().mkpath(QDir(projectDir).filePath("Character/Config"));
    QDir().mkpath(QDir(projectDir).filePath("Plugin/Anime"));

    /* 复制角色配置文件 */
    copyResourceFile(":/defaults/Character/Atri/config.json",
                     QDir(projectDir).filePath("Character/Atri/config.json"));
    copyResourceFile(":/defaults/Character/Atri/context.json",
                     QDir(projectDir).filePath("Character/Atri/context.json"));
    copyResourceFile(":/defaults/Character/Atri/Tachie/default.png",
                     QDir(projectDir).filePath("Character/Atri/Tachie/default.png"));
    copyResourceFile(":/defaults/Character/Config/config.json",
                     QDir(projectDir).filePath("Character/Config/config.json"));

    /* 复制动画插件 */
    copyResourceFile(":/defaults/Plugin/Anime/Basic Animation Package.json",
                     QDir(projectDir).filePath("Plugin/Anime/Basic Animation Package.json"));
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QApplication::setWindowIcon(QIcon(":/assets/logo.png"));

    const QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString projectDir = QDir(userDataDir).filePath("AkiDesk");

    /* 首次运行时，将内置的默认配置部署到用户文档目录 */
    const QString iniPath = LocalConfigPath;
    if (!QFile::exists(iniPath))
    {
        /* 部署默认角色资产和用户配置 */
        deployDefaultConfig(projectDir);
        /* 部署默认本地配置 */
        QFile::copy(":/defaults/config.ini", iniPath);
        QFile::setPermissions(iniPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner |
            QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }

    /* 确保所有数据目录存在 */
    QStringList dirs = {
        projectDir,
        QDir(projectDir).filePath("Character"),
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

    QScopedPointer<SettingsWindow> settingsWin;

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
            settingsWin.reset(new SettingsWindow(&chatWin, &charWin));
            /* 设置窗口的信号连接 */
            QObject::connect(settingsWin.data(), &SettingsWindow::requestReloadAi,
                             &chatWin, &ChatDialog::reloadAiConfig);
            QObject::connect(settingsWin.data(), &SettingsWindow::requestReloadCharImage,
                             &charWin, &CharacterWindow::setTachieImage);
            QObject::connect(settingsWin.data(), &SettingsWindow::requestSetTachieSize,
                             &charWin, &CharacterWindow::setTachieSize);
            QObject::connect(settingsWin.data(), &SettingsWindow::requestResetTachieLoc,
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
