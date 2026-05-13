#pragma once

#include <QJsonArray>
#include <QWidget>

class AiProvider;
class QStringListModel;

namespace Ui { class PageLLM; }

/* LLM 对话模型配置页面：服务商管理（增删改）、API Key、模型列表 */
class PageLLM : public QWidget
{
    Q_OBJECT

public:
    explicit PageLLM(QWidget *parent = nullptr);
    ~PageLLM();

    /* 从配置文件中查找指定服务商的 baseUrl 和 apiKey（供外部使用） */
    static bool findProvider(const QString &name, QString &baseUrl, QString &apiKey);

signals:
    void modelListRefreshed();

private slots:
    void onAddProvider();
    void onRemoveProvider();
    void onProviderSelected(int row);
    void onFieldChanged();
    void onFetchModels();

private:
    void loadConfig();
    void saveProviders();
    void refreshProviderList();
    void loadProviderDetail(int index);
    void clearDetail();

    Ui::PageLLM *ui;
    AiProvider *m_ai;
    QStringListModel *m_modelListModel;
    QJsonArray m_providers;
    int m_currentIndex = -1;
    bool m_loading = false;
};
