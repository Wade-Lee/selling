#include "config.h"
#include "gui_main.h"
#include "quote.h"
#include "trader.h"

#include <QApplication>

using namespace HuiFu;

int main(int argc, char *argv[])
{
    // 读取配置文件
    if (!Config::get_instance().init("config.toml"))
    {
        exit(EXIT_FAILURE);
    }

    QApplication a(argc, argv);
    QuoteController quoteController;
    TraderController traderController;
    GuiMain w{quoteController, traderController};
    w.show();
    return a.exec();
}