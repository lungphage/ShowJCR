#ifndef SHOWJCR_H
#define SHOWJCR_H

#include "sqlitedb.h"

#include <QWidget>
#include <QClipboard>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QDir>
#include <QColor>
#include <QAbstractNativeEventFilter>

QT_BEGIN_NAMESPACE
namespace Ui { class ShowJCR; }
QT_END_NAMESPACE

class AboutDialog;
class TableSelectorDialog;
class BatchQueryDialog;
class QShortcut;

class ShowJCR : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    ShowJCR(QWidget *parent = nullptr);
    ~ShowJCR();

private slots:
    void on_pushButton_selectJournal_clicked();
    void on_lineEdit_journalName_returnPressed();
    void getClipboard();
    void on_checkBox_autoStart_stateChanged(int arg1);
    void on_checkBox_exit2Taskbar_stateChanged(int arg1);
    void on_checkBox_autoActivateWindow_stateChanged(int arg1);
    void on_checkBox_monitorClipboard_stateChanged(int arg1);
    int OnSystemTrayClicked(QSystemTrayIcon::ActivationReason reason);
    int OnExit();
    void on_lineEdit_journalName_textEdited(const QString &arg1);
    void on_toolButton_list_clicked();
    void show_selectTable();
    void show_about();

private:
    static constexpr const char* kCueInput = "请输入期刊名称！";
    static constexpr const char* kCueCheck = "请检查期刊名称！";
    static constexpr const char* kCueNoTable = "请至少选择一个表！";
    static constexpr int kMaxHistory = 10;

    const QColor color_header = QColor(119, 136, 153);
    const QColor color_highlight = QColor(240, 128, 128);

    static const QString author;
    static const QString version;
    static const QString email;
    static const QString codeURL;
    static const QString updateURL;
    static const QString windowTitile;
    static const QString logoIconName;
    static const QString datasetName;
    static const QString defaultJournal;

    Ui::ShowJCR *ui;
    SqliteDB *sqliteDB;
    QString appName;
    QDir appDir;
    QString appPath;

    QMenu *menu;
    QSystemTrayIcon m_systray;
    TableSelectorDialog *selectTableDialog;
    AboutDialog *aboutDialog;
    BatchQueryDialog *batchQueryDialog;

    QSettings *settings;
    bool autoStart = false;
    bool exit2Taskbar = false;
    bool monitorClipboard = false;
    bool autoActivateWindow = false;
    bool darkMode = false;
    QStringList selectedTables;

    QList<FieldValue> journalInfo;
    QString journalName;

    QStringList searchHistory;
    QStringList favorites;

    // 快捷键
    QShortcut *shortcutFocusSearch;
    QShortcut *shortcutEsc;
    QShortcut *shortcutExport;
    QShortcut *shortcutTrend;
    QShortcut *shortcutBatch;
    QShortcut *shortcutFavorite;
    QShortcut *shortcutGlobal;

    void run(const QString &input);
    void updateGUI();
    void setAutoStart();
    void closeEvent(QCloseEvent *event) override;
    void setupCompleter();
    void setupShortcuts();
    void setupMenu();

    // 新功能
    void addSearchHistory(const QString &name);
    void updateHistoryMenu();
    void toggleFavorite();
    void updateFavoriteIcon();
    void showTrendChart();
    void exportCurrentResult();
    void showBatchQuery();
    void toggleDarkMode();
    void applyTheme();
    void activateFromGlobalHotkey();

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
};

#endif // SHOWJCR_H
