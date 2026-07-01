#pragma once

#include <QWidget>

class QNetworkAccessManager;
class QNetworkReply;
class QStringListModel;

namespace Ui { class PageVits; }

/* 语音合成配置页面：API 地址、模型说话人列表获取 */
class PageVits : public QWidget
{
    Q_OBJECT

public:
    explicit PageVits(QWidget *parent = nullptr);
    ~PageVits();

signals:
    void vitsModelListRefreshed();

private slots:
    void onApiUrlChanged(const QString &text);
    void onFetchSpeakers();
    void onSentenceSplitToggled(bool checked);

private:
    Ui::PageVits *ui;
    QNetworkAccessManager *m_network;
    QStringListModel *m_speakerListModel;
    QNetworkReply *m_currentReply = nullptr;
};
