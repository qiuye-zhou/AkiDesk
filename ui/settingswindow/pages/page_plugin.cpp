#include "page_plugin.h"
#include "ui_page_plugin.h"

#include "utils/PluginManager.h"

#include <QMessageBox>

PagePlugin::PagePlugin(QWidget *parent)
    : QWidget(parent), ui(new Ui::PagePlugin)
{
    ui->setupUi(this);
    refreshPluginList();
    connect(ui->btnRefresh, &QPushButton::clicked, this, &PagePlugin::refreshPluginList);
}

PagePlugin::~PagePlugin() { delete ui; }

/* 扫描插件目录并刷新列表显示 */
void PagePlugin::refreshPluginList()
{
    PluginManager mgr;
    bool ok = mgr.reload();

    ui->listPlugins->clear();
    if (ok)
    {
        for (const auto &p : mgr.plugins())
        {
            ui->listPlugins->addItem(
                QString("%1 v%2 by %3 (%4 个动画)")
                    .arg(p.name, p.version, p.author)
                    .arg(p.animations.size()));
        }
    }

    ui->listErrors->clear();
    for (const QString &err : mgr.errors())
        ui->listErrors->addItem(err);

    /* 没有错误时隐藏错误区域 */
    ui->grpErrors->setVisible(!mgr.errors().isEmpty());
}
