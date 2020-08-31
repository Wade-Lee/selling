#include "gui_tradetab.h"
#include "ui_gui_tradetab.h"

GuiTradeTab::GuiTradeTab(QWidget *parent) : QTabWidget(parent),
                                            ui(new Ui::GuiTradeTab)
{
    ui->setupUi(this);
}

GuiTradeTab::~GuiTradeTab()
{
    delete ui;
}

void GuiTradeTab::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_F3:
        setCurrentIndex(1);
        break;
    case Qt::Key_F4:
        setCurrentIndex(0);
        break;
    case Qt::Key_F5:
        setCurrentIndex(2);
        break;
    case Qt::Key_F6:
        setCurrentIndex(3);
        break;
    default:
        break;
    }
}