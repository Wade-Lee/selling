#include "config.h"
#include "gui_main.h"
#include "quote.h"
#include "trader.h"

#include <QApplication>
#include <QMessageLogContext>
#include <QMutex>
#include <QDateTime>
#include <QFile>

#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

using namespace HuiFu;

void MessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;

    QString text;
    switch (type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;
    case QtWarningMsg:
        text = QString("Warning:");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:");
        break;
    default:
        break;
    }

    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(current_date).arg(msg);

    QMutexLocker lock(&mutex);
    QFile f("qtlog.txt");
    f.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&f);
    text_stream << message << "\r\n";
    f.flush();
    f.close();
}

int main(int argc, char *argv[])
{
    // 读取配置文件
    if (!Config::get_instance().init("config.toml"))
    {
        exit(EXIT_FAILURE);
    }

    QApplication a(argc, argv);
    qInstallMessageHandler(MessageHandler);
    QuoteController quoteController;
    TraderController traderController;
    GuiMain w{quoteController, traderController};
    w.show();
    return a.exec();
}