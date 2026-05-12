#include "DragHelper.h"

#include <QMouseEvent>
#include <QWidget>

DragHelper::DragHelper(QWidget *target)
    : QObject(target), m_target(target)
{
    target->installEventFilter(this);
    target->setMouseTracking(true);
}

/* 拦截鼠标事件，实现无边框窗口的拖拽移动 */
bool DragHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_target)
        return QObject::eventFilter(watched, event);

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
        {
            m_dragging = true;
            m_dragStartPos = me->globalPosition().toPoint() - m_target->pos();
        }
        break;
    }
    case QEvent::MouseMove:
    {
        if (m_dragging)
        {
            auto *me = static_cast<QMouseEvent *>(event);
            m_target->move(me->globalPosition().toPoint() - m_dragStartPos);
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    {
        if (m_dragging)
        {
            m_dragging = false;
            emit dragFinished();
        }
        break;
    }
    default:
        break;
    }
    return false;
}
