#include "gui_sell.h"
#include "ui_gui_sell.h"

#include <QAbstractItemView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>

using namespace std;
using namespace HuiFu;

size_t GuiSell::ID = 0;

GuiSell::GuiSell(QWidget *parent) : QWidget(parent),
									ui(new Ui::GuiSell),
									id(GuiSell::ID++),
									current_stock_code("")
{
	ui->setupUi(this);

	ui->stockCode->setInputMask("999999;_");
	init_sell_qty();

	Activate(true);

	ui->stockCode->installEventFilter(this);
	ui->sellPrice->installEventFilter(this);
	ui->sellQty->installEventFilter(this);
}

GuiSell::~GuiSell()
{
	delete ui;
}

void GuiSell::Activate(bool a)
{
	activated = a;
	setEnabled(a);
}

void GuiSell::SetFocus()
{
	Activate(true);
	ui->stockCode->setFocus();
	ui->stockCode->selectAll();
}

#pragma region 股票代码和股票名称
void GuiSell::UserEditStockCode(QString text)
{
	SellReqSyncStockCode(id, text);
}

void GuiSell::req_stock_info()
{
	current_stock_code = ui->stockCode->text();

	XTPQSI *pQSI = StockStaticInfo::GetInstance().GetQSI(current_stock_code);
	if (!pQSI)
	{
		SetStockName(QStringLiteral("找不到对应的股票名称"));
		return;
	}

	SetStockName(pQSI->ticker_name);
	SetSellableQty();

	ui->sellPrice->setFocus();
	ui->sellPrice->selectAll();

	MarketReqSubscribe(pQSI->exchange_id, current_stock_code);
	SellReqPosition(id, current_stock_code);
	SellReqSyncStockInfo(id, current_stock_code, pQSI->ticker_name);
}

void GuiSell::OnUserReqSellPosition(const OrderReq &d)
{
	ui->sellPrice->setFocus();
	ui->sellPrice->selectAll();

	SetStockCode(d.stock_code);
	SetStockName(d.stock_name);
	SetSellPrice(d.price);
	SetSellableQty(d.quantity);

	current_stock_code = ui->stockCode->text();
	XTPQSI *pQSI = StockStaticInfo::GetInstance().GetQSI(current_stock_code);
	MarketReqSubscribe(pQSI->exchange_id, current_stock_code);
	SellReqSyncStockInfo(id, current_stock_code, d.stock_name);
}

void GuiSell::SetStockCode(const QString &text) const { ui->stockCode->setText(text); }
void GuiSell::SetStockName(const QString &text) const { ui->stockName->setText(text); }
#pragma endregion

#pragma region 卖出价格
void GuiSell::UserEditSellPrice(double price)
{
	SellReqSyncStockPrice(id, price);
}

void GuiSell::user_enter_price()
{
	ui->sellQty->setFocus();
	ui->sellQty->setCurrentIndex(0);
}

void GuiSell::SetSellPrice(double price) const
{
	ui->sellPrice->setValue(price);
}
#pragma endregion

#pragma region 可卖股数和卖出数量
void GuiSell::init_sell_qty()
{
	auto lens = ui->sellQty->count();
	for (int i = 0; i < lens; i++)
	{
		ui->sellQty->setItemData(i, ui->sellQty->itemText(i), Qt::UserRole); //显示，固定不可变
		ui->sellQty->setItemData(i, 0, Qt::UserRole + 1);					 //将要卖出的数量，可变
	}
	// 设置有效值范围
	QIntValidator *ival = new QIntValidator(this);
	ival->setRange(100, 10000000000);
	ui->sellQty->lineEdit()->setValidator(ival);
	// 取消自动补全
	ui->sellQty->setCompleter(0);

	SetSellableQty();
}

void GuiSell::UserSelectSellQty(int index)
{
	switch (index)
	{
	case 0:
		SellReqSyncSellQty(id, index);
		break;
	case 7:
		input_sell_qty();
		break;
	default:
		ui->sellQty->lineEdit()->setText(QString::number(ui->sellQty->currentData(Qt::UserRole + 1).value<long>()));
		SellReqSyncSellQty(id, index);
		break;
	}
}

void GuiSell::UserEditSellQty(QString text)
{
	if (ui->sellQty->hasFocus())
	{
		SellReqSyncSellQtyText(id, text);
	}
}

void GuiSell::UserReqSellAllQty()
{
	SetSellQty(1);
	SellReqSyncSellQty(id, 1);
}

void GuiSell::OnPositionReceived(size_t id_, const QString &stock_code, long sellable_qty)
{
	if (id == id_)
	{
		current_stock_code = stock_code;
		SetSellableQty(sellable_qty);
	}
}

void GuiSell::OnOrderReceived(size_t id_, const OrderData &d)
{
	if (id == id_ && d.side == XTP_SIDE_SELL && current_stock_code == d.stock_code)
	{
		int sellable_qty = ui->sellableQty->text().toInt();
		SetSellableQty(sellable_qty - d.quantity);
	}
}

void GuiSell::OnOrderCanceled(size_t id_, const CancelData &d)
{
	if (id == id_ && d.side == XTP_SIDE_SELL && current_stock_code == d.stock_code)
	{
		int sellable_qty = ui->sellableQty->text().toInt();
		SetSellableQty(sellable_qty + d.qty_left);
	}
}

