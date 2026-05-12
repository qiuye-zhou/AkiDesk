#include "historypanel.h"
#include "ui_historypanel.h"

#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

HistoryPanel::HistoryPanel(QWidget *parent)
    : QWidget(parent), ui(new Ui::HistoryPanel)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowOpacity(0.9);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->scrollArea->setWidgetResizable(true);
    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this,
            [this]() {
                ui->scrollArea->verticalScrollBar()->setValue(
                    ui->scrollArea->verticalScrollBar()->maximum());
            });
}

HistoryPanel::~HistoryPanel() { delete ui; }

void HistoryPanel::clear()
{
    QVBoxLayout *layout =
        qobject_cast<QVBoxLayout *>(ui->scrollAreaWidgetContents->layout());
    if (!layout)
        return;

    while (QLayoutItem *item = layout->takeAt(0))
    {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
}

void HistoryPanel::addItem(int index, const QString &role, const QString &text)
{
    QVBoxLayout *layout =
        qobject_cast<QVBoxLayout *>(ui->scrollAreaWidgetContents->layout());
    if (!layout)
        return;

    auto *itemWidget = new QWidget(this);
    auto *itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(4, 4, 4, 4);

    auto *lblRole = new QLabel(role, itemWidget);
    lblRole->setFixedWidth(30);
    lblRole->setStyleSheet("font-weight:bold; font-size:12px;");

    auto *lblText = new QLabel(text, itemWidget);
    lblText->setWordWrap(true);
    lblText->setStyleSheet("font-size:12px;");

    auto *btnJump = new QPushButton(itemWidget);
    btnJump->setToolTip("回溯到这条记录");
    btnJump->setFixedSize(24, 24);
    btnJump->setStyleSheet(
        "QPushButton { background:transparent; border:none; border-radius:5px; }"
        "QPushButton:hover { border:2px solid #CCCCCC; }"
        "QPushButton:pressed { border:2px solid #AAAAAA; }");
    btnJump->setText(QString::fromUtf8("\xe2\x86\xa9"));
    connect(btnJump, &QPushButton::clicked, this, [this, index]() {
        emit jumpToIndex(index);
    });

    itemLayout->addWidget(lblRole);
    itemLayout->addWidget(lblText, 1);
    itemLayout->addWidget(btnJump);

    layout->addWidget(itemWidget);
}

void HistoryPanel::paintEvent(QPaintEvent *)
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QRectF rect(5, 5, width() - 10, height() - 10);
    path.addRoundedRect(rect, 15, 15);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(Qt::white));

    QColor color(0, 0, 0, 50);
    for (int i = 0; i < 5; ++i)
    {
        QPainterPath shadowPath;
        shadowPath.setFillRule(Qt::WindingFill);
        QRectF shadowRect(5 - i, 5 - i, width() - (5 - i) * 2, height() - (5 - i) * 2);
        shadowPath.addRoundedRect(shadowRect, 15, 15);
        color.setAlpha(50 - qSqrt(i) * 22);
        painter.setPen(color);
        painter.drawPath(shadowPath);
    }
}
