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
		if (mQuotes.find(stock_code) != mQuotes.end())
			update_quotes(mQuotes.at(stock_code));
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

void GuiMarket::OnMarketDataReceived(const MarketData &d)
{
	if (mQuotes.find(d.stock_code) == mQuotes.end())
		mQuotes[d.stock_code] = d;
	else
		mQuotes.at(d.stock_code) = d;

	if (current_stock_code != d.stock_code)
	{
		return;
	}

	update_quotes(d);
}

void GuiMarket::set_label_style_sheet(QLabel *pLabel, double price) const
{
	if (fabs(price - pre_close_price) < numeric_limits<double>::epsilon())
		pLabel->setStyleSheet("color: gray;");
	else if (price > pre_close_price)
		pLabel->setStyleSheet("color: red;");
	else
		pLabel->setStyleSheet("color: blue;");
}

void GuiMarket::update_quotes(const MarketData &d)
{
	pre_close_price = mQuotes.at(d.stock_code).pre_close_price;
	ui->lastPrice->setText(QString::number(d.last_price, 'f', 2));
	set_label_style_sheet(ui->lastPrice, d.last_price);
	double pct = (d.last_price - d.pre_close_price) / d.pre_close_price;
	ui->lastChangePct->setText(QString::number(pct * 100, 'f', 2) + "%");
	if (fabs(pct) < numeric_limits<double>::epsilon())
		ui->lastChangePct->setStyleSheet("color: gray;");
	else if (pct > 0)
		ui->lastChangePct->setStyleSheet("color: red;");
	else
		ui->lastChangePct->setStyleSheet("color: blue;");

	for (size_t i = 0; i < QuoteStallSize; i++)
	{
		ask_prices[i]->setText(QString::number(d.ask_price[i], 'f', 2));
		set_label_style_sheet(ask_prices[i], d.ask_price[i]);
		ask_quantities[i]->setText(QString::number(d.ask_qty[i] / 100));
		bid_prices[i]->setText(QString::number(d.bid_price[i], 'f', 2));
		set_label_style_sheet(bid_prices[i], d.bid_price[i]);
		bid_quantities[i]->setText(QString::number(d.bid_qty[i] / 100));
	}
}