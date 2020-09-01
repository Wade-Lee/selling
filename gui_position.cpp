#include "gui_position.h"
#include "ui_gui_position.h"
#include "gui_base.h"

#include <QSpinBox>
#include <QDoubleSpinBox>

using namespace std;
using namespace HuiFu;

size_t GuiPosition::ID = 0;

GuiPosition::GuiPosition(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::GuiPosition),
                                            id(GuiPosition::ID++),
                                            mPositions()
{
    ui->setupUi(this);

    QObject::connect(ui->positionTable, &QTableWidget::itemDoubleClicked, [&](QTableWidgetItem *item) {
        int row = item->row();
        auto stock_code = ui->positionTable->item(row, 0)->text();
        auto stock_name = ui->positionTable->item(row, 1)->text();
        int total_qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(row, 2))->value();
        int sellable_qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(row, 3))->value();
        double price = qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(row, 4))->value();
        UserSelectPosition(PositionData{stock_code, stock_name, XTP_EXCHANGE_UNKNOWN, total_qty, sellable_qty}, price); });
}

GuiPosition::~GuiPosition()
{
    delete ui;
}

void GuiPosition::OnTraderLogin(size_t id_)
{
    if (id != id_)
    {
        return;
    }

    // int rows = ui->positionTable->rowCount();
    // for (int i = rows - 1; i < 0; i++)
    // {
    //     ui->positionTable->removeRow(i);
    // }
    // mPositions.clear();
}

void GuiPosition::OnPositionReceived(size_t id_, const PositionData &d)
{
    if (id != id_)
    {
        return;
    }

    PositionReqSubscribe(
        d.exchange_id,
        d.stock_code);

    insert_row(d.stock_code, d.stock_name, d.total_qty, d.sellable_qty);
}

void GuiPosition::OnPositionQuoteReceived(const TraderMarketData &d)
{
    if (mPositions.find(d.stock_code) != mPositions.end())
    {
        int i = ui->positionTable->row(mPositions.at(d.stock_code));
        qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(i, 4))->setValue(d.last_price);
        int qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 2))->value();
        qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(i, 5))->setValue(d.last_price * qty);
    }
}

void GuiPosition::OnOrderSellReceived(size_t id_, const OrderData &d)
{
    if (id != id_)
    {
        return;
    }

    if (mPositions.find(d.stock_code) != mPositions.end())
    {
        int i = ui->positionTable->row(mPositions.at(d.stock_code));
        int sellable_qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 3))->value();
        qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 3))->setValue(sellable_qty - d.quantity);
    }
}

void GuiPosition::OnOrderSellCanceled(size_t id_, const CancelData &d)
{
    if (id != id_)
    {
        return;
    }

    if (mPositions.find(d.stock_code) != mPositions.end())
    {
        int i = ui->positionTable->row(mPositions.at(d.stock_code));
        int sellable_qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 3))->value();
        qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 3))->setValue(sellable_qty + d.qty_left);
    }
}

void GuiPosition::OnOrderSellTraded(size_t id_, const TradeData &d)
{
    if (id != id_)
    {
        return;
    }

    auto it = mPositions.find(d.stock_code);
    if (it != mPositions.end())
    {
        int i = ui->positionTable->row(it->second);
        int total_qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 2))->value();

        total_qty -= d.quantity;

        if (total_qty == 0)
        {
            ui->positionTable->removeRow(ui->positionTable->row(it->second));
            mPositions.erase(it);
            return;
        }

        qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 2))->setValue(total_qty);
        qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(i, 4))->setValue(d.price);
        qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(i, 5))->setValue(d.price * total_qty);
    }
}

void GuiPosition::OnOrderBuyTraded(size_t id_, const TradeData &d)
{
    if (id != id_)
    {
        return;
    }

    auto it = mPositions.find(d.stock_code);
    if (it != mPositions.end())
    {
        int i = ui->positionTable->row(it->second);
        int total_qty = qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 2))->value();

        total_qty += d.quantity;

        qobject_cast<QSpinBox *>(ui->positionTable->cellWidget(i, 2))->setValue(total_qty);
        qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(i, 4))->setValue(d.price);
        qobject_cast<QDoubleSpinBox *>(ui->positionTable->cellWidget(i, 5))->setValue(d.price * total_qty);
    }
    else
    {
        auto pQSI = StockStaticInfo::GetInstance().GetQSI(d.stock_code);
        QString stock_name = "";
        if (pQSI)
        {
            stock_name = pQSI->ticker_name;
            PositionReqSubscribe(
                pQSI->exchange_id,
                d.stock_code);
        }
        insert_row(d.stock_code, stock_name, d.quantity, 0, d.price);
    }
}

void GuiPosition::insert_row(const StockCode &stock_code, const QString &stock_name, int64_t total_qty, int64_t sellable_qty, double price)
{
    int rows = ui->positionTable->rowCount();

    ui->positionTable->setRowCount(rows + 1);
    ui->positionTable->setItem(rows, 0, new QTableWidgetItem(stock_code));
    ui->positionTable->itemAt(rows, 0)->setData(Qt::UserRole, QVariant{stock_code});
    ui->positionTable->setItem(rows, 1, new QTableWidgetItem(stock_name));
    GuiBase::AddReadOnlySpinBox(*(ui->positionTable), rows, 2, total_qty);
    GuiBase::AddReadOnlySpinBox(*(ui->positionTable), rows, 3, sellable_qty);
    GuiBase::AddReadOnlyDoubleSpinBox(*(ui->positionTable), rows, 4, price);
    GuiBase::AddReadOnlyDoubleSpinBox(*(ui->positionTable), rows, 5, price * total_qty);

    mPositions[stock_code] = ui->positionTable->item(rows, 0);
}