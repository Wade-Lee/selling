#ifndef GUI_MARKET_H
#define GUI_MARKET_H

#include <vector>
#include <QWidget>
#include <QLabel>
#include <unordered_map>

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
	std::unordered_map<StockCode, MarketData> mQuotes;

	StockCode current_stock_code;
	double pre_close_price = 0.0;
	std::vector<QLabel *> ask_prices;
	std::vector<QLabel *> ask_quantities;
	std::vector<QLabel *> bid_prices;
	std::vector<QLabel *> bid_quantities;

	void set_label_style_sheet(QLabel *, double) const;
	void update_quotes(const MarketData &);
};

#endif // GUI_MARKET_H
