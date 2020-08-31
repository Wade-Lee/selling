#include "gui_order_insert.h"
#include "ui_gui_order_insert.h"

using namespace std;
using namespace HuiFu;

size_t GuiOrderInsert::ID = 0;

GuiOrderInsert::GuiOrderInsert(QWidget *parent) : QWidget(parent),
                                                  ui(new Ui::GuiOrderInsert),
                                                  id(GuiOrderInsert::ID++)
{
    ui->setupUi(this);
}

GuiOrderInsert::~GuiOrderInsert()
{
    delete ui;
}

void GuiOrderInsert::OnTraderLogin(size_t id_)
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
    mOrders.clear();
}

void GuiOrderInsert::OnOrderReceived(size_t id_, const OrderData &d)
{
    if (id != id_)
    {
        return;
    }

    int rows = ui->orderTable->rowCount();

    ui->orderTable->setRowCount(rows + 1);

    ui->orderTable->setItem(rows, 0, new QTableWidgetItem(d.insert_time));
    ui->orderTable->setItem(rows, 1, new QTableWidgetItem(d.stock_code));
    auto pQSI = StockStaticInfo::GetInstance().GetQSI(d.stock_code);
    if (pQSI)
    {
        ui->orderTable->setItem(rows, 2, new QTableWidgetItem(pQSI->ticker_name));
    }
    ui->orderTable->setItem(rows, 3, new QTableWidgetItem(d.side == XTP_SIDE_BUY ? QStringLiteral("买入") : QStringLiteral("卖出")));
    ui->orderTable->setItem(rows, 4, new QTableWidgetItem(QString::number(d.quantity)));
    ui->orderTable->setItem(rows, 5, new QTableWidgetItem(QString::number(0)));
    ui->orderTable->setItem(rows, 6, new QTableWidgetItem(QString::number(0)));
    ui->orderTable->setItem(rows, 7, new QTableWidgetItem(QString::number(d.price, 'f', 2)));

    mOrders[d.order_xtp_id] = rows;
}

void GuiOrderInsert::OnOrderTraded(size_t id_, const TradeData &d)
{
    if (id != id_)
    {
        return;
    }

    if (mOrders.find(d.order_xtp_id) != mOrders.end())
    {
        int row = mOrders.at(d.order_xtp_id);
        int qty = ui->orderTable->item(row, 5)->text().toInt();
        qty += d.quantity;
        ui->orderTable->item(row, 5)->setText(QString::number(qty));
    }
}

void GuiOrderInsert::OnOrderCanceled(size_t id_, const CancelData &d)
{
    if (id != id_)
    {
        return;
    }

    if (mOrders.find(d.order_xtp_id) != mOrders.end())
    {
        int row = mOrders.at(d.order_xtp_id);
        int qty = ui->orderTable->item(row, 6)->text().toInt();
        qty += d.qty_left;
        ui->orderTable->item(row, 6)->setText(QString::number(qty));
    }
}