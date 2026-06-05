#include "sqlitedb.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QMessageBox>
#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

SqliteDB::SqliteDB(const QDir &appDir, const QString &datasetName, QObject *parent) : QObject(parent)
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(appDir.absoluteFilePath(datasetName));
    database.setConnectOptions("QSQLITE_OPEN_READONLY");

    if (!database.open())
    {
        qWarning() << "Error: Failed to connect database." << __FUNCTION__ << database.lastError();
        QMessageBox::warning(QApplication::activeWindow(), "期刊信息数据库缺失！", database.lastError().text());
    }
    else
    {
        qDebug() << "Successed to connect database.";
    }

    allTableNames = database.tables();
    allTableNames = sortSpecialStrings(allTableNames);
}

SqliteDB::~SqliteDB()
{
    if(database.isOpen()){
        database.close();
    }
}

QStringList SqliteDB::getAllTableNames() const
{
    return allTableNames;
}

const QStringList& SqliteDB::getAllJournalNames() const
{
    return allJournalNamesList;
}

QList<FieldValue> SqliteDB::getJournalInfo(const QString &journalName, bool allowSelectAgain)
{
    Q_ASSERT(allJournalNamesList.contains(journalName, Qt::CaseInsensitive));

    QList<FieldValue> journalInfo;
    QStringList journalInfoFieldNames;
    QSqlQuery query;

    const QString lowerName = journalName.toLower();
    const QList<int> indices = journalNameToKeyIndices.value(lowerName);

    for(int idx : indices){
        const QString &table = keyEntries[idx].tableName;
        const QString &primaryKey = keyEntries[idx].keyField;

        if(database.isOpen()){
            query.prepare("SELECT * FROM " + table + " WHERE " + primaryKey + " = ? COLLATE NOCASE");
            query.addBindValue(journalName);

            if (!query.exec()){
                qWarning() << "Error: Failed to select" << table << __FUNCTION__ << database.lastError();
                continue;
            }

            int tableIdx = tableNames.indexOf(table);
            if(tableIdx < 0) continue;

            while (query.next()){
                const QStringList &fieldNames = tableFields[tableIdx];
                for(const QString &fieldName : fieldNames){
                    QString value = query.value(fieldName).toString();
                    if(value.isEmpty())
                        continue;
                    if(!journalInfoFieldNames.contains(fieldName) || fieldName != kDefaultPrimaryKey){
                        journalInfo.append({fieldName, value});
                        journalInfoFieldNames << fieldName;
                    }
                }
            }
        }
    }

    if(allowSelectAgain && !journalInfo.isEmpty() && journalInfo[0].fieldName != kDefaultPrimaryKey){
        for(const FieldValue &info : journalInfo){
            if(info.fieldName == kDefaultPrimaryKey){
                journalInfo = getJournalInfo(info.value, false);
                qInfo() << "auto select" << info.value;
                break;
            }
        }
    }
    return journalInfo;
}

QStringList SqliteDB::fuzzySearch(const QString &keyword, int maxResults) const
{
    if(keyword.isEmpty()) return {};

    const QString lower = keyword.toLower();
    QStringList exact, prefix, substring;

    for(const QString &name : allJournalNamesList){
        const QString lowerName = name.toLower();
        if(lowerName == lower){
            exact << name;
        } else if(lowerName.startsWith(lower)){
            prefix << name;
        } else if(lowerName.contains(lower)){
            substring << name;
        }
        if(exact.size() + prefix.size() + substring.size() >= maxResults)
            break;
    }

    QStringList result;
    result.reserve(qMin(maxResults, exact.size() + prefix.size() + substring.size()));
    result.append(exact);
    result.append(prefix);
    result.append(substring);
    if(result.size() > maxResults)
        result = result.mid(0, maxResults);
    return result;
}

void SqliteDB::selectTableNames(const QStringList &selectedTableNames)
{
    tableNames = selectedTableNames;
    selectTableFields();
    buildPrimaryKeyMap();
    selectAllJournalNames();
}

void SqliteDB::selectTableFields()
{
    tableFields.clear();
    QSqlQuery query;
    for(const QString &table : std::as_const(tableNames)){
        QStringList fieldNames;
        if(database.isOpen()){
            query.prepare("PRAGMA table_info(" + table + ")");
            if (!query.exec()){
                qWarning() << "Error: Failed to selectTableFields." << table << __FUNCTION__ << database.lastError();
            }
            while (query.next()){
                fieldNames << query.value(1).toString();
            }
        }
        tableFields << fieldNames;
    }

    Q_ASSERT(tableNames.size() == tableFields.size());
}

void SqliteDB::buildPrimaryKeyMap()
{
    tablePrimaryKeys.clear();
    keyEntries.clear();
    Q_ASSERT(tableNames.size() == tableFields.size());

    for(int i = 0; i < tableNames.size(); i++){
        const QString &table = tableNames[i];
        const QStringList &fields = tableFields[i];

        Q_ASSERT(fields.contains(kDefaultPrimaryKey));

        QStringList keys;
        if(fields.contains(kDefaultPrimaryKey)){
            keys << kDefaultPrimaryKey;
            keyEntries.append({table, kDefaultPrimaryKey});
        }
        if(!fields.isEmpty() && fields[0] != kDefaultPrimaryKey){
            keys << fields[0];
            keyEntries.append({table, fields[0]});
        }
        tablePrimaryKeys[table] = keys;
    }
}

void SqliteDB::selectAllJournalNames()
{
    journalNameToKeyIndices.clear();
    allJournalNamesList.clear();

    QSqlQuery query;
    for(int idx = 0; idx < keyEntries.size(); idx++){
        const QString &table = keyEntries[idx].tableName;
        const QString &primaryKey = keyEntries[idx].keyField;

        if(database.isOpen()){
            if (!query.exec("SELECT " + primaryKey + " FROM " + table)){
                qWarning() << "Error: Failed to select" << table << __FUNCTION__ << database.lastError();
                continue;
            }

            while (query.next()){
                QString name = query.value(0).toString();
                if(name.isEmpty()) continue;

                const QString lowerName = name.toLower();
                journalNameToKeyIndices[lowerName].append(idx);

                if(!allJournalNamesList.contains(name, Qt::CaseInsensitive))
                    allJournalNamesList << name;
            }
        }
    }

    qDebug() << "Total journal names:" << allJournalNamesList.length();
}

QStringList SqliteDB::sortSpecialStrings(const QStringList &input) {
    struct StringItem {
        QString original;
        QString prefix;
        int year = 0;
        int priority = 0;
    };

    const QHash<QString, int> kPrefixPriority = {
        {"GJQKYJMD", 0},
        {"JCR", 1},
        {"CCF", 2},
        {"CCFT", 3},
        {"XR", 4},
        {"FQBJCR", 5}
    };

    const QRegularExpression kPattern("^(\\D+)(\\d+)$");

    QList<StringItem> items;
    for (const QString &s : input) {
        QRegularExpressionMatch match = kPattern.match(s);
        if (match.hasMatch()) {
            items.append({s, match.captured(1), match.captured(2).toInt(),
                         kPrefixPriority.value(match.captured(1), INT_MAX)});
        } else {
            items.append({s, s, 0, INT_MAX});
        }
    }

    std::sort(items.begin(), items.end(), [](const StringItem &a, const StringItem &b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.year != b.year) return a.year > b.year;
        return a.original < b.original;
    });

    QStringList result;
    result.reserve(items.size());
    for (const auto &item : items) {
        result << item.original;
    }
    return result;
}
