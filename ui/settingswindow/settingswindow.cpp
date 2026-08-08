#include "settingswindow.h"
#include "ui/chatdialog/chatdialog.h"
#include "ui/characterwindow/characterwindow.h"

#include "pages/page_llm.h"
#include "pages/page_character.h"
#include "pages/page_vits.h"
#include "pages/page_stt.h"
#include "pages/page_plugin.h"
#include "pages/page_about.h"

#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

/* ─── 侧边栏代理：绘制圆角高亮和图标 ─── */
class SidebarItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRect rect = option.rect.adjusted(8, 4, -8, -4);

        if (option.state & QStyle::State_Selected)
        {
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, QColor(77, 171, 247));
            gradient.setColorAt(1, QColor(51, 154, 240));
            painter->setBrush(gradient);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, 10, 10);

            painter->setPen(QColor(255, 255, 255));
        }
        else if (option.state & QStyle::State_MouseOver)
        {
            painter->setBrush(QColor(241, 243, 245));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, 10, 10);

            painter->setPen(QColor(33, 37, 41));
        }
        else
        {
            painter->setPen(QColor(73, 80, 87));
        }

        QFont font = painter->font();
        font.setPixelSize(14);
        font.setWeight(QFont::Medium);
        painter->setFont(font);

        QRect textRect = rect.adjusted(20, 0, 0, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(44);
        return s;
    }
};

/* ─── 全局样式表 ─── */
QString SettingsWindow::buildStyleSheet() const
{
    return QStringLiteral(R"(

/* ── 主窗口 ── */
QMainWindow {
    background-color: #F8F9FA;
}

/* ── 侧边栏 ── */
QListWidget#sidebar {
    background-color: #FFFFFF;
    border: none;
    border-right: 1px solid #E9ECEF;
    outline: none;
    padding: 16px 8px;
    font-size: 14px;
    font-weight: 500;
}

QListWidget#sidebar::item {
    padding: 0;
    margin: 2px 4px;
    background: transparent;
    border: none;
    border-radius: 8px;
    min-height: 40px;
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
    border: 1px solid #E9ECEF;
    border-radius: 12px;
    margin-top: 20px;
    padding: 24px 20px 16px 20px;
    font-size: 14px;
    font-weight: 600;
    color: #212529;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
}

QGroupBox::title {
    subcontrol-origin: border;
    subcontrol-position: top left;
    padding: 0 10px;
    background-color: #FFFFFF;
    color: #6C757D;
    font-weight: 600;
    font-size: 12px;
}

/* ── 输入框 ── */
QLineEdit {
    border: 1px solid #DEE2E6;
    border-radius: 10px;
    padding: 10px 14px;
    background-color: #FFFFFF;
    font-size: 14px;
    color: #212529;
    selection-background-color: #4DABF7;
    transition: border-color 0.2s ease, box-shadow 0.2s ease;
}

QLineEdit:hover {
    border-color: #ADB5BD;
}

QLineEdit:focus {
    border: 2px solid #4DABF7;
    padding: 9px 13px;
    box-shadow: 0 0 0 3px rgba(77, 171, 247, 0.15);
}

QLineEdit::placeholder {
    color: #ADB5BD;
}

/* ── 下拉框 ── */
QComboBox {
    border: 1px solid #DEE2E6;
    border-radius: 10px;
    padding: 10px 32px 10px 14px;
    background-color: #FFFFFF;
    font-size: 14px;
    color: #212529;
    min-height: 40px;
    transition: border-color 0.2s ease;
}

QComboBox:hover {
    border-color: #ADB5BD;
}

QComboBox:focus {
    border: 2px solid #4DABF7;
    padding: 9px 31px 9px 13px;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 32px;
    border: none;
    border-left: none;
}

QComboBox::down-arrow {
    width: 14px;
    height: 14px;
}

QComboBox QAbstractItemView {
    border: 1px solid #E9ECEF;
    border-radius: 10px;
    background-color: #FFFFFF;
    selection-background-color: #4DABF7;
    selection-color: #FFFFFF;
    padding: 6px;
    outline: none;
    font-size: 14px;
}

