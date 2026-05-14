#include "settingswindow.h"
#include "ui/chatdialog/chatdialog.h"
#include "ui/characterwindow/characterwindow.h"

#include "pages/page_llm.h"
#include "pages/page_character.h"
#include "pages/page_vits.h"
#include "pages/page_stt.h"
#include "pages/page_plugin.h"
#include "pages/page_about.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

/* ─── 圆角遮罩代理：为侧边栏选中项绘制圆角高亮 ─── */
class SidebarItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRect rect = option.rect.adjusted(6, 2, -6, -2);

        if (option.state & QStyle::State_Selected)
        {
            painter->setBrush(QColor(0xE0, 0xE0, 0xE2));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, 8, 8);
        }
        else if (option.state & QStyle::State_MouseOver)
        {
            painter->setBrush(QColor(0xF0, 0xF0, 0xF2));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, 8, 8);
        }

        painter->setPen(QColor(0x1C, 0x1C, 0x1E));
        QFont font = painter->font();
        font.setPixelSize(13);
        painter->setFont(font);

        QRect textRect = rect.adjusted(30, 0, 0, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(34);
        return s;
    }
};

/* ─── 全局样式表 ─── */
QString SettingsWindow::buildStyleSheet() const
{
    return QStringLiteral(R"(

/* ── 主窗口 ── */
QMainWindow {
    background-color: #FFFFFF;
}

/* ── 侧边栏 ── */
QListWidget#sidebar {
    background-color: #F5F5F7;
    border: none;
    border-right: 1px solid #E5E5EA;
    outline: none;
    padding: 8px 4px;
    font-size: 13px;
}

QListWidget#sidebar::item {
    padding: 0;
    margin: 0 0 1px 0;
    background: transparent;
    border: none;
    border-radius: 0;
    min-height: 34px;
}

QListWidget#sidebar::item:selected {
    background: transparent;
    color: #1C1C1E;
}

QListWidget#sidebar::item:hover {
    background: transparent;
}

QListWidget#sidebar::item:selected:hover {
    background: transparent;
}

/* ── 内容区滚动区域 ── */
QScrollArea {
    background: transparent;
    border: none;
}
QScrollArea > QWidget > QWidget {
    background: transparent;
}

/* ── 分组卡片（GroupBox → 卡片风格） ── */
QGroupBox {
    background-color: #FFFFFF;
    border: 1px solid #E5E5EA;
    border-radius: 10px;
    margin-top: 20px;
    padding: 20px 16px 12px 16px;
    font-size: 13px;
    font-weight: bold;
    color: #1C1C1E;
}

QGroupBox::title {
    subcontrol-origin: border;
    subcontrol-position: top left;
    padding: 0 8px;
    background-color: #FFFFFF;
    color: #8E8E93;
    font-weight: 600;
    font-size: 12px;
}

/* ── 输入框 ── */
QLineEdit {
    border: 1px solid #D1D1D6;
    border-radius: 8px;
    padding: 6px 10px;
    background-color: #FFFFFF;
    font-size: 13px;
    color: #1C1C1E;
    selection-background-color: #007AFF;
}

QLineEdit:focus {
    border: 2px solid #007AFF;
    padding: 5px 9px;
}

QLineEdit::placeholder {
    color: #AEAEB2;
}

/* ── 下拉框 ── */
QComboBox {
    border: 1px solid #D1D1D6;
    border-radius: 8px;
    padding: 6px 28px 6px 10px;
    background-color: #FFFFFF;
    font-size: 13px;
    color: #1C1C1E;
    min-height: 24px;
}

QComboBox:hover {
    border-color: #AEAEB2;
}

QComboBox:focus {
    border: 2px solid #007AFF;
    padding: 5px 27px 5px 9px;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 24px;
    border: none;
    border-left: none;
}

QComboBox::down-arrow {
    width: 10px;
    height: 10px;
}

QComboBox QAbstractItemView {
    border: 1px solid #E5E5EA;
    border-radius: 8px;
    background-color: #FFFFFF;
    selection-background-color: #007AFF;
    selection-color: #FFFFFF;
    padding: 4px;
    outline: none;
}

/* ── 按钮 ── */
QPushButton {
    border: 1px solid #D1D1D6;
    border-radius: 8px;
    padding: 6px 16px;
    background-color: #F2F2F7;
    font-size: 13px;
    color: #1C1C1E;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #E5E5EA;
}

