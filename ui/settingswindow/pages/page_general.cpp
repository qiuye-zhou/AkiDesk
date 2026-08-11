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

    double gapRatio = cfg.value("dialog/gapRatio", 0.05).toDouble();
    double offsetYRatio = cfg.value("dialog/offsetYRatio", 0.08).toDouble();
    ui->spinDialogGapRatio->setValue(gapRatio);
    ui->spinDialogOffsetYRatio->setValue(offsetYRatio);

    connect(ui->spinMaxHistory, qOverload<int>(&QSpinBox::valueChanged), this, &PageGeneral::onMaxHistoryChanged);
    connect(ui->spinDialogGapRatio, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &PageGeneral::onDialogGapRatioChanged);
    connect(ui->spinDialogOffsetYRatio, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &PageGeneral::onDialogOffsetYRatioChanged);
}

PageGeneral::~PageGeneral() { delete ui; }

void PageGeneral::onMaxHistoryChanged(int value)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("llm/maxHistoryRecords", value);
    emit configChanged();
}

void PageGeneral::onDialogGapRatioChanged(double value)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("dialog/gapRatio", value);
    emit configChanged();
}

void PageGeneral::onDialogOffsetYRatioChanged(double value)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("dialog/offsetYRatio", value);
    emit configChanged();
}
