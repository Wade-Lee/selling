﻿#ifndef GUI_MARKET_H
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

	StockCode current_stock_code;
	double pre_close_price = 0.0;
	std::vector<QLabel *> ask_prices;
	std::vector<QLabel *> ask_quantities;
	std::vector<QLabel *> bid_prices;
	std::vector<QLabel *> bid_quantities;

	void set_label_style_sheet(QLabel *, double) const;
};

#endif // GUI_MARKET_H
