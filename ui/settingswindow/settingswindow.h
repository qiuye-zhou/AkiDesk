#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class ChatDialog;
class CharacterWindow;

/* 设置窗口：macOS 风格侧边栏导航 */
class SettingsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SettingsWindow(ChatDialog *chat, CharacterWindow *tachie,
                            QWidget *parent = nullptr);

signals:
    void requestReloadAi();
    void requestReloadCharImage(const QString &name);
    void requestSetTachieSize(int size);
    void requestResetTachieLoc();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    QString buildStyleSheet() const;
    ChatDialog *m_chat;
    CharacterWindow *m_tachie;
    QListWidget *m_sidebar;
    QStackedWidget *m_stack;
};