/* ── 按钮 ── */
QPushButton {
    border: 1px solid #DEE2E6;
    border-radius: 4px;
    padding: 3px 10px;
    background-color: #FFFFFF;
    font-size: 12px;
    font-weight: 500;
    color: #495057;
    min-height: 24px;
    transition: all 0.2s ease;
}

QPushButton:hover {
    background-color: #F8F9FA;
    border-color: #ADB5BD;
}

QPushButton:pressed {
    background-color: #E9ECEF;
    transform: scale(0.98);
}

QPushButton#btnFetchModels,
QPushButton#btnFetchSpeakers,
QPushButton#btnRefresh {
    background-color: #4DABF7;
    color: #FFFFFF;
    border: none;
    font-weight: 600;
}

QPushButton#btnFetchModels:hover,
QPushButton#btnFetchSpeakers:hover,
QPushButton#btnRefresh:hover {
    background-color: #339AF0;
}

QPushButton#btnFetchModels:pressed,
QPushButton#btnFetchSpeakers:pressed,
QPushButton#btnRefresh:pressed {
    background-color: #228BE6;
}

QPushButton#btnImport {
    background-color: #51CF66;
    color: #FFFFFF;
    border: none;
    font-weight: 600;
}

QPushButton#btnImport:hover {
    background-color: #40C057;
}

QPushButton#btnImport:pressed {
    background-color: #37B24D;
}

QPushButton#btnDelete {
    background-color: #FF6B6B;
    color: #FFFFFF;
    border: none;
    font-weight: 600;
}

QPushButton#btnDelete:hover {
    background-color: #FF5252;
}

QPushButton#btnDelete:pressed {
    background-color: #E63946;
}

QPushButton#btnResetLoc {
    background-color: #FFFFFF;
    color: #4DABF7;
    border: 1px solid #4DABF7;
    font-weight: 600;
}

QPushButton#btnResetLoc:hover {
    background-color: #E7F5FF;
}

QPushButton#btnAddProvider {
    background-color: #FFFFFF;
    color: #4DABF7;
    border: 1px solid #4DABF7;
    font-weight: 600;
}

QPushButton#btnAddProvider:hover {
    background-color: #E7F5FF;
}

QPushButton#btnRemoveProvider {
    background-color: #FFFFFF;
    color: #FF6B6B;
    border: 1px solid #FF6B6B;
    font-weight: 600;
}

QPushButton#btnRemoveProvider:hover {
    background-color: #FFF5F5;
}

/* ── 复选框 ── */
QCheckBox {
    spacing: 8px;
    font-size: 14px;
    color: #212529;
}

QCheckBox::indicator {
    width: 20px;
    height: 20px;
    border: 2px solid #DEE2E6;
    border-radius: 6px;
    background-color: #FFFFFF;
    transition: all 0.2s ease;
}

QCheckBox::indicator:checked {
    background-color: #4DABF7;
    border-color: #4DABF7;
}

QCheckBox::indicator:hover {
    border-color: #4DABF7;
}

/* ── 数字微调框 ── */
QSpinBox {
    border: 1px solid #DEE2E6;
    border-radius: 10px;
    padding: 8px 12px;
    background-color: #FFFFFF;
    font-size: 14px;
    color: #212529;
    min-height: 40px;
    transition: border-color 0.2s ease;
}

QSpinBox:hover {
    border-color: #ADB5BD;
}

QSpinBox:focus {
    border: 2px solid #4DABF7;
    padding: 7px 11px;
}

QSpinBox::up-button, QSpinBox::down-button {
    border: none;
    width: 24px;
    background: transparent;
}

/* ── 文本编辑器 ── */
QPlainTextEdit {
    border: 1px solid #DEE2E6;
    border-radius: 10px;
    padding: 12px 14px;
    background-color: #FFFFFF;
    font-size: 14px;
    color: #212529;
    selection-background-color: #4DABF7;
    transition: border-color 0.2s ease;
}

