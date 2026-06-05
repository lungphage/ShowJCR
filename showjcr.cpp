#include "showjcr.h"
#include "./ui_showjcr.h"
#include "aboutdialog.h"
#include "tableselectordialog.h"
#include "batchquerydialog.h"
#include "trendchartdialog.h"

#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QCompleter>
#include <QMimeData>
#include <QMenu>
#include <QShortcut>
#include <QFileDialog>
#include <QTextStream>
#include <QApplication>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

const QString ShowJCR::author = "hitfyd";
const QString ShowJCR::version = "v2026-2.0";
const QString ShowJCR::email = "hitfyd@foxmail.com";
const QString ShowJCR::codeURL = "https://github.com/hitfyd/ShowJCR";
const QString ShowJCR::updateURL = "https://github.com/hitfyd/ShowJCR/releases";
const QString ShowJCR::windowTitile = tr("分区表2026");
const QString ShowJCR::logoIconName = ":/image/jcr-logo.jpg";
const QString ShowJCR::datasetName = "jcr.db";
const QString ShowJCR::defaultJournal = "National Science Review";

ShowJCR::ShowJCR(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShowJCR)
    , batchQueryDialog(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle(windowTitile + " " + version);

    appName = QApplication::applicationName();
    appDir = QDir(QApplication::applicationDirPath());
    appPath = QApplication::applicationFilePath();

    qDebug() << "start check:" << appName << appDir.path() << appPath;

    // 数据库
    sqliteDB = new SqliteDB(appDir, datasetName, this);

    ui->lineEdit_journalName->setPlaceholderText(kCueInput);

    // 读取运行参数
    settings = new QSettings(author, appName, this);
    autoStart = settings->value("autoStart").toBool();
    exit2Taskbar = settings->value("exit2Taskbar").toBool();
    monitorClipboard = settings->value("monitorClipboard").toBool();
    autoActivateWindow = settings->value("autoActivateWindow").toBool();
    darkMode = settings->value("darkMode").toBool();
    searchHistory = settings->value("searchHistory").toStringList();
    favorites = settings->value("favorites").toStringList();

    ui->checkBox_autoStart->setChecked(autoStart);
    ui->checkBox_exit2Taskbar->setChecked(exit2Taskbar);
    ui->checkBox_monitorClipboard->setChecked(monitorClipboard);
    ui->checkBox_autoActivateWindow->setChecked(autoActivateWindow);

    QStringList old_selectedTables = settings->value("selectedTables").toStringList();
    if(old_selectedTables.isEmpty()){
        selectedTables = sqliteDB->getAllTableNames();
    } else {
        selectedTables.clear();
        for (const QString &item : sqliteDB->getAllTableNames()) {
            if (old_selectedTables.contains(item)) {
                selectedTables.append(item);
            }
        }
    }
    sqliteDB->selectTableNames(selectedTables);

    setupCompleter();
    setupMenu();
    setupShortcuts();
    applyTheme();

    selectTableDialog = new TableSelectorDialog(sqliteDB->getAllTableNames(), selectedTables, this);
    aboutDialog = new AboutDialog(appName, version, email, codeURL, updateURL, this);

    setAutoStart();

#ifdef Q_OS_WIN
    // 注册全局热键 Win+J
    RegisterHotKey((HWND)winId(), 1, MOD_WIN | MOD_NOREPEAT, 0x4A); // 0x4A = 'J'
    qApp->installNativeEventFilter(this);
#endif
}

ShowJCR::~ShowJCR()
{
    settings->setValue("autoStart", autoStart);
    settings->setValue("exit2Taskbar", exit2Taskbar);
    settings->setValue("monitorClipboard", monitorClipboard);
    settings->setValue("autoActivateWindow", autoActivateWindow);
    settings->setValue("darkMode", darkMode);
    settings->setValue("selectedTables", selectedTables);
    settings->setValue("searchHistory", searchHistory);
    settings->setValue("favorites", favorites);

#ifdef Q_OS_WIN
    UnregisterHotKey((HWND)winId(), 1);
    qApp->removeNativeEventFilter(this);
#endif
}

