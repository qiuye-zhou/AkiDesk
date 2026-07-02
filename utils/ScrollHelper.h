#pragma once

#include <QObject>

class QScrollBar;
class QWidget;

/* 将目标控件的鼠标滚轮事件转发到指定的滚动条上 */
class ScrollHelper : public QObject
{
    Q_OBJECT

public:
    explicit ScrollHelper(QWidget *target, QScrollBar *scrollBar,
                          int stepSize = 5, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QScrollBar *m_scrollBar = nullptr;
    int m_stepSize = 5;
};
