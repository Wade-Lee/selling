#include "gui_sell.h"
#include "ui_gui_sell.h"

#include <QAbstractItemView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>

using namespace std;
using namespace HuiFu;

size_t GuiSell::ID = 0;

SellSpinBox::SellSpinBox(QWidget *parent) : QSpinBox(parent)
{
	combo = new QComboBox(this);
	combo->addItem(QStringLiteral("① 全仓"));
	combo->addItem(QStringLiteral("② 1/2"));
	combo->addItem(QStringLiteral("③ 1/3"));
	combo->addItem(QStringLiteral("④ 1/4"));
	combo->addItem(QStringLiteral("⑤ 1/5"));
	combo->addItem(QStringLiteral("⑥ 1/10"));
	combo->addItem(QStringLiteral("⑦ 自定义分母"));

	auto lens = combo->count() - 1;
	for (int i = 0; i < lens; i++)
	{
		combo->setItemData(i, combo->itemText(i), Qt::UserRole); //显示，固定不可变
		combo->setItemData(i, 0, Qt::UserRole + 1);				 //将要卖出的数量，可变
	}

	combo->setObjectName(QString::fromUtf8("sellQtyCombo"));
	combo->setEditable(false);
	combo->setInsertPolicy(QComboBox::NoInsert);

	combo->hide();
}

void SellSpinBox::OnSpacePressed()
{
	combo->raise();
	combo->setCurrentIndex(3);
	combo->showPopup();
}

SellDoubleSpinBox::SellDoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
	combo = new QComboBox(this);
	combo->addItem(QStringLiteral("① 买一"));
	combo->addItem(QStringLiteral("② 买二"));
	combo->addItem(QStringLiteral("③ 买三"));
	combo->addItem(QStringLiteral("④ 买五"));

	combo->setObjectName(QString::fromUtf8("sellPriceCombo"));
	combo->setEditable(false);
	combo->setInsertPolicy(QComboBox::NoInsert);

	combo->hide();
}

void SellDoubleSpinBox::OnSpacePressed()
{
	combo->raise();
	combo->setCurrentIndex(0);
	combo->showPopup();
}

GuiSell::GuiSell(QWidget *parent) : QWidget(parent), ui(new Ui::GuiSell), id(GuiSell::ID++), current_stock_code("")
{
	ui->setupUi(this);

	ui->stockCode->setInputMask("999999;_");
	QObject::connect(ui->sellPrice->combo, SIGNAL(activated(int)), this, SLOT(UserSelectPrice(int)));
	QObject::connect(ui->sellQty->combo, SIGNAL(activated(int)), this, SLOT(UserSelectSellQty(int)));
	SetSellableQty();

	Activate(true);

	ui->stockCode->installEventFilter(this);
	ui->sellPrice->installEventFilter(this);
	ui->sellQty->installEventFilter(this);
	ui->sellBtn->installEventFilter(this);
}

GuiSell::~GuiSell()
{
	delete ui;
}

bool GuiSell::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		int k = keyEvent->key();
		if (k >= Qt::Key_A && k <= Qt::Key_Z)
		{
			if (k == Qt::Key_Period && watched == ui->sellPrice)
			{
				return false;
			}

			keyEvent->ignore();
			return true;
		}
		else if (k == Qt::Key_Return)
		{
			if (watched == ui->stockCode)
			{
				ui->sellPrice->setFocus();
				ui->sellPrice->selectAll();
			}
			else if (watched == ui->sellPrice)
			{
				ui->sellQty->setFocus();
				ui->sellQty->selectAll();
			}
			else if (watched == ui->sellQty || watched == ui->sellBtn)
				UserReqSelling();
			return true;
		}
		else if (k == Qt::Key_Space)
		{
			if (watched == ui->sellQty)
			{
				ui->sellQty->OnSpacePressed();
			}
			else if (watched == ui->sellPrice)
			{
				ui->sellPrice->OnSpacePressed();
			}
			return true;
		}
		else if (k == Qt::Key_Tab)
		{
			if (watched == ui->sellBtn)
			{
				ui->stockCode->setFocus();
				ui->stockCode->selectAll();
				return true;
			}
		}

		return false;
	}
	else if (event->type() == QEvent::FocusOut)
	{
		focused = false;
		if (watched == ui->stockCode || watched == ui->sellPrice)
			req_stock_info();
		return false;
	}
	else if (event->type() == QEvent::FocusIn)
	{
		focused = true;
		if (watched == ui->stockCode)
		{
			ui->stockCode->setFocus();
			ui->stockCode->selectAll();
			return true;
		}
		return false;
	}
	else
		return false;
}

