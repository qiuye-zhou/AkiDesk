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
    itemWidget->setStyleSheet(
        "QWidget { background:transparent; border-radius:6px; }"
        "QWidget:hover { background:#F2F2F7; }");
    auto *itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(6, 6, 6, 6);

    auto *lblRole = new QLabel(role, itemWidget);
    lblRole->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    lblRole->setStyleSheet("font-weight:bold; font-size:12px; color:#8E8E93;");

    auto *lblText = new QLabel(text, itemWidget);
    lblText->setWordWrap(true);
    lblText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    lblText->setStyleSheet("font-size:12px; color:#1C1C1E;");

    auto *btnJump = new QPushButton(itemWidget);
    btnJump->setToolTip("回溯到这条记录");
    btnJump->setFixedSize(24, 24);
    btnJump->setStyleSheet(
        "QPushButton { background:transparent; border:none; border-radius:5px; }"
        "QPushButton:hover { border:2px solid #C7C7CC; }"
        "QPushButton:pressed { border:2px solid #AEAEB2; }");
    btnJump->setText(QString::fromUtf8("\xe2\x86\xa9"));
    connect(btnJump, &QPushButton::clicked, this, [this, index]() {
        emit jumpToIndex(index);
    });

    auto *btnDelete = new QPushButton(itemWidget);
    btnDelete->setToolTip("删除这条记录");
    btnDelete->setFixedSize(24, 24);
    btnDelete->setStyleSheet(
        "QPushButton { background:transparent; border:none; border-radius:5px; }"
        "QPushButton:hover { border:2px solid #FF3B30; color:#FF3B30; }"
        "QPushButton:pressed { border:2px solid #D63028; }");
    btnDelete->setText(QString::fromUtf8("\xc3\x97"));
    btnDelete->setCursor(Qt::PointingHandCursor);
    connect(btnDelete, &QPushButton::clicked, this, [this, index]() {
        emit deleteIndex(index);
    });

    itemLayout->addWidget(lblRole);
    itemLayout->addWidget(lblText, 1);
    itemLayout->addWidget(btnJump);
    itemLayout->addWidget(btnDelete);

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
