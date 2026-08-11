#pragma once

#include <QWidget>

namespace Ui { class PageGeneral; }

/* 通用设置页面：历史记录上限等全局配置 */
class PageGeneral : public QWidget
{
    Q_OBJECT

public:
    explicit PageGeneral(QWidget *parent = nullptr);
    ~PageGeneral();

private slots:
    void onMaxHistoryChanged(int value);
    void onDialogGapRatioChanged(double value);
    void onDialogOffsetYRatioChanged(double value);

signals:
    void configChanged();

private:
    Ui::PageGeneral *ui = nullptr;
};
