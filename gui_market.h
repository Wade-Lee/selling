#ifndef GUI_MARKET_H
#define GUI_MARKET_H

#include <vector>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>

#include "base.h"

using namespace HuiFu;

namespace Ui
{
	class GuiMarket;
}

class GuiMarket : public QWidget
{
	Q_OBJECT

public slots:
	void OnSubscribed(const QString &stock_code = "", bool subbed = false);
	void OnMarketDataReceived(const MarketData &);

	void UserReqUpper();
	void UserReqLower();
	void UserReqAsk1();
	void UserReqAsk2();
	void UserReqAsk3();
	void UserReqAsk4();
	void UserReqAsk5();
	void UserReqBid1();
	void UserReqBid2();
	void UserReqBid3();
	void UserReqBid4();
	void UserReqBid5();
	void UserReqLast();

signals:
	void UserSelectPrice(double);

public:
	explicit GuiMarket(QWidget *parent = nullptr);
	~GuiMarket();

private:
	Ui::GuiMarket *ui;

	size_t quote_stall_size;
	StockCode current_stock_code;
	std::vector<QPushButton *> ask_prices;
	std::vector<QLabel *> ask_quantities;
	std::vector<QPushButton *> bid_prices;
	std::vector<QLabel *> bid_quantities;
};

#endif // GUI_MARKET_H
