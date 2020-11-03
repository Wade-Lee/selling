#ifndef GUI_SELL_H
#define GUI_SELL_H

#include "base.h"

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <map>

namespace Ui
{
	class GuiSell;
}

class SellSpinBox : public QSpinBox
{
	Q_OBJECT

public:
	explicit SellSpinBox(QWidget *parent = nullptr);
	void OnSpacePressed();
	QComboBox *combo;
};

class SellDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

public:
	explicit SellDoubleSpinBox(QWidget *parent = nullptr);
	void OnSpacePressed();
	QComboBox *combo;
};

class GuiSell : public QWidget
{
	Q_OBJECT

protected:
	bool eventFilter(QObject *, QEvent *) override;

#pragma region 属性成员
private:
	// 对应账户id
	static size_t ID;
	size_t id;
	HuiFu::StockCode current_stock_code;

	// 是否被激活
	bool activated = false;

	// 是否拥有焦点
	bool focused = false;

public:
	size_t GetID() const { return id; }
	const HuiFu::StockCode &GetCurrentStockCode() const { return current_stock_code; }

	void Activate(bool a);
	bool IsActivated() const { return activated; }

	void SetFocus();
	bool HasFocus() const { return focused; }
#pragma endregion

#pragma region 仓位管理
private:
	struct SellPosition
	{
		double price_level2;
		double price_bid1;
		double price_bid2;
		double price_bid3;
		double price_bid5;
		int64_t quantity;
		int64_t sellable_qty;
	};
	std::map<HuiFu::StockCode, SellPosition> mPositions;

public slots:
	void OnPositionReceived(size_t, const HuiFu::PositionData &);
	void OnMarketDataReceived(const HuiFu::MarketData &);
	void OnUserReqSellPosition(const HuiFu::OrderReq &);
#pragma endregion

#pragma region 股票代码和股票名称
public slots:
	void UserEditStockCode(QString);

signals:
	void SellReqSyncStockCode(size_t, const QString &) const;
	void MarketReqSubscribe(int, const QString &) const;

public:
	void SetStockCode(const QString &text);
	void SetStockName(const QString &text) const;
#pragma endregion

#pragma region 卖出价格
public slots:
	void UserEditSellPrice(double);
	void UserSelectPrice(int);

signals:
	void SellReqSyncStockPrice(size_t, double) const;

public:
	void SetSellPrice(double price) const;
	double GetSellPrice() const;
#pragma endregion

#pragma region 可卖股数和卖出数量
public slots:
	void UserEditSellQty(int);
	void UserSelectSellQty(int);
	void OnOrderReceived(size_t, const HuiFu::OrderData &);
	void OnOrderTraded(size_t, const HuiFu::TradeData &);
	void OnOrderCanceled(size_t, const HuiFu::CancelData &);

public:
	// 设置可卖股数及清零卖出数量
	void SetSellableQty(long sellable_qty = 0) const;
	// 按序号设置卖出数量
	void SetSellQty(int index) const;
	// 按分母设置卖出数量
	void SetSellQtyDeno(int deno) const;
	// 同步另一个账户的卖出数量
	void SetSellQty(int64_t, int64_t) const;
	int GetSellQty() const;

private:
	void update_sell_qty(long) const;
	void input_sell_qty_deno();
#pragma endregion

#pragma region 卖出和重置
private:
	void UserReqSelling();
	void UserClear();

public slots:
	void OnOrderError(size_t, const QString &, const QString &, int32_t);
	void OnOrderRefused(size_t, const QString &);

signals:
	void SellReqSelling(size_t, const QString &, const QString &, double, int64_t) const;
#pragma endregion

#pragma region 关联账户
signals:
	void SellReqSyncStockInfo(size_t, const HuiFu::StockSellInfo &) const;

public:
	void SyncStockInfo(const HuiFu::StockSellInfo &);

private:
	void req_stock_info();
#pragma endregion

private:
	Ui::GuiSell *ui;

public:
	explicit GuiSell(QWidget *parent = nullptr);
	~GuiSell();
};
#endif // GUI_SELL_H