#include "ScrollHelper.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QWidget>

ScrollHelper::ScrollHelper(QWidget *target, QScrollBar *scrollBar,
                           int stepSize, QObject *parent)
    : QObject(parent), m_scrollBar(scrollBar), m_stepSize(stepSize)
{
    target->installEventFilter(this);
    auto *scrollArea = qobject_cast<QAbstractScrollArea *>(target);
    if (scrollArea && scrollArea->viewport())
        scrollArea->viewport()->installEventFilter(this);
}

/* 将滚轮事件转换为滚动条的值变化 */
bool ScrollHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_scrollBar || event->type() != QEvent::Wheel)
        return QObject::eventFilter(watched, event);

    auto *we = static_cast<QWheelEvent *>(event);
    const int delta = we->angleDelta().y();
    const int step = (delta > 0 ? -m_stepSize : m_stepSize);
    m_scrollBar->setValue(m_scrollBar->value() + step);
    return true;
}
