#ifndef GUI_ORDER_H
#define GUI_ORDER_H

#include "base.h"

#include <map>
#include <QWidget>
#include <QTableWidgetItem>

namespace Ui
{
	class GuiOrder;
}

class GuiOrder : public QWidget
{
	Q_OBJECT

public slots:
	void UserReqCancelOrders() const;
	void UserReqCancelAllOrders() const;

	void OnTraderLogin(size_t);
	void OnOrderReceived(size_t, const HuiFu::OrderData &);
	void OnOrderTraded(size_t, const HuiFu::TradeData &);
	void OnOrderCanceled(size_t, const HuiFu::CancelData &);

signals:
	void ReqCancelOrder(size_t, uint64_t) const;

#pragma region 属性成员
private:
	// 对应账户id
	static size_t ID;
	size_t id;

	std::map<uint64_t, QTableWidgetItem *> mOrders;

public:
	int GetID() const { return id; }
#pragma endregion

public:
	explicit GuiOrder(QWidget *parent = nullptr);
	~GuiOrder();

private:
	Ui::GuiOrder *ui;
};

#endif // GUI_ORDER_H
