#ifndef TRENDCHARTDIALOG_H
#define TRENDCHARTDIALOG_H

#include <QDialog>
#include <QMap>

class TrendChartDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TrendChartDialog(const QString &journalName,
                              const QMap<QString, double> &yearData,
                              QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_journalName;
    QMap<QString, double> m_yearData;
};

#endif // TRENDCHARTDIALOG_H