// ============== 基础功能 ==============

void ShowJCR::on_pushButton_selectJournal_clicked()
{
    run(ui->lineEdit_journalName->text());
}

void ShowJCR::on_lineEdit_journalName_returnPressed()
{
    on_pushButton_selectJournal_clicked();
}

void ShowJCR::run(const QString &input)
{
    QString tempJournalName = input.simplified();
    journalName = tempJournalName;

    if(journalName.isEmpty()){
        return;
    }

    // 精确匹配
    const QStringList &names = sqliteDB->getAllJournalNames();
    QString matchedName;
    for(const QString &n : names){
        if(n.compare(journalName, Qt::CaseInsensitive) == 0 ||
           n.compare(journalName.remove('.'), Qt::CaseInsensitive) == 0){
            matchedName = n;
            break;
        }
    }

    // 模糊匹配
    if(matchedName.isEmpty()){
        QStringList fuzzy = sqliteDB->fuzzySearch(journalName, 1);
        if(!fuzzy.isEmpty()){
            matchedName = fuzzy.first();
        }
    }

    if(matchedName.isEmpty()){
        if(!ui->lineEdit_journalName->text().contains(kCueCheck)){
            ui->lineEdit_journalName->setText(kCueCheck + ui->lineEdit_journalName->text());
        }
        return;
    }

    journalName = matchedName;
    qDebug() << "select the journal:" << journalName;
    journalInfo = sqliteDB->getJournalInfo(journalName);
    addSearchHistory(journalName);
    updateGUI();
}

void ShowJCR::updateGUI()
{
    ui->tableView_journalInformation->setShowGrid(true);
    ui->tableView_journalInformation->setGridStyle(Qt::DashLine);
    ui->tableView_journalInformation->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QStandardItemModel* oldModel = qobject_cast<QStandardItemModel*>(ui->tableView_journalInformation->model());

    auto *model = new QStandardItemModel(ui->tableView_journalInformation);
    for(int i = 0; i < journalInfo.size(); i++){
        auto *key_item = new QStandardItem(journalInfo[i].fieldName);
        auto *value_item = new QStandardItem(journalInfo[i].value);

        if(journalInfo[i].fieldName.contains("年份")){
            key_item->setBackground(color_header);
            value_item->setBackground(color_header);
        }
        if(journalInfo[i].fieldName.contains("IF Quartile") ||
            journalInfo[i].fieldName.contains("CCF推荐类型") || journalInfo[i].fieldName.contains("预警") ||
            journalInfo[i].fieldName.contains("大类分区") || journalInfo[i].fieldName.contains("Top") ||
            journalInfo[i].fieldName.contains("标注")){
            key_item->setBackground(color_highlight);
            value_item->setBackground(color_highlight);
        }
        model->setItem(i, 0, key_item);
        model->setItem(i, 1, value_item);
    }

    ui->tableView_journalInformation->setModel(model);
    delete oldModel;

    ui->lineEdit_journalName->setText(journalName);
    updateFavoriteIcon();

    if(autoActivateWindow){
        this->showNormal();
        this->activateWindow();
    }
}

void ShowJCR::setAutoStart()
{
    QString nativeAppPath = QDir::toNativeSeparators(appPath);
    QString autoStartValue = nativeAppPath + " autoStart";
    QString regPath = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    QSettings reg(regPath, QSettings::NativeFormat);
    QString val = reg.value(appName).toString();
    if(val != autoStartValue && autoStart){
        reg.setValue(appName, autoStartValue);
    } else if(val == autoStartValue && !autoStart){
        reg.remove(appName);
    }
}

