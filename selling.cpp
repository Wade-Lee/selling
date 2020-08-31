#include "config.h"
#include "gui_main.h"

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
    GuiMain w;
    w.show();
    return a.exec();
}