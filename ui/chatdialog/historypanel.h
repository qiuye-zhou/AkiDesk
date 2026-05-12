#pragma once

#include <QWidget>

namespace Ui { class HistoryPanel; }

class HistoryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPanel(QWidget *parent = nullptr);
    ~HistoryPanel();

    void clear();
    void addItem(int index, const QString &role, const QString &text);

signals:
    void jumpToIndex(int index);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::HistoryPanel *ui;
};