void ShowJCR::closeEvent(QCloseEvent *event)
{
    if(exit2Taskbar){
        this->hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void ShowJCR::getClipboard()
{
    if(monitorClipboard){
        const QClipboard *clipboard = QApplication::clipboard();
        const QMimeData *mimeData = clipboard->mimeData();
        if (mimeData->hasText()) {
            run(clipboard->text());
        }
    }
}

void ShowJCR::on_checkBox_autoStart_stateChanged(int arg1) { autoStart = arg1; setAutoStart(); }
void ShowJCR::on_checkBox_exit2Taskbar_stateChanged(int arg1) { exit2Taskbar = arg1; }
void ShowJCR::on_checkBox_autoActivateWindow_stateChanged(int arg1) { autoActivateWindow = arg1; }
void ShowJCR::on_checkBox_monitorClipboard_stateChanged(int arg1) { monitorClipboard = arg1; }

int ShowJCR::OnSystemTrayClicked(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick){
        this->showNormal();
        this->activateWindow();
    }
    return 0;
}

int ShowJCR::OnExit()
{
    QApplication::exit(0);
    return 0;
}

void ShowJCR::on_lineEdit_journalName_textEdited(const QString &arg1)
{
    if(arg1.contains(kCueCheck)){
        ui->lineEdit_journalName->setText(arg1.split(kCueCheck).last());
    }
}

void ShowJCR::on_toolButton_list_clicked()
{
    QPoint pos(0, ui->toolButton_list->sizeHint().height());
    menu->exec(ui->toolButton_list->mapToGlobal(pos));
}

void ShowJCR::show_selectTable()
{
    if(selectTableDialog->exec() == QDialog::Accepted) {
        selectedTables = selectTableDialog->selectedTables();
        if(selectedTables.isEmpty()){
            ui->lineEdit_journalName->setText(kCueNoTable);
            return;
        }
        sqliteDB->selectTableNames(selectedTables);
        run(ui->lineEdit_journalName->text());
        setupCompleter();
    }
}

void ShowJCR::show_about()
{
    aboutDialog->show();
}

void ShowJCR::setupCompleter()
{
    QCompleter *old = ui->lineEdit_journalName->completer();
    if (old) {
        old->deleteLater();
    }

    auto *pCompleter = new QCompleter(sqliteDB->getAllJournalNames(), this);
    pCompleter->setFilterMode(Qt::MatchContains);
    pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->lineEdit_journalName->setCompleter(pCompleter);
}

// ============== 菜单 ==============

void ShowJCR::setupMenu()
{
    menu = new QMenu(this);
    menu->addAction(ui->actionSelectTable);
    menu->addAction(ui->actionAbout);
    menu->addSeparator();

    // 搜索历史子菜单
    auto *historyMenu = menu->addMenu("搜索历史");
    historyMenu->setObjectName("historyMenu");
    menu->addSeparator();

    // 新功能菜单项
    auto *actFav = menu->addAction("收藏当前期刊");
    actFav->setObjectName("actionFavorite");
    connect(actFav, &QAction::triggered, this, &ShowJCR::toggleFavorite);

    auto *actTrend = menu->addAction("IF 趋势图");
    actTrend->setObjectName("actionTrend");
    connect(actTrend, &QAction::triggered, this, &ShowJCR::showTrendChart);

    auto *actExport = menu->addAction("导出当前结果");
    actExport->setObjectName("actionExport");
    connect(actExport, &QAction::triggered, this, &ShowJCR::exportCurrentResult);

    auto *actBatch = menu->addAction("批量查询");
    actBatch->setObjectName("actionBatch");
    connect(actBatch, &QAction::triggered, this, &ShowJCR::showBatchQuery);

    menu->addSeparator();

    auto *actTheme = menu->addAction("深色模式");
    actTheme->setObjectName("actionTheme");
    connect(actTheme, &QAction::triggered, this, &ShowJCR::toggleDarkMode);

    menu->addSeparator();
    menu->addAction(ui->actionExit);

    // 系统托盘
    connect(ui->actionSelectTable, &QAction::triggered, this, &ShowJCR::show_selectTable);
    connect(ui->actionAbout, &QAction::triggered, this, &ShowJCR::show_about);
    connect(ui->actionExit, &QAction::triggered, this, &ShowJCR::OnExit);

    m_systray.setToolTip(appName + " " + version);
    m_systray.setIcon(QIcon(logoIconName));
    m_systray.setContextMenu(menu);
    m_systray.show();
    connect(&m_systray, &QSystemTrayIcon::activated, this, &ShowJCR::OnSystemTrayClicked);

    // 剪切板监听
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &ShowJCR::getClipboard);

    updateHistoryMenu();
}

