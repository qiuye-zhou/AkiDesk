#include "page_vits.h"
#include "ui_page_vits.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringListModel>

PageVits::PageVits(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageVits), m_network(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    m_speakerListModel = new QStringListModel(this);
    ui->listSpeakers->setModel(m_speakerListModel);

    JsonConfig cfg(GlobalConfigPath);
    ui->editApiUrl->setText(cfg.value("vits/ApiUrl").toString());
    ui->checkSentenceSplit->setChecked(cfg.value("vits/SentenceSplit", true).toBool());

    /* 加载已缓存的模型说话人列表 */
    QJsonArray arr = cfg.value("vits/ModelAndSpeakerList").toArray();
    QStringList list;
    for (const QJsonValue &v : arr)
        list << v.toString();
    m_speakerListModel->setStringList(list);

    connect(ui->editApiUrl, &QLineEdit::textChanged, this, &PageVits::onApiUrlChanged);
    connect(ui->btnFetchSpeakers, &QPushButton::clicked, this, &PageVits::onFetchSpeakers);
    connect(ui->checkSentenceSplit, &QCheckBox::toggled, this, &PageVits::onSentenceSplitToggled);
}

PageVits::~PageVits() { delete ui; }

void PageVits::onApiUrlChanged(const QString &text)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("vits/ApiUrl", text);
}

void PageVits::onSentenceSplitToggled(bool checked)
{
    JsonConfig cfg(GlobalConfigPath);
    cfg.setValue("vits/SentenceSplit", checked);
}

/* 向 vits-simple-api 的 /voice/speakers 接口请求模型说话人列表 */
void PageVits::onFetchSpeakers()
{
    QUrl url(ui->editApiUrl->text() + "/voice/speakers");
    QNetworkReply *reply = m_network->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError)
        {
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject())
        {
            reply->deleteLater();
            return;
        }

        QStringList list;
        QJsonObject root = doc.object();
        for (auto it = root.begin(); it != root.end(); ++it)
        {
            QString modelType = it.key();
            QJsonArray speakers = it.value().toArray();
            for (const QJsonValue &sv : speakers)
            {
                QJsonObject obj = sv.toObject();
                list << modelType + " - " +
                        QString::number(obj.value("id").toInt()) + " - " +
                        obj.value("name").toString();
            }
        }

        m_speakerListModel->setStringList(list);

        JsonConfig cfg(GlobalConfigPath);
        QJsonArray arr;
        for (const QString &s : list)
            arr.append(s);
        cfg.setValue("vits/ModelAndSpeakerList", arr);

        emit vitsModelListRefreshed();
        reply->deleteLater();
    });
}
