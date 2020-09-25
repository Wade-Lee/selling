#include "gui_tradetab.h"
#include "ui_gui_tradetab.h"

#include "gui_selectorder.h"
#include "gui_dialog.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QSpinBox>

using namespace std;
using namespace HuiFu;

size_t GuiTradeTab::ID = 0;

GuiTradeTab::GuiTradeTab(QWidget *parent) : QTabWidget(parent),
                                            ui(new Ui::GuiTradeTab),
                                            id(GuiTradeTab::ID++)
{
    ui->setupUi(this);

    ui->sellablePositionTable->installEventFilter(this);
    ui->orderTable->installEventFilter(this);
    ui->positionTable->installEventFilter(this);
    ui->orderTradeTable->installEventFilter(this);
    ui->orderInsertTable->installEventFilter(this);
}

GuiTradeTab::~GuiTradeTab()
{
    delete ui;
}

void GuiTradeTab::SetFocus(int index)
{
    this->setCurrentIndex(index);
    switch (index)
    {
    case 0:
        if (ui->sellablePositionTable->rowCount() > 0)
        {
            ui->sellablePositionTable->setFocus();
            ui->sellablePositionTable->selectRow(0);
        }
        break;
    case 1:
        if (ui->orderTable->rowCount() > 0)
        {
            ui->orderTable->setFocus();
            ui->orderTable->selectRow(0);
        }
        break;
    default:
        break;
    }
}

set<StockCode> GuiTradeTab::GetSellPositions() const
{
    set<StockCode> positions;
    for (auto &val : mSellPositions)
    {
        positions.insert(val.first);
    }
    return positions;
}

void GuiTradeTab::SetSellPositionIndex(StockCode stock_code, size_t index)
{
    if (mSellPositions.find(stock_code) != mSellPositions.end())
    {
        int i = ui->sellablePositionTable->row(mSellPositions.at(stock_code));
        ui->sellablePositionTable->item(i, 0)->setText(QString::number(index));
        ui->sellablePositionTable->sortItems(0);
    }
}

bool GuiTradeTab::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int k = keyEvent->key();
        if (k >= Qt::Key_A && k <= Qt::Key_Z)
        {
            if (watched == ui->orderTable && (keyEvent->modifiers() & Qt::ControlModifier))
            {
                if (k == Qt::Key_A)
                {
                    for (auto it : mOrders)
                    {
                        int i = ui->orderTable->row(it.second);
                        qobject_cast<GuiSelectOrder *>(ui->orderTable->cellWidget(i, 0))->SetSelect(true);
                    }
                    return true;
                }
                else if (k == Qt::Key_Z)
                {
                    for (auto it : mOrders)
                    {
                        int i = ui->orderTable->row(it.second);
                        qobject_cast<GuiSelectOrder *>(ui->orderTable->cellWidget(i, 0))->SetSelect(false);
                    }
                    return true;
                }
            }

            keyEvent->ignore();
            return true;
        }
        else if (k == Qt::Key_Space)
        {
            if (watched == ui->orderTable)
            {
                auto items = ui->orderTable->selectionModel()->selectedIndexes();
                int r = -1;
                foreach (auto &item, items)
                {
                    if (item.row() != r)
                    {
                        r = item.row();
                        qobject_cast<GuiSelectOrder *>(ui->orderTable->cellWidget(r, 0))->Activate();
                    }
                }
            }
            return true;
        }
        else if (k == Qt::Key_Return)
        {
            if (watched == ui->sellablePositionTable)
            {
                auto items = ui->sellablePositionTable->selectedItems();
                UserSelectPosition(items.at(0));
            }
            else if (watched == ui->orderTable)
            {
                req_cancel_orders();
            }
            return true;
        }
        return false;
    }
    else if (event->type() == QEvent::FocusOut)
    {
        focused = false;
        if (watched == ui->sellablePositionTable)
            ui->sellablePositionTable->setCurrentItem(nullptr);
        else if (watched == ui->orderTable)
            ui->orderTable->setCurrentItem(nullptr);
        else if (watched == ui->positionTable)
            ui->positionTable->setCurrentItem(nullptr);
        else if (watched == ui->orderTradeTable)
            ui->orderTradeTable->setCurrentItem(nullptr);
        else if (watched == ui->orderInsertTable)
            ui->orderInsertTable->setCurrentItem(nullptr);

        return false;
    }
    else if (event->type() == QEvent::FocusIn)
    {
        focused = true;
        return false;
    }
    else
        return false;
}

