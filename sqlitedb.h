#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QObject>
#include <QDir>
#include <QHash>
#include <QMap>
#include <QtSql/QSqlDatabase>

struct FieldValue {
    QString fieldName;
    QString value;
};

struct TableKey {
    QString tableName;
    QString keyField;
};

class SqliteDB : public QObject
{
    Q_OBJECT
public:
    explicit SqliteDB(const QDir &appDir, const QString &datasetName, QObject *parent = nullptr);
    ~SqliteDB();

    QStringList getAllTableNames() const;
    const QStringList& getAllJournalNames() const;
    QList<FieldValue> getJournalInfo(const QString &journalName, bool allowSelectAgain = true);
    void selectTableNames(const QStringList &tableNames);
    QStringList fuzzySearch(const QString &keyword, int maxResults = 50) const;

private:
    static constexpr const char* kDefaultPrimaryKey = "Journal";

    QSqlDatabase database;
    QStringList allTableNames;
    QStringList tableNames;
    QList<QStringList> tableFields;
    QMap<QString, QStringList> tablePrimaryKeys;
    QStringList allJournalNamesList;

    QHash<QString, QList<int>> journalNameToKeyIndices;
    QList<TableKey> keyEntries;

    void selectTableFields();
    void buildPrimaryKeyMap();
    void selectAllJournalNames();
    QStringList sortSpecialStrings(const QStringList &input);
};

#endif // SQLITEDB_H
