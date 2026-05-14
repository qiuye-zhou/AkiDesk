#pragma once

#include <QWidget>

namespace Ui { class PageStt; }

/* 语音识别配置页面：百度 API Key、麦克风设备选择 */
class PageStt : public QWidget
{
    Q_OBJECT

public:
    explicit PageStt(QWidget *parent = nullptr);
    ~PageStt();

private slots:
    void onApiKeyChanged(const QString &text);
    void onSecretKeyChanged(const QString &text);
    void onDeviceChanged(int index);

signals:
    void configChanged();

private:
    void loadDevices();

    Ui::PageStt *ui;
};