void GuiTradeTab::OnTraderLogin(size_t id_)
{
    if (id != id_)
    {
        return;
    }

    // 可卖持仓
    mSellPositions.clear();
    int rows = ui->sellablePositionTable->rowCount();
    for (int i = rows - 1; i < 0; i++)
    {
        ui->sellablePositionTable->removeRow(i);
    }

    // 可撤委托
    mOrders.clear();
    rows = ui->orderTable->rowCount();
    for (int i = rows - 1; i < 0; i++)
    {
        ui->orderTable->removeRow(i);
    }

    // 持仓列表
    mPositions.clear();
    rows = ui->positionTable->rowCount();
    for (int i = rows - 1; i < 0; i++)
    {
        ui->positionTable->removeRow(i);
    }

    // 当日成交
    rows = ui->orderTradeTable->rowCount();
    for (int i = rows - 1; i < 0; i++)
    {
        ui->orderTradeTable->removeRow(i);
    }

    // 当日委托
    mInsertOrders.clear();
    rows = ui->orderInsertTable->rowCount();
    for (int i = rows - 1; i < 0; i++)
    {
        ui->orderInsertTable->removeRow(i);
    }
}

void GuiTradeTab::insert_position(const StockCode &stock_code, const QString &stock_name, int64_t total_qty, double price)
{
    int rows = ui->positionTable->rowCount();

    ui->positionTable->setRowCount(rows + 1);
    ui->positionTable->setItem(rows, 0, new QTableWidgetItem(stock_code));
    ui->positionTable->setItem(rows, 1, new QTableWidgetItem(stock_name));
    ui->positionTable->setItem(rows, 2, new QTableWidgetItem(QString::number(total_qty)));
    ui->positionTable->setItem(rows, 3, new QTableWidgetItem(QString::number(price, 'f', 2)));
    ui->positionTable->setItem(rows, 4, new QTableWidgetItem(QString::number(price * total_qty, 'f', 2)));
    ui->positionTable->setItem(rows, 5, new QTableWidgetItem(QString::number(price, 'f', 2)));
    if (mSellPositionPrices.find(stock_code) == mSellPositionPrices.end())
    {
        ui->positionTable->setItem(rows, 6, new QTableWidgetItem(QString::number(0.0, 'f', 2)));
        ui->positionTable->setItem(rows, 7, new QTableWidgetItem(QString::number(0.0, 'f', 2)));
    }
    else
    {
        auto &prices = mSellPositionPrices.at(stock_code);
        ui->positionTable->setItem(rows, 5, new QTableWidgetItem(QString::number(prices.trade_avg_price, 'f', 2)));
        ui->positionTable->setItem(rows, 6, new QTableWidgetItem(QString::number(0.0, 'f', 2)));
        ui->positionTable->setItem(rows, 7, new QTableWidgetItem(QString::number(prices.GetTradeProfit(price, total_qty), 'f', 2)));
    }

    mPositions[stock_code] = ui->positionTable->item(rows, 0);
}

void GuiTradeTab::insert_position(const StockCode &stock_code, const QString &stock_name, int64_t total_qty, int64_t sellable_qty, double price)
{
    int rows = ui->sellablePositionTable->rowCount();

    ui->sellablePositionTable->setRowCount(rows + 1);
    ui->sellablePositionTable->setItem(rows, 0, new GuiIndexItem(QString::number(0)));
    ui->sellablePositionTable->setItem(rows, 1, new QTableWidgetItem(stock_code));
    ui->sellablePositionTable->setItem(rows, 2, new QTableWidgetItem(stock_name));
    ui->sellablePositionTable->setItem(rows, 3, new QTableWidgetItem(QString::number(total_qty)));
    ui->sellablePositionTable->setItem(rows, 4, new QTableWidgetItem(QString::number(sellable_qty)));
    ui->sellablePositionTable->setItem(rows, 5, new QTableWidgetItem(QString::number(price, 'f', 2)));
    ui->sellablePositionTable->setItem(rows, 6, new QTableWidgetItem(QString::number(price * total_qty, 'f', 2)));

    mSellPositions[stock_code] = ui->sellablePositionTable->item(rows, 0);
}