QPushButton:pressed {
    background-color: #D1D1D6;
}

QPushButton#btnFetchModels,
QPushButton#btnFetchSpeakers {
    background-color: #007AFF;
    color: #FFFFFF;
    border: none;
    font-weight: 500;
}

QPushButton#btnFetchModels:hover,
QPushButton#btnFetchSpeakers:hover {
    background-color: #0066D6;
}

QPushButton#btnFetchModels:pressed,
QPushButton#btnFetchSpeakers:pressed {
    background-color: #0055B3;
}

QPushButton#btnImport {
    background-color: #34C759;
    color: #FFFFFF;
    border: none;
    font-weight: 500;
}

QPushButton#btnImport:hover {
    background-color: #2DB34D;
}

QPushButton#btnDelete {
    background-color: #FF3B30;
    color: #FFFFFF;
    border: none;
    font-weight: 500;
}

QPushButton#btnDelete:hover {
    background-color: #E6352B;
}

QPushButton#btnRefresh {
    background-color: #007AFF;
    color: #FFFFFF;
    border: none;
    font-weight: 500;
}

QPushButton#btnRefresh:hover {
    background-color: #0066D6;
}

QPushButton#btnResetLoc {
    background-color: #F2F2F7;
    color: #007AFF;
    border: 1px solid #007AFF;
    font-weight: 500;
}

QPushButton#btnResetLoc:hover {
    background-color: #E8F0FE;
}

/* ── 复选框 ── */
QCheckBox {
    spacing: 6px;
    font-size: 13px;
    color: #1C1C1E;
}

QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #C7C7CC;
    border-radius: 5px;
    background-color: #FFFFFF;
}

QCheckBox::indicator:checked {
    background-color: #007AFF;
    border-color: #007AFF;
}

QCheckBox::indicator:hover {
    border-color: #007AFF;
}

/* ── 数字微调框 ── */
QSpinBox {
    border: 1px solid #D1D1D6;
    border-radius: 8px;
    padding: 4px 8px;
    background-color: #FFFFFF;
    font-size: 13px;
    color: #1C1C1E;
    min-height: 24px;
}

QSpinBox:focus {
    border: 2px solid #007AFF;
    padding: 3px 7px;
}

QSpinBox::up-button, QSpinBox::down-button {
    border: none;
    width: 18px;
    background: transparent;
}

/* ── 文本编辑器 ── */
QPlainTextEdit {
    border: 1px solid #D1D1D6;
    border-radius: 8px;
    padding: 8px 10px;
    background-color: #FFFFFF;
    font-size: 13px;
    color: #1C1C1E;
    selection-background-color: #007AFF;
}

QPlainTextEdit:focus {
    border: 2px solid #007AFF;
    padding: 7px 9px;
}

/* ── 列表视图 ── */
QListView {
    border: 1px solid #E5E5EA;
    border-radius: 8px;
    background-color: #FAFAFA;
    alternate-background-color: #FFFFFF;
    outline: none;
    font-size: 13px;
    padding: 4px;
}

QListView::item {
    padding: 4px 8px;
    border-radius: 6px;
    color: #1C1C1E;
}

QListView::item:selected {
    background-color: #007AFF;
    color: #FFFFFF;
}

QListView::item:hover:!selected {
    background-color: #F2F2F7;
}

/* ── 列表控件（插件列表、绑定列表、错误列表） ── */
QListWidget {
    border: 1px solid #E5E5EA;
    border-radius: 8px;
    background-color: #FAFAFA;
    outline: none;
    font-size: 13px;
    padding: 4px;
}

QListWidget::item {
    padding: 6px 8px;
    border-radius: 6px;
    color: #1C1C1E;
}

QListWidget::item:selected {
    background-color: #007AFF;
    color: #FFFFFF;
}

QListWidget::item:hover:!selected {
    background-color: #F2F2F7;
}

/* ── 标签 ── */
QLabel {
    color: #1C1C1E;
    font-size: 13px;
}

QLabel#labelTitle {
    color: #1C1C1E;
    font-size: 24px;
    font-weight: bold;
}

QLabel#labelSubtitle {
    color: #8E8E93;
    font-size: 14px;
}

QLabel#labelVersion {
    color: #8E8E93;
    font-size: 12px;
}

QLabel#labelGithub a {
    color: #007AFF;
    text-decoration: none;
}

