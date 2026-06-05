#include "batchquerydialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QLabel>

BatchQueryDialog::BatchQueryDialog(SqliteDB *db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle("批量查询");
    setMinimumSize(700, 500);
    resize(800, 600);

    auto *mainLayout = new QVBoxLayout(this);

    auto *inputLabel = new QLabel("输入期刊名称（每行一个）：");
    mainLayout->addWidget(inputLabel);

    m_inputEdit = new QTextEdit();
    m_inputEdit->setPlaceholderText("Nature\nScience\nCell\n...");
    m_inputEdit->setMaximumHeight(120);
    mainLayout->addWidget(m_inputEdit);

    auto *btnLayout = new QHBoxLayout();
    auto *queryBtn = new QPushButton("查询");
    auto *exportBtn = new QPushButton("导出 CSV");
    auto *clearBtn = new QPushButton("清空");
    btnLayout->addWidget(queryBtn);
    btnLayout->addWidget(exportBtn);
    btnLayout->addWidget(clearBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    m_resultTable = new QTableWidget();
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_resultTable);

    connect(queryBtn, &QPushButton::clicked, this, &BatchQueryDialog::onQuery);
    connect(exportBtn, &QPushButton::clicked, this, &BatchQueryDialog::onExport);
    connect(clearBtn, &QPushButton::clicked, m_inputEdit, &QTextEdit::clear);
}

void BatchQueryDialog::onQuery()
{
    QStringList names = m_inputEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    for(QString &name : names)
        name = name.simplified();

    if(names.isEmpty()) return;

    // 收集所有可能的字段名
    QStringList allFields;
    QList<QList<FieldValue>> allResults;

    for(const QString &name : names){
        const QStringList &dbNames = m_db->getAllJournalNames();
        QString matchedName;
        for(const QString &n : dbNames){
            if(n.compare(name, Qt::CaseInsensitive) == 0){
                matchedName = n;
                break;
            }
        }
        if(matchedName.isEmpty()){
            // 模糊匹配
            QStringList fuzzy = m_db->fuzzySearch(name, 1);
            if(!fuzzy.isEmpty()) matchedName = fuzzy.first();
        }

        QList<FieldValue> info;
        if(!matchedName.isEmpty()){
            info = m_db->getJournalInfo(matchedName);
            for(const FieldValue &fv : info){
                if(!allFields.contains(fv.fieldName))
                    allFields.append(fv.fieldName);
            }
        } else {
            info.append(FieldValue{"Journal", name});
            info.append(FieldValue{"状态", "未找到"});
        }
        allResults.append(info);
    }

    // 确保 Journal 字段在第一列
    allFields.removeAll("Journal");
    allFields.prepend("Journal");

    m_resultTable->setRowCount(allResults.size());
    m_resultTable->setColumnCount(allFields.size());
    m_resultTable->setHorizontalHeaderLabels(allFields);

    for(int row = 0; row < allResults.size(); row++){
        const auto &info = allResults[row];
        for(int col = 0; col < allFields.size(); col++){
            for(const FieldValue &fv : info){
                if(fv.fieldName == allFields[col]){
                    m_resultTable->setItem(row, col, new QTableWidgetItem(fv.value));
                    break;
                }
            }
        }
    }

    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
}

void BatchQueryDialog::onExport()
{
    if(m_resultTable->rowCount() == 0){
        QMessageBox::information(this, "提示", "请先查询数据");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV", "batch_query_result.csv", "CSV Files (*.csv)");
    if(filePath.isEmpty()) return;

    exportToCsv(filePath);
    QMessageBox::information(this, "导出成功", "已导出到: " + filePath);
}

void BatchQueryDialog::exportToCsv(const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::warning(this, "错误", "无法写入文件");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";  // UTF-8 BOM for Excel

    // 表头
    QStringList headers;
    for(int col = 0; col < m_resultTable->columnCount(); col++){
        headers << "\"" + m_resultTable->horizontalHeaderItem(col)->text() + "\"";
    }
    out << headers.join(',') << "\n";

    // 数据
    for(int row = 0; row < m_resultTable->rowCount(); row++){
        QStringList values;
        for(int col = 0; col < m_resultTable->columnCount(); col++){
            auto *item = m_resultTable->item(row, col);
            QString val = item ? item->text() : "";
            val.replace("\"", "\"\"");
            values << "\"" + val + "\"";
        }
        out << values.join(',') << "\n";
    }

    file.close();
}