void GuiTradeTab::OnPositionReceived(size_t id_, const PositionData &d)
{
    if (id != id_)
    {
        return;
    }

    PositionReqSubscribe(
        d.exchange_id,
        d.stock_code);

    if (mSellPositionPrices.find(d.stock_code) == mSellPositionPrices.end())
        mSellPositionPrices[d.stock_code] = PositionPrice{-1.0, d.avg_price, 0.0};
    else
        mSellPositionPrices.at(d.stock_code).cost_price = d.avg_price;
    insert_position(d.stock_code, d.stock_name, d.total_qty, d.avg_price);
    if (d.yesterday_qty != 0 && d.total_qty != 0)
        insert_position(d.stock_code, d.stock_name, d.total_qty, d.sellable_qty);
}

void GuiTradeTab::OnPositionQuoteReceived(const TraderMarketData &d)
{

    if (mPositions.find(d.stock_code) != mPositions.end())
    {
        int i = ui->positionTable->row(mPositions.at(d.stock_code));
        ui->positionTable->item(i, 3)->setText(QString::number(d.last_price, 'f', 2));
        int total_qty = ui->positionTable->item(i, 2)->text().toInt();
        ui->positionTable->item(i, 4)->setText(QString::number(d.last_price * total_qty, 'f', 2));

        if (mSellPositionPrices.find(d.stock_code) != mSellPositionPrices.end())
        {
            auto &prices = mSellPositionPrices.at(d.stock_code);
            prices.pre_close_price = d.pre_close_price;
            ui->positionTable->item(i, 6)->setText(QString::number((prices.trade_avg_price / prices.pre_close_price - 1) * 100, 'f', 2));
            ui->positionTable->item(i, 7)->setText(QString::number(prices.GetTradeProfit(d.last_price, total_qty), 'f', 2));
        }
    }

    if (mSellPositions.find(d.stock_code) != mSellPositions.end())
    {
        int i = ui->sellablePositionTable->row(mSellPositions.at(d.stock_code));
        ui->sellablePositionTable->item(i, 5)->setText(QString::number(d.last_price, 'f', 2));
        ui->sellablePositionTable->item(i, 5)->setData(Qt::UserRole, QVariant{d.bid_price});
        int total_qty = ui->sellablePositionTable->item(i, 3)->text().toInt();
        ui->sellablePositionTable->item(i, 6)->setText(QString::number(d.last_price * total_qty, 'f', 2));
    }
}

