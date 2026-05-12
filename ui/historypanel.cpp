#include "historypanel.h"
#include "ui_historypanel.h"

#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>

HistoryPanel::HistoryPanel(QWidget *parent)
    : QWidget(parent), ui(new Ui::HistoryPanel)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

HistoryPanel::~HistoryPanel() { delete ui; }

void HistoryPanel::clear()
{
    QLayoutItem *child;
    while ((child = ui->contentLayout->takeAt(0)) != nullptr)
    {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
}

/* 添加一条历史记录条目，点击后发射 jumpToIndex 信号 */
void HistoryPanel::addItem(int index, const QString &role, const QString &text)
{
    auto *btn = new QPushButton(role + "：" + text, this);
    btn->setStyleSheet("text-align:left; padding:4px 8px; border:none; font-size:12px;");
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, this, [this, index]() {
        emit jumpToIndex(index);
    });
    ui->contentLayout->addWidget(btn);
}

/* 绘制圆角背景 + 阴影 */
void HistoryPanel::paintEvent(QPaintEvent *)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(5, 5, width() - 10, height() - 10), 12, 12);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(QColor(250, 250, 250)));
}
