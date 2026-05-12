#include "page_llm.h"
#include "ui_page_llm.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"
#include "core/AiProvider.h"

#include <QJsonArray>
#include <QStringListModel>

PageLLM::PageLLM(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageLLM)
{
    ui->setupUi(this);

    m_ai = new AiProvider(this);
    m_modelListModel = new QStringListModel(this);
    ui->listModels->setModel(m_modelListModel);

    loadConfig();

    /* API Key 修改时自动保存 */
    connect(ui->editApiKey, &QLineEdit::textChanged, this, &PageLLM::onApiKeyChanged);
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
                JsonConfig cfg(GlobalConfigPath);
                cfg.setValue("llm/" + m_currentServer + "/ModelList", ids);
                m_modelListModel->setStringList(names);
                emit modelListRefreshed();
            });
    connect(m_ai, &AiProvider::errorOccurred, this,
            [this](const QString &err) {
                ui->labelStatus->setText("错误：" + err);
            });
}

PageLLM::~PageLLM() { delete ui; }

/* 读取已保存的 API Key 和模型列表 */
void PageLLM::loadConfig()
{
    JsonConfig cfg(GlobalConfigPath);
    m_currentServer = ui->comboServer->currentText();

    QString key = cfg.value("llm/" + m_currentServer + "/ApiKey").toString();
    ui->editApiKey->setText(key);

    QJsonArray arr = cfg.value("llm/" + m_currentServer + "/ModelList").toArray();
    QStringList list;
    for (const QJsonValue &v : arr)
        list << v.toString();
    m_modelListModel->setStringList(list);
    ui->labelStatus->setText(list.isEmpty() ? "未配置" : "已配置");
}

void PageLLM::onApiKeyChanged(const QString &text)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("llm/" + m_currentServer + "/ApiKey", text);
    ui->labelStatus->setText("已保存");
}

/* 点击获取按钮：设置 API 类型并拉取模型列表 */
void PageLLM::onFetchModels()
{
    m_currentServer = ui->comboServer->currentText();
    if (m_currentServer == "OpenAI")
        m_ai->setServiceType(AiProvider::OpenAI);
    else if (m_currentServer == "DeepSeek")
        m_ai->setServiceType(AiProvider::DeepSeek);
    else
        m_ai->setServiceType(AiProvider::Custom);

    m_ai->setApiKey(ui->editApiKey->text());
    m_ai->fetchModels();
    ui->labelStatus->setText("正在获取...");
}