QLabel#labelGithub a:hover {
    text-decoration: underline;
}

/* ── 滚动条 ── */
QScrollBar:vertical {
    width: 6px;
    background: transparent;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: #C7C7CC;
    border-radius: 3px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background: #AEAEB2;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    height: 0;
    background: transparent;
}

QScrollBar:horizontal {
    height: 6px;
    background: transparent;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background: #C7C7CC;
    border-radius: 3px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background: #AEAEB2;
}

QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal {
    width: 0;
    background: transparent;
}

)");
}

SettingsWindow::SettingsWindow(ChatDialog *chat, CharacterWindow *tachie,
                               QWidget *parent)
    : QMainWindow(parent), m_chat(chat), m_tachie(tachie)
{
    setupUi();
}

void SettingsWindow::setupUi()
{
    setWindowTitle("设置");
    resize(720, 520);
    setAttribute(Qt::WA_TranslucentBackground, false);

    Qt::WindowFlags flags = Qt::Dialog | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    /* 应用全局样式表 */
    setStyleSheet(buildStyleSheet());

    /* ── 主布局 ── */
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    /* ── 左侧边栏 ── */
    m_sidebar = new QListWidget(this);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(200);
    m_sidebar->setItemDelegate(new SidebarItemDelegate(m_sidebar));
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setIconSize(QSize(0, 0)); /* 不使用图标，纯文字 */

    m_sidebar->addItem("对话模型");
    m_sidebar->addItem("角色设置");
    m_sidebar->addItem("语音合成");
    m_sidebar->addItem("语音识别");
    m_sidebar->addItem("插件管理");
    m_sidebar->addItem("关于");
    m_sidebar->setCurrentRow(0);

    /* ── 右侧内容区 ── */
    m_stack = new QStackedWidget(this);

    /* 各个子页面，外层包裹 QScrollArea 以支持内容溢出滚动 */
    auto makeScrollable = [this](QWidget *page) -> QScrollArea * {
        auto *container = new QWidget(this);
        auto *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(20, 16, 20, 20);
        containerLayout->addWidget(page);

        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidget(container);
        return scroll;
    };

    auto *pageLLM = new PageLLM(this);
    auto *pageChar = new PageCharacter(this);
    auto *pageVits = new PageVits(this);
    auto *pageStt = new PageStt(this);
    auto *pagePlugin = new PagePlugin(this);
    auto *pageAbout = new PageAbout(this);

    /* 确保页面在滚动区域内正确展开 */
    for (QWidget *page : {static_cast<QWidget *>(pageLLM), static_cast<QWidget *>(pageChar), static_cast<QWidget *>(pageVits), static_cast<QWidget *>(pageStt), static_cast<QWidget *>(pagePlugin), static_cast<QWidget *>(pageAbout)})
        page->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    m_stack->addWidget(makeScrollable(pageLLM));
    m_stack->addWidget(makeScrollable(pageChar));
    m_stack->addWidget(makeScrollable(pageVits));
    m_stack->addWidget(makeScrollable(pageStt));
    m_stack->addWidget(makeScrollable(pagePlugin));
    m_stack->addWidget(makeScrollable(pageAbout));

    /* ── 组装布局 ── */
    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_stack, 1);
    setCentralWidget(centralWidget);

    /* ── 侧边栏切换 ── */
    connect(m_sidebar, &QListWidget::currentRowChanged,
            m_stack, &QStackedWidget::setCurrentIndex);

    /* ── 角色页面信号连接 ── */
    connect(pageChar, &PageCharacter::requestReloadCharSelect, this,
            [this](const QString &name) { emit requestReloadCharImage(name); });
    connect(pageChar, &PageCharacter::requestReloadAi, this,
            [this]() { emit requestReloadAi(); });
    connect(pageChar, &PageCharacter::requestSetTachieSize, this,
            [this](int s) { emit requestSetTachieSize(s); });
    connect(pageChar, &PageCharacter::requestResetTachieLoc, this,
            [this]() { emit requestResetTachieLoc(); });

    /* VITS 页面信号 */
    connect(pageVits, &PageVits::vitsModelListRefreshed, pageChar,
            &PageCharacter::refreshVitsModelList);

    /* LLM 页面信号：服务商列表变更时通知角色页面刷新 */
    connect(pageLLM, &PageLLM::modelListRefreshed, pageChar,
            &PageCharacter::refreshServerList);
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}
