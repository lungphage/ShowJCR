#include "trendchartdialog.h"

#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QLabel>

TrendChartDialog::TrendChartDialog(const QString &journalName,
                                   const QMap<QString, double> &yearData,
                                   QWidget *parent)
    : QDialog(parent), m_journalName(journalName), m_yearData(yearData)
{
    setWindowTitle("IF 趋势图 — " + journalName);
    setMinimumSize(500, 350);
    resize(600, 400);
}

void TrendChartDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if(m_yearData.isEmpty()){
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "无影响因子数据");
        return;
    }

    const int marginLeft = 80, marginTop = 60, marginRight = 40, marginBottom = 80;
    const int chartW = width() - marginLeft - marginRight;
    const int chartH = height() - marginTop - marginBottom;

    if(chartW <= 0 || chartH <= 0) return;

    // 找最大值
    double maxVal = 0;
    for(auto it = m_yearData.begin(); it != m_yearData.end(); ++it){
        if(it.value() > maxVal) maxVal = it.value();
    }
    if(maxVal <= 0) maxVal = 1.0;
    maxVal = qCeil(maxVal);

    // 标题
    painter.setPen(palette().color(QPalette::Text));
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRect(0, 10, width(), 40), Qt::AlignCenter, m_journalName + " — Impact Factor 趋势");

    // 坐标轴
    painter.setPen(QPen(palette().color(QPalette::Text), 1));
    painter.drawLine(marginLeft, marginTop, marginLeft, marginTop + chartH);
    painter.drawLine(marginLeft, marginTop + chartH, marginLeft + chartW, marginTop + chartH);

    // Y 轴刻度
    QFont axisFont = font();
    axisFont.setPointSize(9);
    painter.setFont(axisFont);
    int yTicks = 5;
    for(int i = 0; i <= yTicks; i++){
        int y = marginTop + chartH - (i * chartH / yTicks);
        double val = maxVal * i / yTicks;
        painter.drawLine(marginLeft - 5, y, marginLeft, y);
        painter.drawText(QRect(0, y - 10, marginLeft - 8, 20),
                         Qt::AlignRight | Qt::AlignVCenter, QString::number(val, 'f', 1));
    }

    // 柱状图
    int n = m_yearData.size();
    if(n == 0) return;

    int barAreaW = chartW - 40;
    int barW = qMin(60, barAreaW / n);
    int gap = (barAreaW - barW * n) / (n + 1);
    if(gap < 5) gap = 5;

    QStringList keys = m_yearData.keys();
    for(int i = 0; i < n; i++){
        double val = m_yearData.value(keys[i]);
        int barH = (int)(val / maxVal * chartH);
        int x = marginLeft + 20 + gap + i * (barW + gap);
        int y = marginTop + chartH - barH;

        // 渐变色
        QColor barColor = QColor(70, 130, 180);  // steel blue
        if(i == n - 1) barColor = QColor(220, 80, 60);  // 最新一年红色
        painter.setBrush(barColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(x, y, barW, barH, 3, 3);

        // 数值标签
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(QRect(x - 10, y - 22, barW + 20, 20),
                         Qt::AlignCenter, QString::number(val, 'f', 2));

        // X 轴标签
        painter.save();
        painter.translate(x + barW / 2, marginTop + chartH + 10);
        painter.rotate(30);
        painter.drawText(0, 0, keys[i]);
        painter.restore();
    }

    // Y 轴标题
    painter.save();
    painter.translate(15, marginTop + chartH / 2);
    painter.rotate(-90);
    painter.drawText(0, 0, "Impact Factor");
    painter.restore();
}
