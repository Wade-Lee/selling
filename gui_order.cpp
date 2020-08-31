#include "gui_order.h"
#include "ui_gui_order.h"

#include "gui_base.h"
#include "gui_selectorder.h"
#include <QSpinBox>
#include <QMessageBox>

using namespace std;
using namespace HuiFu;

size_t GuiOrder::ID = 0;

GuiOrder::GuiOrder(QWidget *parent) : QWidget(parent),
									  ui(new Ui::GuiOrder),
									  id(GuiOrder::ID++),
									  mOrders()
{
	ui->setupUi(this);
}

GuiOrder::~GuiOrder()
{
	delete ui;
}

void GuiOrder::OnTraderLogin(size_t id_)
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

void GuiOrder::OnOrderReceived(size_t id_, const OrderData &d)
{
	if (id != id_)
	{
		return;
	}

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
}

void GuiOrder::OnOrderTraded(size_t id_, const TradeData &d)
{
	if (id != id_)
	{
		return;
	}

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
}

void GuiOrder::OnOrderCanceled(size_t id_, const CancelData &d)
{
	if (id != id_)
	{
		return;
	}

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
}

void GuiOrder::UserReqCancelOrders() const
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

void GuiOrder::UserReqCancelAllOrders() const
{
	QString text{QStringLiteral("是否要撤销以下股票的卖单：\n")};
	int rows = ui->orderTable->rowCount();
	for (int i = 0; i < rows; i++)
	{
		text += ui->orderTable->item(i, 1)->text();
		text += "\n";
	}

	if (QMessageBox::information(nullptr, "Cancel Orders", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
	{
		for (auto it : mOrders)
		{
			int i = ui->orderTable->row(it.second);
			qobject_cast<GuiSelectOrder *>(ui->orderTable->cellWidget(i, 0))->SetSelect(true);
			ReqCancelOrder(id, it.first);
		}
	}
}
