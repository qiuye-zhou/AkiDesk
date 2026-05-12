#pragma once

#include <QWidget>

class AiProvider;
class QStringListModel;

namespace Ui { class PageLLM; }

/* LLM 对话模型配置页面：API Key 管理、模型列表获取 */
class PageLLM : public QWidget
{
    Q_OBJECT

public:
    explicit PageLLM(QWidget *parent = nullptr);
    ~PageLLM();

signals:
    void modelListRefreshed();

private slots:
    void onApiKeyChanged(const QString &text);
    void onFetchModels();

private:
    void loadConfig();
    Ui::PageLLM *ui;
    AiProvider *m_ai;
    QStringListModel *m_modelListModel;
    QString m_currentServer;
};