#pragma region 属性成员
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
#pragma endregion

#pragma region 仓位管理
void GuiSell::OnPositionReceived(size_t id_, const PositionData &d)
{
	if (id != id_)
	{
		return;
	}

	mPositions[d.stock_code] = SellPosition{0, 0, 0, 0, 0, d.total_qty, d.sellable_qty};
}

void GuiSell::OnMarketDataReceived(const MarketData &d)
{
	if (mPositions.find(d.stock_code) == mPositions.end())
	{
		return;
	}
	auto &position = mPositions.at(d.stock_code);
	position.price_bid1 = d.bid_price[0];
	position.price_bid2 = d.bid_price[1];
	position.price_bid3 = d.bid_price[2];
	position.price_bid5 = d.bid_price[4];
}

void GuiSell::OnUserReqSellPosition(const OrderReq &d)
{
	if (mPositions.find(d.stock_code) != mPositions.end())
	{
		SetStockCode(d.stock_code);
		SetStockName(d.stock_name);
		SetSellPrice(d.price);
		SetSellableQty(mPositions.at(d.stock_code).sellable_qty);
	}

	ui->sellPrice->setFocus();
	ui->sellPrice->selectAll();

	req_stock_info();
}
#pragma endregion

#pragma region 股票代码和股票名称
void GuiSell::UserEditStockCode(QString text)
{
	current_stock_code = text;
	SellReqSyncStockCode(id, text);
}

void GuiSell::SetStockCode(const QString &text)
{
	current_stock_code = text;
	ui->stockCode->setText(text);
}

void GuiSell::SetStockName(const QString &text) const { ui->stockName->setText(text); }
#pragma endregion

#pragma region 卖出价格
void GuiSell::UserEditSellPrice(double price)
{
	if (!ui->sellPrice->hasFocus())
	{
		return;
	}

	if (mPositions.find(current_stock_code) != mPositions.end())
		SellReqSyncStockPrice(id, price);
}

void GuiSell::UserSelectPrice(int index)
{
	if (mPositions.find(current_stock_code) == mPositions.end())
	{
		ui->sellPrice->combo->hide();
		return;
	}
	auto &position = mPositions.at(current_stock_code);

	double price = 0;
	switch (index)
	{
	case 0:
		price = position.price_bid1;
		break;
	case 1:
		price = position.price_bid2;
		break;
	case 2:
		price = position.price_bid3;
		break;
	case 3:
		price = position.price_bid5;
		break;
	default:
		break;
	}
	SetSellPrice(price);
	ui->sellPrice->combo->hide();

	if (mPositions.find(current_stock_code) != mPositions.end())
		SellReqSyncStockPrice(id, price);
}

void GuiSell::SetSellPrice(double price) const
{
	if (mPositions.find(current_stock_code) != mPositions.end())
		ui->sellPrice->setValue(price);
	else
		ui->sellPrice->setValue(0);
}

double GuiSell::GetSellPrice() const
{
	return ui->sellPrice->value();
}
#pragma endregion

#pragma region 可卖股数和卖出数量
void GuiSell::UserReqSellAllQty()
{
	SetSellQty(0);
	req_stock_info();
}

void GuiSell::UserEditSellQty(int qty)
{
	if (!ui->sellQty->hasFocus())
	{
		return;
	}

	req_stock_info();
}

