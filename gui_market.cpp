#include "gui_market.h"
#include "ui_gui_market.h"

using namespace std;

size_t QuoteStallSize = 5;

GuiMarket::GuiMarket(QWidget *parent) : QWidget(parent),
										current_stock_code(""),
										ui(new Ui::GuiMarket)
{
	ui->setupUi(this);

	ask_prices = {ui->ask_p1, ui->ask_p2, ui->ask_p3, ui->ask_p4, ui->ask_p5};
	ask_quantities = {ui->ask_q1, ui->ask_q2, ui->ask_q3, ui->ask_q4, ui->ask_q5};
	bid_prices = {ui->bid_p1, ui->bid_p2, ui->bid_p3, ui->bid_p4, ui->bid_p5};
	bid_quantities = {ui->bid_q1, ui->bid_q2, ui->bid_q3, ui->bid_q4, ui->bid_q5};
}

GuiMarket::~GuiMarket()
{
	ask_prices.clear();
	ask_quantities.clear();
	bid_prices.clear();
	bid_quantities.clear();
	delete ui;
}

void GuiMarket::OnSubscribed(const QString &stock_code, bool subbed)
{
	if (subbed)
	{
		current_stock_code = stock_code;
		ui->stockCode->setText(current_stock_code);
		auto pQSI = StockStaticInfo::GetInstance().GetQSI(current_stock_code);
		if (pQSI)
		{
			ui->stockName->setText(pQSI->ticker_name);
			ui->lowerBoundPrice->setText(QString::number(pQSI->lower_limit_price, 'f', 2));
			ui->upperBoundPrice->setText(QString::number(pQSI->upper_limit_price, 'f', 2));
		}
	}
	else
	{
		current_stock_code = "";
		ui->stockCode->setText("");
		ui->stockName->setText("");
		ui->lowerBoundPrice->setText("");
		ui->upperBoundPrice->setText("");
	}
}

void GuiMarket::OnMarketDataReceived(const MarketData &data)
{
	if (current_stock_code != data.stock_code)
	{
		return;
	}

	for (size_t i = 0; i < QuoteStallSize; i++)
	{
		ask_prices[i]->setText(QString::number(data.ask_price[i], 'f', 2));
		ask_quantities[i]->setText(QString::number(data.ask_qty[i] / 100));
		bid_prices[i]->setText(QString::number(data.bid_price[i], 'f', 2));
		bid_quantities[i]->setText(QString::number(data.bid_qty[i] / 100));
		ui->lastPrice->setText(QString::number(data.last_price, 'f', 2));
	}
}