void GuiTradeTab::OnOrderReceived(size_t id_, const OrderData &d)
{
    if (id != id_)
    {
        return;
    }

    // 可卖持仓
    if (d.side == XTP_SIDE_SELL)
    {
        if (mSellPositions.find(d.stock_code) != mSellPositions.end())
        {
            int i = ui->sellablePositionTable->row(mSellPositions.at(d.stock_code));
            int sellable_qty = ui->sellablePositionTable->item(i, 4)->text().toInt();
            ui->sellablePositionTable->item(i, 4)->setText(QString::number(sellable_qty - d.quantity));
        }
    }

    // 可撤委托
    ui->orderTable->setSortingEnabled(false);
    int rows = ui->orderTable->rowCount();
    ui->orderTable->setRowCount(rows + 1);
    GuiSelectOrder *selOrder = new GuiSelectOrder();
    selOrder->SetStockCode(d.stock_code);
    ui->orderTable->setCellWidget(rows, 0, selOrder);
    auto pQSI = StockStaticInfo::GetInstance().GetQSI(d.stock_code);
    while (!pQSI)
    {
        pQSI = StockStaticInfo::GetInstance().GetQSI(d.stock_code);
    }
    ui->orderTable->setItem(rows, 1, new QTableWidgetItem(pQSI->ticker_name));
    ui->orderTable->setItem(rows, 2, new QTableWidgetItem(QString::number(d.price, 'f', 2)));
    ui->orderTable->setItem(rows, 3, new QTableWidgetItem(QString::number(d.quantity)));
    ui->orderTable->setItem(rows, 4, new QTableWidgetItem(d.side == XTP_SIDE_BUY ? QStringLiteral("买入") : QStringLiteral("卖出")));
    ui->orderTable->setItem(rows, 5, new QTableWidgetItem(d.insert_time));
    mOrders[d.order_xtp_id] = ui->orderTable->item(rows, 1);
    ui->orderTable->setSortingEnabled(true);
    ui->orderTable->sortItems(5, Qt::DescendingOrder);

    // 当日委托
    ui->orderInsertTable->setSortingEnabled(false);
    rows = ui->orderInsertTable->rowCount();
    ui->orderInsertTable->setRowCount(rows + 1);
    ui->orderInsertTable->setItem(rows, 0, new QTableWidgetItem(d.insert_time));
    ui->orderInsertTable->setItem(rows, 1, new QTableWidgetItem(d.stock_code));
    ui->orderInsertTable->setItem(rows, 2, new QTableWidgetItem(pQSI->ticker_name));
    ui->orderInsertTable->setItem(rows, 3, new QTableWidgetItem(d.side == XTP_SIDE_BUY ? QStringLiteral("买入") : QStringLiteral("卖出")));
    ui->orderInsertTable->setItem(rows, 4, new QTableWidgetItem(QString::number(d.quantity)));
    ui->orderInsertTable->setItem(rows, 5, new QTableWidgetItem(QString::number(0)));
    ui->orderInsertTable->setItem(rows, 6, new QTableWidgetItem(QString::number(0)));
    ui->orderInsertTable->setItem(rows, 7, new QTableWidgetItem(QString::number(d.price, 'f', 2)));
    mInsertOrders[d.order_xtp_id] = ui->orderInsertTable->item(rows, 0);
    ui->orderInsertTable->setSortingEnabled(true);
    ui->orderInsertTable->sortItems(0, Qt::DescendingOrder);
}

void GuiTradeTab::add_buy_position(const HuiFu::TradeData &d)
{
    auto it = mPositions.find(d.stock_code);
    if (it != mPositions.end())
    {
        int i = ui->positionTable->row(it->second);
        int total_qty = ui->positionTable->item(i, 2)->text().toInt();
        total_qty += d.quantity;
        ui->positionTable->item(i, 2)->setText(QString::number(total_qty));
        ui->positionTable->item(i, 3)->setText(QString::number(d.price, 'f', 2));
        ui->positionTable->item(i, 4)->setText(QString::number(d.price * total_qty, 'f', 2));
        ui->positionTable->item(i, 5)->setText(QString::number(d.trade_avg_price, 'f', 2));
        ui->positionTable->item(i, 6)->setText(QString::number(0.0, 'f', 2));
        ui->positionTable->item(i, 7)->setText(QString::number(0.0, 'f', 2));
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
        insert_position(d.stock_code, stock_name, d.quantity, d.price);
    }
}

