#include "settingswindow.h"
#include "ui/chatdialog/chatdialog.h"
#include "ui/characterwindow/characterwindow.h"

#include "pages/page_llm.h"
#include "pages/page_character.h"
#include "pages/page_vits.h"
#include "pages/page_plugin.h"
#include "pages/page_about.h"

#include <QTabWidget>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(ChatDialog *chat, CharacterWindow *tachie,
                               QWidget *parent)
    : QMainWindow(parent), m_chat(chat), m_tachie(tachie)
{
    setupUi();
}

void SettingsWindow::setupUi()
{
    setWindowTitle("设置");
    resize(600, 500);

    auto *tabs = new QTabWidget(this);

    /* 各个子页面 */
    auto *pageLLM = new PageLLM(this);
    auto *pageChar = new PageCharacter(this);
    auto *pageVits = new PageVits(this);
    auto *pagePlugin = new PagePlugin(this);
    auto *pageAbout = new PageAbout(this);

    tabs->addTab(pageLLM, "对话模型");
    tabs->addTab(pageChar, "角色设置");
    tabs->addTab(pageVits, "语音合成");
    tabs->addTab(pagePlugin, "插件管理");
    tabs->addTab(pageAbout, "关于");

    setCentralWidget(tabs);

    /* 角色页面信号连接 */
    connect(pageChar, &PageCharacter::requestReloadCharSelect, this,
            [this](const QString &name) { emit requestReloadCharImage(name); });
    connect(pageChar, &PageCharacter::requestReloadAi, this,
            [this]() { emit requestReloadAi(); });
    connect(pageChar, &PageCharacter::requestSetTachieSize, this,
            [this](int s) { emit requestSetTachieSize(s); });
    connect(pageChar, &PageCharacter::requestResetTachieLoc, this,
            [this]() { emit requestResetTachieLoc(); });

    /* VITS 页面信号 */
    connect(pageVits, &PageVits::vitsModelListRefreshed, pageChar,
            &PageCharacter::refreshVitsModelList);
}
