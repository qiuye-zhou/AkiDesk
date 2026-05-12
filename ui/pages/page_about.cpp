#include "page_about.h"
#include "ui_page_about.h"

#include "app_version.h"

PageAbout::PageAbout(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageAbout)
{
    ui->setupUi(this);
    ui->labelVersion->setText(QString("版本：%1").arg(APP_VERSION));
}

PageAbout::~PageAbout() { delete ui; }
