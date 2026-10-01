#pragma once

#include <QObject>
#include <QPoint>

/* 为无边框窗口添加鼠标拖拽移动能力 */
class DragHelper : public QObject
{
    Q_OBJECT

public:
    explicit DragHelper(QWidget *target);

signals:
    void dragFinished();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *m_target = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};