void GuiTradeTab::sub_sell_position(const HuiFu::TradeData &d, bool is_selling_position)
{
    if (is_selling_position)
    {
        auto it = mSellPositions.find(d.stock_code);
        if (it != mSellPositions.end())
        {
            int i = ui->sellablePositionTable->row(it->second);
            int total_qty = ui->sellablePositionTable->item(i, 3)->text().toInt();
            total_qty -= d.quantity;
            if (total_qty == 0)
            {
                ui->sellablePositionTable->removeRow(ui->sellablePositionTable->row(it->second));
                mSellPositions.erase(it);
                return;
            }

            ui->sellablePositionTable->item(i, 3)->setText(QString::number(total_qty));
            ui->sellablePositionTable->item(i, 5)->setText(QString::number(d.price, 'f', 2));
            ui->sellablePositionTable->item(i, 6)->setText(QString::number(d.price * total_qty, 'f', 2));
        }
    }
    else
    {
        if (mSellPositionPrices.find(d.stock_code) != mSellPositionPrices.end())
        {
            auto &prices = mSellPositionPrices.at(d.stock_code);
            prices.trade_avg_price = d.trade_avg_price;
            prices.trade_prices.push_back(d.price);
            prices.trade_qtys.push_back(d.quantity);
        }
        else
        {
            mSellPositionPrices[d.stock_code] = PositionPrice{-1.0, -1.0, d.trade_avg_price};
        }

        auto it = mPositions.find(d.stock_code);
        if (it != mPositions.end())
        {
            int i = ui->positionTable->row(it->second);
            int total_qty = ui->positionTable->item(i, 2)->text().toInt();
            total_qty -= d.quantity;

            ui->positionTable->item(i, 2)->setText(QString::number(total_qty));
            ui->positionTable->item(i, 3)->setText(QString::number(d.price, 'f', 2));
            ui->positionTable->item(i, 4)->setText(QString::number(d.price * total_qty, 'f', 2));

            auto &prices = mSellPositionPrices.at(d.stock_code);
            prices.trade_avg_price = d.trade_avg_price;
            ui->positionTable->item(i, 5)->setText(QString::number(prices.trade_avg_price, 'f', 2));
            if (prices.pre_close_price > 0)
                ui->positionTable->item(i, 6)->setText(QString::number((prices.trade_avg_price / prices.pre_close_price - 1) * 100, 'f', 2));
            ui->positionTable->item(i, 7)->setText(QString::number(prices.GetTradeProfit(d.price, total_qty), 'f', 2));
        }
    }
}

void GuiTradeTab::OnOrderTraded(size_t id_, const TradeData &d)
{
    if (id != id_)
    {
        return;
    }

    // 可卖持仓和持仓列表
    if (d.side == XTP_SIDE_BUY)
        add_buy_position(d);
    else
    {
        sub_sell_position(d, true);
        sub_sell_position(d, false);
    }

    // 可撤委托
    auto it = mOrders.find(d.order_xtp_id);
    if (it != mOrders.end())
    {
        int i = ui->orderTable->row(it->second);
        int qty = ui->orderTable->item(i, 3)->text().toInt();
        qty -= d.quantity;
        ui->orderTable->item(i, 3)->setText(QString::number(qty));
        if (qty == 0)
        {
            ui->orderTable->removeRow(i);
            mOrders.erase(it);
        }
    }

    // 当日成交
    ui->orderTradeTable->setSortingEnabled(false);
    int rows = ui->orderTradeTable->rowCount();
    ui->orderTradeTable->setRowCount(rows + 1);
    ui->orderTradeTable->setItem(rows, 0, new QTableWidgetItem(d.trade_time));
    ui->orderTradeTable->setItem(rows, 1, new QTableWidgetItem(d.stock_code));
    auto pQSI = StockStaticInfo::GetInstance().GetQSI(d.stock_code);
    if (pQSI)
    {
        ui->orderTradeTable->setItem(rows, 2, new QTableWidgetItem(pQSI->ticker_name));
    }
    ui->orderTradeTable->setItem(rows, 3, new QTableWidgetItem(d.side == XTP_SIDE_BUY ? QStringLiteral("买入") : QStringLiteral("卖出")));
    ui->orderTradeTable->setItem(rows, 4, new QTableWidgetItem(QString::number(d.quantity)));
    ui->orderTradeTable->setItem(rows, 5, new QTableWidgetItem(QString::number(d.price, 'f', 2)));
    ui->orderTradeTable->setItem(rows, 6, new QTableWidgetItem(QString::number(d.trade_amount, 'f', 2)));
    ui->orderTradeTable->setSortingEnabled(true);
    ui->orderTradeTable->sortItems(0, Qt::DescendingOrder);

    // 当日委托
    if (mInsertOrders.find(d.order_xtp_id) != mInsertOrders.end())
    {
        int i = ui->orderInsertTable->row(mInsertOrders.at(d.order_xtp_id));
        int qty = ui->orderInsertTable->item(i, 5)->text().toInt();
        qty += d.quantity;
        ui->orderInsertTable->item(i, 5)->setText(QString::number(qty));
    }
}