void GuiSell::UserSelectSellQty(int index)
{
	switch (index)
	{
	case 6:
		input_sell_qty_deno();
		break;
	default:
		SetSellQty(index);
		break;
	}
	ui->sellQty->combo->hide();

	req_stock_info();
}

void GuiSell::OnOrderReceived(size_t id_, const OrderData &d)
{
	if (id == id_ && mPositions.find(d.stock_code) != mPositions.end())
	{
		mPositions.at(d.stock_code).sellable_qty -= d.quantity;
		if (current_stock_code == d.stock_code)
			SetSellableQty(mPositions.at(d.stock_code).sellable_qty);
	}
}

void GuiSell::OnOrderTraded(size_t id_, const TradeData &d)
{
	if (id == id_ && mPositions.find(d.stock_code) != mPositions.end())
	{
		auto &position = mPositions.at(d.stock_code);
		position.quantity -= d.quantity;
	}
}

void GuiSell::OnOrderCanceled(size_t id_, const CancelData &d)
{
	if (id == id_ && mPositions.find(d.stock_code) != mPositions.end())
	{
		mPositions.at(d.stock_code).sellable_qty += d.qty_left;
		if (current_stock_code == d.stock_code)
			SetSellableQty(mPositions.at(d.stock_code).sellable_qty);
	}
}

void GuiSell::SetSellableQty(long sellable_qty) const
{
	ui->sellableQty->setText(QString::number(sellable_qty));
	update_sell_qty(sellable_qty);
}

void GuiSell::SetSellQty(int index) const
{
	ui->sellQty->setValue(ui->sellQty->combo->currentData(Qt::UserRole + 1).value<long>());
}

void GuiSell::SetSellQty(int64_t sell_qty, int64_t total_qty) const
{
	if (sell_qty == 0 || total_qty == 0)
	{
		return;
	}

	if (mPositions.find(current_stock_code) != mPositions.end())
	{
		auto &position = mPositions.at(current_stock_code);
		if (sell_qty == total_qty)
		{
			ui->sellQty->setValue(position.sellable_qty);
		}
		else
		{
			int64_t qty = ceil(sell_qty * position.quantity / total_qty / 100.0) * 100;
			qty = min(position.sellable_qty, qty);
			ui->sellQty->setValue(qty);
		}
	}
	else
		ui->sellQty->setValue(0);
}

int GuiSell::GetSellQty() const
{
	return ui->sellQty->value();
}

void GuiSell::SetSellQtyDeno(int deno) const
{
	if (mPositions.find(current_stock_code) != mPositions.end())
	{
		auto &position = mPositions.at(current_stock_code);
		int64_t qty = (position.sellable_qty / deno) / 100 * 100;
		ui->sellQty->setValue(qty);
	}
}

void GuiSell::update_sell_qty(long sellable_qty) const
{
	if (sellable_qty == 0)
	{
		auto lens = ui->sellQty->combo->count() - 1;
		for (int i = 0; i < lens; i++)
		{
			ui->sellQty->combo->setItemText(i, ui->sellQty->combo->itemData(i, Qt::UserRole).value<QString>());
			ui->sellQty->combo->setItemData(i, 0, Qt::UserRole + 1);
		}
		return;
	}

	static int denos[] = {1, 2, 3, 4, 5, 10};
	auto lens = sizeof(denos) / sizeof(denos[0]);
	for (auto i = 0; i < lens; i++)
	{
		long qty = (sellable_qty / denos[i]) / 100 * 100;
		ui->sellQty->combo->setItemText(i, QString("%1 %2").arg(ui->sellQty->combo->itemData(i, Qt::UserRole).value<QString>()).arg(qty));
		ui->sellQty->combo->setItemData(i, qty, Qt::UserRole + 1);
	}
}

void GuiSell::input_sell_qty_deno()
{
	bool ok = false;
	int deno = QInputDialog::getInt(this,
									"QInputDialog_SellQty",
									QStringLiteral("请输入卖出股份的分母"),
									1, 1, 100, 1, &ok);

	if (ok)
		SetSellQtyDeno(deno);
}
#pragma endregion

