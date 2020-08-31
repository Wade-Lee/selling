#include "gui_order_trade.h"
#include "ui_gui_order_trade.h"

using namespace std;
using namespace HuiFu;

size_t GuiOrderTrade::ID = 0;

GuiOrderTrade::GuiOrderTrade(QWidget *parent) : QWidget(parent),
                                                ui(new Ui::GuiOrderTrade),
                                                id(GuiOrderTrade::ID++)
{
    ui->setupUi(this);
}

GuiOrderTrade::~GuiOrderTrade()
{
    delete ui;
}

void GuiOrderTrade::OnTraderLogin(size_t id_)
{
    if (id != id_)
    {
        return;
    }

    int rows = ui->orderTable->rowCount();
    for (int i = rows - 1; i < 0; i++)
    {
        ui->orderTable->removeRow(i);
    }
}

void GuiOrderTrade::OnOrderTraded(size_t id_, const TradeData &d)
{
    if (id != id_)
    {
        return;
    }

    qDebug() << QStringLiteral("成交：") << d.order_xtp_id << d.stock_code << d.trade_time << d.quantity << d.price;

    int rows = ui->orderTable->rowCount();

    ui->orderTable->setRowCount(rows + 1);

    ui->orderTable->setItem(rows, 0, new QTableWidgetItem(d.trade_time));
    ui->orderTable->setItem(rows, 1, new QTableWidgetItem(d.stock_code));
    auto pQSI = StockStaticInfo::GetInstance().GetQSI(d.stock_code);
    if (pQSI)
    {
        ui->orderTable->setItem(rows, 2, new QTableWidgetItem(pQSI->ticker_name));
    }
    ui->orderTable->setItem(rows, 3, new QTableWidgetItem(d.side == XTP_SIDE_BUY ? QStringLiteral("买入") : QStringLiteral("卖出")));
    ui->orderTable->setItem(rows, 4, new QTableWidgetItem(QString::number(d.quantity)));
    ui->orderTable->setItem(rows, 5, new QTableWidgetItem(QString::number(d.price, 'f', 2)));
    ui->orderTable->setItem(rows, 6, new QTableWidgetItem(QString::number(d.trade_amount, 'f', 2)));
}