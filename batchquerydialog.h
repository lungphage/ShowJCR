#ifndef BATCHQUERYDIALOG_H
#define BATCHQUERYDIALOG_H

#include <QDialog>
#include "sqlitedb.h"

class QTextEdit;
class QTableWidget;

class BatchQueryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BatchQueryDialog(SqliteDB *db, QWidget *parent = nullptr);

private slots:
    void onQuery();
    void onExport();

private:
    SqliteDB *m_db;
    QTextEdit *m_inputEdit;
    QTableWidget *m_resultTable;
    void exportToCsv(const QString &filePath);
};

#endif // BATCHQUERYDIALOG_H
