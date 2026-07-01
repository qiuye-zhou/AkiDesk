#include "page_llm.h"
#include "ui_page_llm.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"
#include "core/AiProvider.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QStringListModel>

PageLLM::PageLLM(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageLLM)
{
    ui->setupUi(this);

    m_ai = new AiProvider(this);
    m_modelListModel = new QStringListModel(this);
    ui->listModels->setModel(m_modelListModel);

    connect(ui->btnAddProvider, &QPushButton::clicked, this, &PageLLM::onAddProvider);
    connect(ui->btnRemoveProvider, &QPushButton::clicked, this, &PageLLM::onRemoveProvider);
    connect(ui->listProviders, &QListWidget::currentRowChanged, this, &PageLLM::onProviderSelected);

    /* 详情字段修改时自动保存 */
    connect(ui->editProviderName, &QLineEdit::textChanged, this, &PageLLM::onFieldChanged);
    connect(ui->editBaseUrl, &QLineEdit::textChanged, this, &PageLLM::onFieldChanged);
    connect(ui->editApiKey, &QLineEdit::textChanged, this, &PageLLM::onFieldChanged);

    connect(ui->btnFetchModels, &QPushButton::clicked, this, &PageLLM::onFetchModels);

    /* 模型列表返回后保存并更新 UI */
    connect(m_ai, &AiProvider::modelsReceived, this,
            [this](const QList<AiProvider::ModelInfo> &models) {
                QStringList names;
                QJsonArray ids;
                for (const auto &m : models)
                {
                    names << m.id + (m.ownedBy.isEmpty() ? "" : " (" + m.ownedBy + ")");
                    ids.append(m.id);
                }

                /* 保存到当前选中服务商 */
                if (m_currentIndex >= 0 && m_currentIndex < m_providers.size())
                {
                    QJsonObject prov = m_providers[m_currentIndex].toObject();
                    prov["modelList"] = ids;
                    m_providers[m_currentIndex] = prov;
                    saveProviders();
                }
                m_modelListModel->setStringList(names);
                ui->labelStatus->setText("已获取 " + QString::number(models.size()) + " 个模型");
                emit modelListRefreshed();
            });
    connect(m_ai, &AiProvider::errorOccurred, this,
            [this](const QString &err) {
                ui->labelStatus->setText("错误：" + err);
            });

    loadConfig();
}

PageLLM::~PageLLM()
{
    if (m_ai)
        m_ai->cancelAll();
    delete ui;
}

/* 查找指定名称的服务商，返回 baseUrl 和 apiKey */
bool PageLLM::findProvider(const QString &name, QString &baseUrl, QString &apiKey)
{
    JsonConfig cfg(GlobalConfigPath);
    QJsonArray providers = cfg.value("llm/providers").toArray();
    for (const QJsonValue &v : providers)
    {
        QJsonObject prov = v.toObject();
        if (prov["name"].toString() == name)
        {
            baseUrl = prov["baseUrl"].toString();
            apiKey = prov["apiKey"].toString();
            return true;
        }
    }
    return false;
}

/* 加载配置：填充服务商列表 */
void PageLLM::loadConfig()
{
    JsonConfig cfg(GlobalConfigPath);
    m_providers = cfg.value("llm/providers").toArray();

    /* 首次运行时创建默认服务商 */
    if (m_providers.isEmpty())
    {
        QJsonObject def;
        def["name"] = "DeepSeek";
        def["baseUrl"] = "https://api.deepseek.com/v1";
        def["apiKey"] = "";
        def["modelList"] = QJsonArray();
        m_providers.append(def);
        saveProviders();
    }

    refreshProviderList();

    /* 选中上次使用的服务商 */
    QString selected = cfg.value("llm/selectedProvider").toString();
    if (!selected.isEmpty())
    {
        for (int i = 0; i < ui->listProviders->count(); ++i)
        {
            if (ui->listProviders->item(i)->text() == selected)
            {
                ui->listProviders->setCurrentRow(i);
                return;
            }
        }
    }
    if (ui->listProviders->count() > 0)
        ui->listProviders->setCurrentRow(0);
    else
        clearDetail();
}

void PageLLM::saveProviders()
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("llm/providers", m_providers);
}