QPlainTextEdit:hover {
    border-color: #ADB5BD;
}

QPlainTextEdit:focus {
    border: 2px solid #4DABF7;
    padding: 11px 13px;
    box-shadow: 0 0 0 3px rgba(77, 171, 247, 0.15);
}

/* ── 列表视图 ── */
QListView {
    border: 1px solid #E9ECEF;
    border-radius: 10px;
    background-color: #FAFAFA;
    alternate-background-color: #FFFFFF;
    outline: none;
    font-size: 14px;
    padding: 6px;
}

QListView::item {
    padding: 8px 12px;
    border-radius: 8px;
    color: #212529;
}

QListView::item:selected {
    background-color: #4DABF7;
    color: #FFFFFF;
}

QListView::item:hover:!selected {
    background-color: #F1F3F5;
}

/* ── 列表控件（插件列表、绑定列表、错误列表） ── */
QListWidget {
    border: 1px solid #E9ECEF;
    border-radius: 10px;
    background-color: #FAFAFA;
    outline: none;
    font-size: 14px;
    padding: 6px;
}

QListWidget::item {
    padding: 10px 12px;
    border-radius: 8px;
    color: #212529;
}

QListWidget::item:selected {
    background-color: #4DABF7;
    color: #FFFFFF;
}

QListWidget::item:hover:!selected {
    background-color: #F1F3F5;
}

/* ── 标签 ── */
QLabel {
    color: #212529;
    font-size: 14px;
}

QLabel#labelTitle {
    color: #212529;
    font-size: 28px;
    font-weight: 700;
}

QLabel#labelSubtitle {
    color: #6C757D;
    font-size: 15px;
}

QLabel#labelVersion {
    color: #ADB5BD;
    font-size: 13px;
}

QLabel#labelGithub a {
    color: #4DABF7;
    text-decoration: none;
}

QLabel#labelGithub a:hover {
    text-decoration: underline;
}

/* ── 滚动条 ── */
QScrollBar:vertical {
    width: 8px;
    background: transparent;
    margin: 4px;
}

QScrollBar::handle:vertical {
    background: #CED4DA;
    border-radius: 4px;
    min-height: 40px;
    transition: background 0.2s ease;
}

QScrollBar::handle:vertical:hover {
    background: #ADB5BD;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    height: 0;
    background: transparent;
}

QScrollBar:horizontal {
    height: 8px;
    background: transparent;
    margin: 4px;
}

QScrollBar::handle:horizontal {
    background: #CED4DA;
    border-radius: 4px;
    min-width: 40px;
    transition: background 0.2s ease;
}

QScrollBar::handle:horizontal:hover {
    background: #ADB5BD;
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
    resize(800, 560);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

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
    m_sidebar->setIconSize(QSize(0, 0));

    m_sidebar->addItem("对话模型");
    m_sidebar->addItem("角色设置");
    m_sidebar->addItem("语音合成");
    m_sidebar->addItem("语音识别");
    m_sidebar->addItem("插件管理");
    m_sidebar->addItem("关于");
    m_sidebar->setCurrentRow(0);

    /* ── 右侧内容区 ── */
    m_stack = new QStackedWidget(this);

    auto makeScrollable = [this](QWidget *page) -> QScrollArea * {
        auto *container = new QWidget(this);
        auto *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(24, 20, 24, 24);
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

    /* 应用全局样式表 */
    setStyleSheet(buildStyleSheet());

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

    /* STT 页面信号：配置变更时通知 ChatDialog 刷新 */
    connect(pageStt, &PageStt::configChanged, this,
            [this]() { emit requestReloadAi(); });

    /* LLM 页面信号：服务商列表变更时通知角色页面刷新 */
    connect(pageLLM, &PageLLM::modelListRefreshed, pageChar,
            &PageCharacter::refreshServerList);
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    if (QApplication::closingDown())
        event->accept();
    else
    {
        event->ignore();
        hide();
    }
}