void ShowJCR::updateHistoryMenu()
{
    auto *historyMenu = menu->findChild<QMenu*>("historyMenu");
    if(!historyMenu) return;
    historyMenu->clear();

    if(searchHistory.isEmpty()){
        historyMenu->addAction("（无记录）")->setEnabled(false);
        return;
    }

    for(const QString &name : searchHistory){
        QAction *act = historyMenu->addAction(name);
        connect(act, &QAction::triggered, this, [this, name](){
            run(name);
        });
    }

    historyMenu->addSeparator();
    QAction *clearAct = historyMenu->addAction("清空历史");
    connect(clearAct, &QAction::triggered, this, [this](){
        searchHistory.clear();
        updateHistoryMenu();
    });
}

// ============== 快捷键 ==============

void ShowJCR::setupShortcuts()
{
    // Ctrl+F 聚焦搜索框
    shortcutFocusSearch = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(shortcutFocusSearch, &QShortcut::activated, this, [this](){
        ui->lineEdit_journalName->setFocus();
        ui->lineEdit_journalName->selectAll();
    });

    // Esc 清空搜索框
    shortcutEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(shortcutEsc, &QShortcut::activated, this, [this](){
        ui->lineEdit_journalName->clear();
        ui->lineEdit_journalName->setFocus();
    });

    // Ctrl+E 导出
    shortcutExport = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(shortcutExport, &QShortcut::activated, this, &ShowJCR::exportCurrentResult);

    // Ctrl+T 趋势图
    shortcutTrend = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(shortcutTrend, &QShortcut::activated, this, &ShowJCR::showTrendChart);

    // Ctrl+B 批量查询
    shortcutBatch = new QShortcut(QKeySequence("Ctrl+B"), this);
    connect(shortcutBatch, &QShortcut::activated, this, &ShowJCR::showBatchQuery);

    // Ctrl+D 收藏
    shortcutFavorite = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(shortcutFavorite, &QShortcut::activated, this, &ShowJCR::toggleFavorite);
}

// ============== 搜索历史 ==============

void ShowJCR::addSearchHistory(const QString &name)
{
    searchHistory.removeAll(name);
    searchHistory.prepend(name);
    while(searchHistory.size() > kMaxHistory)
        searchHistory.removeLast();
    updateHistoryMenu();
}

// ============== 收藏夹 ==============

void ShowJCR::toggleFavorite()
{
    if(journalName.isEmpty()) return;

    if(favorites.contains(journalName, Qt::CaseInsensitive)){
        // 取消收藏
        for(int i = 0; i < favorites.size(); i++){
            if(favorites[i].compare(journalName, Qt::CaseInsensitive) == 0){
                favorites.removeAt(i);
                break;
            }
        }
        m_systray.showMessage("取消收藏", journalName, QSystemTrayIcon::Information, 1500);
    } else {
        favorites.append(journalName);
        m_systray.showMessage("已收藏", journalName, QSystemTrayIcon::Information, 1500);
    }
    updateFavoriteIcon();
}

void ShowJCR::updateFavoriteIcon()
{
    auto *act = menu->findChild<QAction*>("actionFavorite");
    if(!act) return;

    bool isFav = false;
    for(const QString &f : favorites){
        if(f.compare(journalName, Qt::CaseInsensitive) == 0){
            isFav = true;
            break;
        }
    }
    act->setText(isFav ? "取消收藏" : "收藏当前期刊");
}

// ============== IF 趋势图 ==============

