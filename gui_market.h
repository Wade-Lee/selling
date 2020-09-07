#ifndef GUI_MARKET_H
#define GUI_MARKET_H

#include <vector>
#include <QWidget>
#include <QLabel>

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

public:
	explicit GuiMarket(QWidget *parent = nullptr);
	~GuiMarket();

private:
	Ui::GuiMarket *ui;

	size_t quote_stall_size;
	StockCode current_stock_code;
	std::vector<QLabel *> ask_prices;
	std::vector<QLabel *> ask_quantities;
	std::vector<QLabel *> bid_prices;
	std::vector<QLabel *> bid_quantities;
};

#endif // GUI_MARKET_H
