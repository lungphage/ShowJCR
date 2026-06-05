#include "showjcr.h"

#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QMutex>
#include <QDateTime>
#include <QFile>
#include <QDir>

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#ifndef QT_DEBUG
    if(type == QtDebugMsg)
        return;
#endif

    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QStringLiteral("Debug:");
        break;
    case QtInfoMsg:
        text = QStringLiteral("Info:");
        break;
    case QtWarningMsg:
        text = QStringLiteral("Warning:");
        break;
    case QtCriticalMsg:
        text = QStringLiteral("Critical:");
        break;
    case QtFatalMsg:
        text = QStringLiteral("Fatal:");
        break;
    default:
        return;
    }

    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ");
    QString message;

#ifdef QT_DEBUG
    QString context_info = QString("File:(%1) Line:(%2) Function:(%3)")
                               .arg(QString(context.file)).arg(context.line).arg(context.function);
    message = QString("%1 (%2) %3 %4").arg(text, current_date_time, context_info, msg);
#else
    message = QString("%1 (%2) %3").arg(text, current_date_time, msg);
#endif

    static QMutex mutex;
    static QString logName = QApplication::applicationName() + "_log.txt";
    QMutexLocker locker(&mutex);
    QFile file(QDir::temp().absoluteFilePath(logName));
    if(file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
        QTextStream text_stream(&file);
        text_stream << message << "\n";
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qInstallMessageHandler(outputMessage);

    QSharedMemory sm(QApplication::applicationName());
    if(sm.attach()){
        qWarning() << "SingleApp is running!" << __FUNCTION__;
        QMessageBox::warning(nullptr, "程序正在运行", "程序已经启动，请不要重复运行！请检查任务栏或系统托盘！");
        return 0;
    }

    if(!sm.create(1)){
        sm.detach();
        if(!sm.create(1)){
            qCritical() << "Failed to create shared memory for single instance check";
            return 1;
        }
    }

    ShowJCR w;
    if (argc > 1 && QString(argv[1]) == "autoStart"){
        qDebug() << "autoStart mode";
        w.hide();
    } else {
        w.show();
    }

    int ret = a.exec();
    sm.detach();
    return ret;
}
