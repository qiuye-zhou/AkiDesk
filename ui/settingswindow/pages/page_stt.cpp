#include "page_stt.h"
#include "ui_page_stt.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"

#include <QMediaDevices>
#include <QAudioDevice>

PageStt::PageStt(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageStt)
{
    ui->setupUi(this);

    JsonConfig cfg(GlobalConfigPath);
    ui->editApiKey->setText(cfg.value("stt/apiKey").toString());
    ui->editSecretKey->setText(cfg.value("stt/secretKey").toString());

    loadDevices();

    connect(ui->editApiKey, &QLineEdit::textChanged, this, &PageStt::onApiKeyChanged);
    connect(ui->editSecretKey, &QLineEdit::textChanged, this, &PageStt::onSecretKeyChanged);
    connect(ui->comboDevice, &QComboBox::currentIndexChanged, this, &PageStt::onDeviceChanged);
}

PageStt::~PageStt() { delete ui; }

void PageStt::loadDevices()
{
    const QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    QString savedId = JsonConfig(GlobalConfigPath).value("stt/deviceId").toString();

    int selectIndex = 0;
    for (int i = 0; i < devices.size(); ++i)
    {
        const QAudioDevice &dev = devices.at(i);
        ui->comboDevice->addItem(dev.description(), dev.id());
        if (dev.id() == savedId)
            selectIndex = i;
    }

    if (devices.isEmpty())
        ui->comboDevice->addItem("未检测到麦克风设备", "");

    ui->comboDevice->setCurrentIndex(selectIndex);
}

void PageStt::onApiKeyChanged(const QString &text)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("stt/apiKey", text);
    emit configChanged();
}

void PageStt::onSecretKeyChanged(const QString &text)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("stt/secretKey", text);
    emit configChanged();
}

void PageStt::onDeviceChanged(int index)
{
    if (index < 0)
        return;
    QString deviceId = ui->comboDevice->itemData(index).toString();
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("stt/deviceId", deviceId);
}
