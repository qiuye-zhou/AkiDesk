#pragma once

#include <QWidget>

namespace Ui { class PagePlugin; }

/* 动画插件管理页面：展示已加载的插件列表和错误信息 */
class PagePlugin : public QWidget
{
    Q_OBJECT

public:
    explicit PagePlugin(QWidget *parent = nullptr);
    ~PagePlugin();

private:
    void refreshPluginList();
    Ui::PagePlugin *ui = nullptr;
};
