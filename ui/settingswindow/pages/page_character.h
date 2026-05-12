#pragma once

#include <QWidget>

class PluginManager;

namespace Ui { class PageCharacter; }

/* 角色管理页面：角色选择/导入/删除、Prompt 编辑、模型绑定、VITS 绑定 */
class PageCharacter : public QWidget
{
    Q_OBJECT

public:
    explicit PageCharacter(QWidget *parent = nullptr);
    ~PageCharacter();

    void refreshVitsModelList();

signals:
    void requestReloadCharSelect(const QString &name);
    void requestReloadAi();
    void requestSetTachieSize(int size);
    void requestResetTachieLoc();

private slots:
    void onCharChanged(const QString &name);
    void onPromptChanged();
    void onTachieSizeChanged(int value);
    void onServerChanged(const QString &server);
    void onModelChanged(const QString &model);
    void onVitsToggled(bool enabled);
    void onVitsModelChanged(const QString &model);
    void onImportCharacter();
    void onDeleteCharacter();

private:
    void loadCurrentConfig();
    void refreshCharList();
    void refreshModelList();
    void refreshTachieBindings();

    Ui::PageCharacter *ui;
    PluginManager *m_pluginManager;
    bool m_loading = false;
};
