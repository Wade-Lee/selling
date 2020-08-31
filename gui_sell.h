#ifndef GUI_SELL_H
#define GUI_SELL_H

#include "base.h"

#include <QWidget>
#include <QKeyEvent>

namespace Ui
{
	class GuiSell;
}

class GuiSell : public QWidget
{
	Q_OBJECT

#pragma region 股票代码和股票名称
public slots:
	void UserEditStockCode(QString);
	void UserReqStockInfo();
	void OnUserSelectPosition(const HuiFu::PositionData &, double);

signals:
	void MarketReqSubscribe(int, const QString &) const;
	void SellReqSyncStockCode(size_t, const QString &) const;
	void SellReqSyncStockInfo(size_t, const QString &, const QString &) const;

public:
	void SetStockCode(const QString &text) const;
	void SetStockName(const QString &text) const;
#pragma endregion

protected:
	void keyReleaseEvent(QKeyEvent *);
	void keyPressEvent(QKeyEvent *);

#pragma region 卖出价格
public slots:
	void UserEditSellPrice(double);
	void OnUserSelectPrice(double);

signals:
	void SellReqSyncStockPrice(size_t, double) const;

public:
	void SetSellPrice(double price) const;
#pragma endregion

#pragma region 可卖股数和卖出数量
public slots:
	void UserSelectSellQty(int);
	void UserEditSellQty(QString);
	void UserReqSellAllQty();
	void OnPositionReceived(size_t, const QString &, long);

signals:
	void SellReqPosition(size_t, const QString &) const;
	void SellReqSyncSellQty(size_t, int) const;
	void SellReqSyncSellQtyText(size_t, const QString &) const;

private:
	void init_sell_qty();
	void update_sell_qty(long) const;
	void input_sell_qty();

public:
	// 设置可卖股数及清零卖出数量
	void SetSellableQty(long sellable_qty = 0) const;
	// 按序号同步另一个账户设置卖出数量
	void SetSellQty(int index) const;
	// 按填入值同步另一个账户设置卖出数量
	void SetSellQty(const QString &text) const;
#pragma endregion

#pragma region 卖出和重置
public slots:
	void UserReqSelling();
	void UserClear();

signals:
	void SellReqSelling(size_t, const QString &, double, int64_t) const;

private:
	void req_selling() const;
#pragma endregion

	// void ReqInsertOrder(const QString &, double, long, bool) const;
	// void SellPriceEdited(int, double) const;
	// void SellQtySelected(int, int) const;
	// void ReqSellStock(int) const;

	// void UpdatePosition(const QString &, const QString &, long, long) const;
	// void OnStockCodeEdited(const QString &) const;
	// void OnSellPriceEdited(double) const;
	// void OnSellQtyActivated(int) const;
	// void OnSellQtyEditTextChanged(const QString &text);

#pragma region 属性成员
private:
	// 对应账户id
	static size_t ID;
	size_t id;
	HuiFu::StockCode current_stock_code;

	// 是否被激活
	bool activated = false;

public:
	size_t GetID() const { return id; }
	const HuiFu::StockCode &GetCurrentStockCode() const { return current_stock_code; }

	void SetActivate(bool a)
	{
		activated = a;
		if (!activated)
		{
			activated = (id == 0);
		}

		qInfo() << activated;
	}
	bool IsActivated() const { return activated; }
#pragma endregion

private:
	Ui::GuiSell *ui;

public:
	explicit GuiSell(QWidget *parent = nullptr);
	~GuiSell();
};
#endif // GUI_SELL_H