// TODO：
void GuiSell::update_sell_qty(long sellable_qty) const
{
	if (sellable_qty == 0)
	{
		auto lens = ui->sellQty->count();
		for (int i = 0; i < lens; i++)
		{
			ui->sellQty->setItemText(i, ui->sellQty->itemData(i, Qt::UserRole).value<QString>());
			ui->sellQty->setItemData(i, 0, Qt::UserRole + 1);
		}
		return;
	}

	static int denos[] = {1, 2, 3, 4, 5, 10};
	auto lens = sizeof(denos) / sizeof(denos[0]);
	for (auto i = 1; i <= lens; i++)
	{
		long qty = (sellable_qty / denos[i - 1]) / 100 * 100;
		ui->sellQty->setItemText(i, QString("%1 %2").arg(ui->sellQty->itemData(i, Qt::UserRole).value<QString>()).arg(qty));
		ui->sellQty->setItemData(i, qty, Qt::UserRole + 1);
	}
}

void GuiSell::input_sell_qty()
{
	bool ok = false;
	int deno = QInputDialog::getInt(this,
									"QInputDialog_SellQty",
									QStringLiteral("请输入卖出股份的分母"),
									1, 1, 100, 1, &ok);

	if (ok)
	{
		int sellable_qty = ui->sellableQty->text().toInt();
		long qty = (sellable_qty / deno) / 100 * 100;
		ui->sellQty->lineEdit()->setText(QString::number(qty));
	}
}

void GuiSell::SetSellableQty(long sellable_qty) const
{
	ui->sellableQty->setText(QString::number(sellable_qty));
	update_sell_qty(sellable_qty);
	ui->sellQty->setCurrentIndex(0);
}

void GuiSell::SetSellQty(int index) const
{
	ui->sellQty->setCurrentIndex(index);
	ui->sellQty->lineEdit()->setText(QString::number(ui->sellQty->currentData(Qt::UserRole + 1).value<long>()));
}

void GuiSell::SetSellQty(const QString &text) const
{
	ui->sellQty->setCurrentIndex(0);
	ui->sellQty->lineEdit()->setText(text);
}

bool GuiSell::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() >= Qt::Key_A && keyEvent->key() <= Qt::Key_Z)
		{
			if (keyEvent->key() == Qt::Key_Period && watched == ui->sellPrice)
			{
				return false;
			}

			keyEvent->ignore();
			return true;
		}
		else if (keyEvent->key() == Qt::Key_Return)
		{
			if (watched == ui->stockCode)
				req_stock_info();
			else if (watched == ui->sellPrice)
				user_enter_price();
			else if (watched == ui->sellQty)
				req_selling();
			return true;
		}
		else if (keyEvent->key() == Qt::Key_Space)
		{
			if (watched == ui->sellQty)
			{
				if (!ui->sellQty->view()->isVisible())
					ui->sellQty->showPopup();
			}
			return true;
		}
		return false;
	}
	else
		return false;
}
#pragma endregion

#pragma region 卖出和重置
void GuiSell::req_selling()
{
	double price = ui->sellPrice->value();
	long quantity;
	switch (ui->sellQty->currentIndex())
	{
	case 0:
	case 7:
		quantity = ui->sellQty->currentText().toLong();
		break;
	default:
		quantity = ui->sellQty->currentData(Qt::UserRole + 1).value<long>();
		break;
	}

	QString stock_code = ui->stockCode->text();

	auto pQSI = StockStaticInfo::GetInstance().GetQSI(stock_code);
	if (!pQSI)
	{
		QString text{QStringLiteral("找不到股票代码：")};
		text += stock_code;
		QMessageBox::warning(nullptr, "Warning", text);
		UserClear();
		ui->stockCode->setFocus();
		ui->stockCode->selectAll();
		return;
	}

	if (price < pQSI->lower_limit_price)
	{
		QString text = stock_code;
		text += QStringLiteral("卖出价格低于跌停价：");
		text += QString::number(pQSI->lower_limit_price, 'f', 2);
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellPrice->setFocus();
		ui->sellPrice->selectAll();
		return;
	}

	if (price > pQSI->upper_limit_price)
	{
		QString text = stock_code;
		text += QStringLiteral("卖出价格高于涨停价：");
		text += QString::number(pQSI->upper_limit_price, 'f', 2);
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellPrice->setFocus();
		ui->sellPrice->selectAll();
		return;
	}

	if (quantity == 0)
	{
		QString text = stock_code;
		text += QStringLiteral("卖出数量为0");
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellQty->setCurrentIndex(0);
		return;
	}

	int sellable_qty = ui->sellableQty->text().toInt();
	if (quantity > sellable_qty)
	{
		QString text = stock_code;
		text += QStringLiteral("卖出数量高于可卖数量：");
		text += QString::number(sellable_qty);
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellQty->setCurrentIndex(0);
		return;
	}

	QString text{QStringLiteral("是否卖出：\n")};
	text += ui->stockName->text();
	text += "\n";
	text += QStringLiteral("价格：");
	text += QString::number(price, 'f', 2);
	text += "\n";
	text += QStringLiteral("数量：");
	text += QString::number(quantity);

	if (QMessageBox::information(nullptr, "Req Selling", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
	{
		SellReqSelling(id, stock_code, price, quantity);
		if (ui->sellBtn->hasFocus())
		{
			// ReqSellStock(GetID());
		}
	}
}

void GuiSell::UserReqSelling()
{
	req_selling();
}

void GuiSell::UserClear()
{
	SetStockCode("");
	SetStockName("");
	SetSellPrice(0.0);
	SetSellableQty();
}
#pragma endregion