#pragma region 卖出和重置
void GuiSell::UserReqSelling()
{
	if (mPositions.find(current_stock_code) == mPositions.end())
	{
		QString text{QStringLiteral("找不到股票代码：")};
		text += current_stock_code;
		QMessageBox::warning(nullptr, "Warning", text);
		UserClear();
		ui->stockCode->setFocus();
		ui->stockCode->selectAll();
		return;
	}

	double price = ui->sellPrice->value();
	long quantity = ui->sellQty->value();

	auto pQSI = StockStaticInfo::GetInstance().GetQSI(current_stock_code);
	if (price < pQSI->lower_limit_price)
	{
		QString text = current_stock_code;
		text += QStringLiteral("卖出价格低于跌停价：");
		text += QString::number(pQSI->lower_limit_price, 'f', 2);
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellPrice->setFocus();
		ui->sellPrice->selectAll();
		return;
	}

	if (price > pQSI->upper_limit_price)
	{
		QString text = current_stock_code;
		text += QStringLiteral("卖出价格高于涨停价：");
		text += QString::number(pQSI->upper_limit_price, 'f', 2);
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellPrice->setFocus();
		ui->sellPrice->selectAll();
		return;
	}

	if (quantity == 0)
	{
		QString text = current_stock_code;
		text += QStringLiteral("卖出数量为0");
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellQty->setValue(0);
		ui->sellQty->selectAll();
		return;
	}

	int sellable_qty = ui->sellableQty->text().toInt();
	if (quantity > sellable_qty)
	{
		QString text = current_stock_code;
		text += QStringLiteral("卖出数量高于可卖数量：");
		text += QString::number(sellable_qty);
		QMessageBox::warning(nullptr, "Warning", text);
		ui->sellQty->setValue(0);
		ui->sellQty->selectAll();
		return;
	}

	SellReqSelling(id, current_stock_code, ui->stockName->text(), price, quantity);
}

void GuiSell::UserClear()
{
	SetStockCode("");
	SetStockName("");
	SetSellPrice(0.0);
	SetSellableQty();
	ui->sellQty->setValue(0);
}

void GuiSell::OnOrderError(size_t id_, const QString &stock_code, const QString &msg, int32_t error_id)
{
	if (id == id_)
	{
		QString text = stock_code;
		text += QStringLiteral("报单错误：");
		text += msg;
		text += QStringLiteral("错误编码：");
		text += QString::number(error_id);
		QMessageBox::warning(nullptr, "Order Error", text);
	}
}

void GuiSell::OnOrderRefused(size_t id_, const QString &stock_code)
{
	if (id == id_)
	{
		QMessageBox::warning(nullptr, "Order Error", stock_code);
	}
}
#pragma endregion

#pragma region 关联账户
void GuiSell::SyncStockInfo(const StockSellInfo &stock_info)
{
	if (mPositions.find(stock_info.stock_code) != mPositions.end())
	{
		SetStockCode(stock_info.stock_code);
		SetStockName(stock_info.stock_name);
		SetSellPrice(stock_info.price);
		SetSellableQty(mPositions.at(stock_info.stock_code).sellable_qty);
		SetSellQty(stock_info.sell_qty, stock_info.total_qty);
	}
	else
		UserClear();
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

	MarketReqSubscribe(pQSI->exchange_id, current_stock_code);

	StockSellInfo info;
	info.stock_code = current_stock_code;
	info.stock_name = pQSI->ticker_name;
	info.price = GetSellPrice();
	info.sell_qty = GetSellQty();

	if (mPositions.find(current_stock_code) != mPositions.end())
	{
		SetSellableQty(mPositions.at(current_stock_code).sellable_qty);
		// 全量卖出时特殊处理
		if (info.sell_qty == mPositions.at(current_stock_code).sellable_qty)
			info.total_qty = info.sell_qty;
		else
			info.total_qty = mPositions.at(current_stock_code).quantity;
	}
	else
	{
		SetSellableQty();
		info.total_qty = 0;
	}

	SellReqSyncStockInfo(id, info);
}
#pragma endregion