void ShowJCR::showTrendChart()
{
    if(journalInfo.isEmpty()){
        QMessageBox::information(this, "提示", "请先查询一个期刊");
        return;
    }

    QMap<QString, double> yearData;
    for(int i = 0; i < journalInfo.size(); i++){
        const QString &field = journalInfo[i].fieldName;
        if(field.contains("IF(") || field.contains("IF (")){
            // 提取年份: "IF(2024)" -> "2024"
            QString year;
            int start = field.indexOf('(');
            int end = field.indexOf(')');
            if(start >= 0 && end > start){
                year = field.mid(start + 1, end - start - 1);
            }
            bool ok;
            double val = journalInfo[i].value.toDouble(&ok);
            if(ok && !year.isEmpty()){
                yearData[year] = val;
            }
        }
    }

    if(yearData.isEmpty()){
        // 尝试另一种字段格式
        for(int i = 0; i < journalInfo.size(); i++){
            if(journalInfo[i].fieldName == "IF"){
                // 查找对应的年份字段
                QString year;
                if(i > 0 && journalInfo[i-1].fieldName.contains("年份")){
                    year = journalInfo[i-1].value;
                }
                bool ok;
                double val = journalInfo[i].value.toDouble(&ok);
                if(ok && !year.isEmpty()){
                    yearData[year] = val;
                }
            }
        }
    }

    if(yearData.isEmpty()){
        QMessageBox::information(this, "提示", "当前期刊无影响因子数据");
        return;
    }

    auto *dlg = new TrendChartDialog(journalName, yearData, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ============== 导出 ==============

void ShowJCR::exportCurrentResult()
{
    if(journalInfo.isEmpty()){
        QMessageBox::information(this, "提示", "请先查询一个期刊");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出查询结果", journalName + "_info.csv", "CSV Files (*.csv)");
    if(filePath.isEmpty()) return;

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::warning(this, "错误", "无法写入文件");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";  // BOM for Excel

    out << "\"字段\",\"值\"\n";
    for(const FieldValue &fv : journalInfo){
        QString key = fv.fieldName;
        QString val = fv.value;
        key.replace("\"", "\"\"");
        val.replace("\"", "\"\"");
        out << "\"" + key + "\",\"" + val + "\"\n";
    }

    file.close();
    QMessageBox::information(this, "导出成功", "已导出到: " + filePath);
}

// ============== 批量查询 ==============

void ShowJCR::showBatchQuery()
{
    if(!batchQueryDialog){
        batchQueryDialog = new BatchQueryDialog(sqliteDB, this);
    }
    batchQueryDialog->show();
    batchQueryDialog->raise();
    batchQueryDialog->activateWindow();
}

// ============== 深色模式 ==============

void ShowJCR::toggleDarkMode()
{
    darkMode = !darkMode;
    applyTheme();

    auto *act = menu->findChild<QAction*>("actionTheme");
    if(act) act->setText(darkMode ? "浅色模式" : "深色模式");
}

void ShowJCR::applyTheme()
{
    auto *act = menu ? menu->findChild<QAction*>("actionTheme") : nullptr;
    if(act) act->setText(darkMode ? "浅色模式" : "深色模式");

    if(darkMode){
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
        darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(45, 45, 48));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
        qApp->setPalette(darkPalette);
    } else {
        qApp->setPalette(qApp->style()->standardPalette());
    }
}

// ============== 全局热键 ==============

bool ShowJCR::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)
#ifdef Q_OS_WIN
    if(eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG"){
        auto *msg = static_cast<MSG*>(message);
        if(msg->message == WM_HOTKEY && msg->wParam == 1){
            activateFromGlobalHotkey();
            return true;
        }
    }
#endif
    return false;
}

void ShowJCR::activateFromGlobalHotkey()
{
    this->showNormal();
    this->activateWindow();
    this->raise();
    ui->lineEdit_journalName->setFocus();
    ui->lineEdit_journalName->selectAll();
}
