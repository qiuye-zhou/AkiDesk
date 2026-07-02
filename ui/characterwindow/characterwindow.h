#pragma once

#include "utils/PluginManager.h"

#include <QWidget>

class QSequentialAnimationGroup;

namespace Ui { class CharacterWindow; }

/* 立绘窗口：显示角色图片、切换表情、播放动画、支持拖拽和鼠标穿透 */
class CharacterWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CharacterWindow(QWidget *parent = nullptr);
    ~CharacterWindow();

signals:
    /* 右键点击时请求切换对话框显隐 */
    void requestToggleChat();

public slots:
    void setTachieImage(const QString &name);
    void setTachieSize(int sizePercent);
    void resetPosition();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void savePosition();
    void restorePosition();
    void tryPlayAnimation(const QString &actionName);

    Ui::CharacterWindow *ui = nullptr;
    QPixmap m_currentPixmap;
    QImage m_scaledImage;
    QPoint m_scaledTopLeft{0, 0};
    bool m_positionRestored = false;
    PluginManager m_pluginManager;
    QSequentialAnimationGroup *m_activeAnim = nullptr;

#ifdef Q_OS_LINUX
    void applyInputShape(const QRegion &region);
    void applyInputShapeFromImage();
    void applyInputShapeFullWindow();
#endif
};