void GuiTradeTab::OnOrderCanceled(size_t id_, const CancelData &d)
{
    if (id != id_)
    {
        return;
    }

    // 可卖持仓
    if (d.side == XTP_SIDE_SELL)
    {
        if (mSellPositions.find(d.stock_code) != mSellPositions.end())
        {
            int i = ui->sellablePositionTable->row(mSellPositions.at(d.stock_code));
            int sellable_qty = ui->sellablePositionTable->item(i, 4)->text().toInt();
            ui->sellablePositionTable->item(i, 4)->setText(QString::number(sellable_qty + d.qty_left));
        }
    }

    // 可撤委托
    auto it = mOrders.find(d.order_xtp_id);
    if (it != mOrders.end())
    {
        int i = ui->orderTable->row(it->second);
        int qty = ui->orderTable->item(i, 3)->text().toInt();
        qty -= d.qty_left;
        ui->orderTable->item(i, 3)->setText(QString::number(qty));
        if (qty == 0)
        {
            ui->orderTable->removeRow(i);
            mOrders.erase(it);
        }
    }

    // 当日委托
    if (mInsertOrders.find(d.order_xtp_id) != mInsertOrders.end())
    {
        int i = ui->orderInsertTable->row(mInsertOrders.at(d.order_xtp_id));
        int qty = ui->orderInsertTable->item(i, 6)->text().toInt();
        qty += d.qty_left;
        ui->orderInsertTable->item(i, 6)->setText(QString::number(qty));
    }
}

void GuiTradeTab::UserSelectPosition(QTableWidgetItem *item)
{
    int row = item->row();
    auto stock_code = ui->sellablePositionTable->item(row, 1)->text();
    auto stock_name = ui->sellablePositionTable->item(row, 2)->text();
    double price = ui->sellablePositionTable->item(row, 5)->data(Qt::UserRole).value<double>();
    UserReqSellPosition(OrderReq{stock_code, stock_name, price});
}

void GuiTradeTab::UserSelectPosition(size_t index) const
{
    for (auto &val : mSellPositions)
    {
        if (val.second->text().toInt() == index)
        {
            int r = ui->sellablePositionTable->row(val.second);
            auto stock_code = ui->sellablePositionTable->item(r, 1)->text();
            auto stock_name = ui->sellablePositionTable->item(r, 2)->text();
            double price = ui->sellablePositionTable->item(r, 5)->data(Qt::UserRole).value<double>();
            UserReqSellPosition(OrderReq{stock_code, stock_name, price});
            return;
        }
    }
}

void GuiTradeTab::UserChooseOrder(bool ascending) const
{
    if (ascending)
    {
        ui->orderTable->sortItems(5, Qt::AscendingOrder);
        ui->orderTradeTable->sortItems(0, Qt::AscendingOrder);
        ui->orderInsertTable->sortItems(0, Qt::AscendingOrder);
    }
    else
    {
        ui->orderTable->sortItems(5, Qt::DescendingOrder);
        ui->orderTradeTable->sortItems(0, Qt::DescendingOrder);
        ui->orderInsertTable->sortItems(0, Qt::DescendingOrder);
    }
}

void GuiTradeTab::req_cancel_orders() const
{
    vector<uint64_t> orderIDs;
    QString text{QStringLiteral("是否要撤销以下股票的卖单：\n")};
    for (auto &it : mOrders)
    {
        int i = ui->orderTable->row(it.second);
        if (qobject_cast<GuiSelectOrder *>(ui->orderTable->cellWidget(i, 0))->IsSelected())
        {
            orderIDs.push_back(it.first);
            text += ui->orderTable->item(i, 1)->text();
            text += "\n";
        }
    }

    if (orderIDs.size() == 0)
    {
        return;
    }

    if (QMessageBox::information(nullptr, "Cancel Orders", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        for (auto order_id : orderIDs)
        {
            ReqCancelOrder(id, order_id);
        }
    }
}