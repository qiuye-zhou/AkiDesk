#include "page_general.h"
#include "ui_page_general.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"

PageGeneral::PageGeneral(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageGeneral)
{
    ui->setupUi(this);

    JsonConfig cfg(GlobalConfigPath);
    int maxHistory = cfg.value("llm/maxHistoryRecords", 10).toInt();
    ui->spinMaxHistory->setValue(maxHistory < 2 ? 2 : maxHistory);

    connect(ui->spinMaxHistory, qOverload<int>(&QSpinBox::valueChanged), this, &PageGeneral::onMaxHistoryChanged);
}

PageGeneral::~PageGeneral() { delete ui; }

void PageGeneral::onMaxHistoryChanged(int value)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("llm/maxHistoryRecords", value);
    emit configChanged();
}