void PageLLM::refreshProviderList()
{
    ui->listProviders->clear();
    for (const QJsonValue &v : m_providers)
    {
        QJsonObject prov = v.toObject();
        ui->listProviders->addItem(prov["name"].toString());
    }
}

void PageLLM::loadProviderDetail(int index)
{
    m_loading = false; /* 暂停字段变更触发保存 */

    if (index < 0 || index >= m_providers.size())
    {
        clearDetail();
        return;
    }

    m_currentIndex = index;
    QJsonObject prov = m_providers[index].toObject();

    ui->editProviderName->setText(prov["name"].toString());
    ui->editBaseUrl->setText(prov["baseUrl"].toString());
    ui->editApiKey->setText(prov["apiKey"].toString());

    /* 加载模型列表 */
    QJsonArray arr = prov["modelList"].toArray();
    QStringList list;
    for (const QJsonValue &v : arr)
        list << v.toString();
    m_modelListModel->setStringList(list);

    ui->labelStatus->setText(list.isEmpty() ? "未获取模型" : "已配置 " + QString::number(list.size()) + " 个模型");

    /* 记录当前选中 */
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("llm/selectedProvider", prov["name"].toString());

    m_loading = true;
}

void PageLLM::clearDetail()
{
    m_loading = false;
    m_currentIndex = -1;
    ui->editProviderName->clear();
    ui->editBaseUrl->clear();
    ui->editApiKey->clear();
    m_modelListModel->setStringList({});
    ui->labelStatus->setText("请选择或添加一个服务商");
}

void PageLLM::onAddProvider()
{
    QJsonObject prov;
    int count = m_providers.size();
    prov["name"] = "新服务商 " + QString::number(count + 1);
    prov["baseUrl"] = "";
    prov["apiKey"] = "";
    prov["modelList"] = QJsonArray();
    m_providers.append(prov);

    saveProviders();
    refreshProviderList();
    ui->listProviders->setCurrentRow(ui->listProviders->count() - 1);
    emit modelListRefreshed();
}

void PageLLM::onRemoveProvider()
{
    int row = ui->listProviders->currentRow();
    if (row < 0 || row >= m_providers.size())
        return;

    QString name = m_providers[row].toObject()["name"].toString();
    auto btn = QMessageBox::question(this, "删除服务商",
        QString("确定要删除服务商 \"%1\" 吗？").arg(name));
    if (btn != QMessageBox::Yes)
        return;

    m_providers.removeAt(row);
    saveProviders();
    refreshProviderList();

    if (ui->listProviders->count() > 0)
        ui->listProviders->setCurrentRow(qMin(row, ui->listProviders->count() - 1));
    else
        clearDetail();

    emit modelListRefreshed();
}

void PageLLM::onProviderSelected(int row)
{
    loadProviderDetail(row);
}

/* 详情字段变更时保存到 m_providers 数组 */
void PageLLM::onFieldChanged()
{
    if (!m_loading || m_currentIndex < 0 || m_currentIndex >= m_providers.size())
        return;

    QJsonObject prov = m_providers[m_currentIndex].toObject();
    prov["name"] = ui->editProviderName->text();
    prov["baseUrl"] = ui->editBaseUrl->text();
    prov["apiKey"] = ui->editApiKey->text();
    m_providers[m_currentIndex] = prov;

    /* 更新列表显示名称 */
    if (ui->listProviders->item(m_currentIndex))
        ui->listProviders->item(m_currentIndex)->setText(prov["name"].toString());

    saveProviders();
    ui->labelStatus->setText("已保存");
}

/* 点击获取按钮：用当前 baseUrl + apiKey 拉取模型列表 */
void PageLLM::onFetchModels()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_providers.size())
        return;

    QString baseUrl = ui->editBaseUrl->text().trimmed();
    QString apiKey = ui->editApiKey->text();

    if (baseUrl.isEmpty())
    {
        ui->labelStatus->setText("请先填写请求地址");
        return;
    }

    /* 移除末尾的斜杠 */
    if (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    m_ai->setApiUrl(baseUrl);
    m_ai->setApiKey(apiKey);
    m_ai->fetchModels();
    ui->labelStatus->setText("正在获取...");
}
