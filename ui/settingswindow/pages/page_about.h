#pragma once

#include <QWidget>

namespace Ui { class PageAbout; }

/* 关于页面：显示版本信息和项目链接 */
class PageAbout : public QWidget
{
    Q_OBJECT

public:
    explicit PageAbout(QWidget *parent = nullptr);
    ~PageAbout();

private:
    Ui::PageAbout *ui;
};
