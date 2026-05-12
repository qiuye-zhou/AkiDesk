#pragma once

#include <QMainWindow>

class ChatDialog;
class CharacterWindow;

/* 设置窗口：使用 TabWidget 组织各个设置子页面 */
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

private:
    void setupUi();
    ChatDialog *m_chat;
    CharacterWindow *m_tachie;
};
