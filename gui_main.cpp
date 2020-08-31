#include "gui_main.h"
#include "config.h"
#include "./ui_gui_main.h"

#include "gui_market.h"
#include "gui_position.h"
#include "gui_order.h"
#include "gui_order_trade.h"
#include "gui_order_insert.h"
#include "gui_asset.h"

#include <QMessageBox>

using namespace std;

GuiMain::GuiMain(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GuiMain),
      pTrader(make_unique<Trader>()),
      pQuote(make_unique<Quote>()),
      accounts_connected(true)
{
    ui->setupUi(this);

    gui_sells = ui->Middle->findChildren<GuiSell *>();

    nAccounts = Config::get_instance().get_account_num();

    //初始化行情连接
    pQuote->login();

    register_bind();
}

GuiMain::~GuiMain()
{
    for (size_t i = 0; i < nAccounts; i++)
    {
        pTrader->logout(i);
    }
    pQuote->logout();
    delete ui;
}

#pragma region 关联事件
void GuiMain::register_bind() const
{
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<size_t>("size_t&");
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<int64_t>("int64_t&");
    qRegisterMetaType<XTP_EXCHANGE_TYPE>("XTP_EXCHANGE_TYPE");
    qRegisterMetaType<XTP_EXCHANGE_TYPE>("XTP_EXCHANGE_TYPE&");

    QObject::connect(pQuote.get(), &Quote::QuoteError, this, &GuiMain::OnQuoteError);
    QObject::connect(pTrader.get(), &Trader::TraderError, this, &GuiMain::OnTraderError);

    // Trader <-> Quote
    QObject::connect(pTrader.get(), &Trader::TraderReqSubscribe, pQuote.get(), &Quote::OnTraderReqSubscribe);
    QObject::connect(pQuote.get(), &Quote::PositionQuoteReceived, pTrader.get(), &Trader::OnPositionQuoteReceived);

    // Market <- Quote
    QObject::connect(pQuote.get(), &Quote::Subscribed, ui->market, &GuiMarket::OnSubscribed);
    QObject::connect(pQuote.get(), &Quote::MarketDataReceived, ui->market, &GuiMarket::OnMarketDataReceived);

    QList<GuiPosition *> gui_positions = ui->Lower->findChildren<GuiPosition *>();
    QList<GuiOrder *> gui_orders = ui->Lower->findChildren<GuiOrder *>();
    QList<GuiOrderTrade *> gui_order_trades = ui->Lower->findChildren<GuiOrderTrade *>();
    QList<GuiOrderInsert *> gui_order_inserts = ui->Lower->findChildren<GuiOrderInsert *>();
    QList<GuiAsset *> gui_assets = ui->Upper->findChildren<GuiAsset *>();

    for (size_t i = 0; i < nAccounts; i++)
    {
        auto &sell = *gui_sells[i];
        // Sell <- Market
        QObject::connect(ui->market, &GuiMarket::UserSelectPrice, &sell, &GuiSell::OnUserSelectPrice);
        // Sell <-> Quote
        QObject::connect(&sell, &GuiSell::MarketReqSubscribe, pQuote.get(), &Quote::OnMarketReqSubscribe);
        // Sell <-> Trader
        QObject::connect(&sell, &GuiSell::SellReqPosition, pTrader.get(), &Trader::OnSellReqPosition);
        QObject::connect(pTrader.get(), &Trader::SellPositionReceived, &sell, &GuiSell::OnPositionReceived);
        QObject::connect(&sell, &GuiSell::SellReqSelling, pTrader.get(), &Trader::OnSellReqSelling);
        // Sell -> Sell
        QObject::connect(&sell, &GuiSell::SellReqSyncStockCode, this, &GuiMain::OnSellReqSyncStockCode);
        QObject::connect(&sell, &GuiSell::SellReqSyncStockInfo, this, &GuiMain::OnSellReqSyncStockInfo);
        QObject::connect(&sell, &GuiSell::SellReqSyncStockPrice, this, &GuiMain::OnSellReqSyncStockPrice);
        QObject::connect(&sell, &GuiSell::SellReqSyncSellQty, this, &GuiMain::OnSellReqSyncSellQty);
        QObject::connect(&sell, &GuiSell::SellReqSyncSellQtyText, this, &GuiMain::OnSellReqSyncSellQtyText);

        auto &position = *gui_positions[i];
        // Position <-> Trader
        QObject::connect(pTrader.get(), &Trader::TraderLogin, &position, &GuiPosition::OnTraderLogin);
        QObject::connect(pTrader.get(), &Trader::AccountPositionReceived, &position, &GuiPosition::OnPositionReceived);
        QObject::connect(pTrader.get(), &Trader::OrderSellReceived, &position, &GuiPosition::OnOrderSellReceived);
        QObject::connect(pTrader.get(), &Trader::OrderSellCanceled, &position, &GuiPosition::OnOrderSellCanceled);
        QObject::connect(pTrader.get(), &Trader::OrderSellTraded, &position, &GuiPosition::OnOrderSellTraded);
        QObject::connect(pTrader.get(), &Trader::OrderBuyTraded, &position, &GuiPosition::OnOrderBuyTraded);
        // Position <- Quote
        QObject::connect(pQuote.get(), &Quote::PositionQuoteReceived, &position, &GuiPosition::OnPositionQuoteReceived);
        // Position -> Sell
        QObject::connect(&position, &GuiPosition::UserSelectPosition, &sell, &GuiSell::OnUserSelectPosition);

        auto &order = *gui_orders[i];
        // Order <-> Trader
        QObject::connect(pTrader.get(), &Trader::TraderLogin, &order, &GuiOrder::OnTraderLogin);
        QObject::connect(pTrader.get(), &Trader::OrderReceived, &order, &GuiOrder::OnOrderReceived);
        QObject::connect(pTrader.get(), &Trader::OrderTraded, &order, &GuiOrder::OnOrderTraded);
        QObject::connect(pTrader.get(), &Trader::OrderCanceled, &order, &GuiOrder::OnOrderCanceled);
        QObject::connect(&order, &GuiOrder::ReqCancelOrder, pTrader.get(), &Trader::OnReqCancelOrder);

        auto &order_trade = *gui_order_trades[i];
        // OrderTrade <- Trader
        QObject::connect(pTrader.get(), &Trader::TraderLogin, &order_trade, &GuiOrderTrade::OnTraderLogin);
        QObject::connect(pTrader.get(), &Trader::OrderTraded, &order_trade, &GuiOrderTrade::OnOrderTraded);

        auto &order_insert = *gui_order_inserts[i];
        // OrderInsert <- Trader
        QObject::connect(pTrader.get(), &Trader::TraderLogin, &order_insert, &GuiOrderInsert::OnTraderLogin);
        QObject::connect(pTrader.get(), &Trader::OrderReceived, &order_insert, &GuiOrderInsert::OnOrderReceived);
        QObject::connect(pTrader.get(), &Trader::OrderTraded, &order_insert, &GuiOrderInsert::OnOrderTraded);
        QObject::connect(pTrader.get(), &Trader::OrderCanceled, &order_insert, &GuiOrderInsert::OnOrderCanceled);

        auto &asset = *gui_assets[i];
        // Asset <- Trader
        QObject::connect(pTrader.get(), &Trader::AssetReceived, &asset, &GuiAsset::OnAssetReceived);

        //初始化交易连接
        pTrader->login(i);
    }
}
#pragma endregion

#pragma region 关联账户
void GuiMain::OnAccountConnected(bool checked)
{
    accounts_connected = checked;
    for (size_t i = 0; i < nAccounts; i++)
    {
        auto &sell = *gui_sells[i];
        sell.SetActivate(accounts_connected);
    }

    ui->accConnector->setText(QString(QStringLiteral("账户%1关联")).arg(checked ? QStringLiteral("已") : QStringLiteral("未")));
}

void GuiMain::OnSellReqSyncStockCode(size_t id, const QString &stock_code) const
{
    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                sell->SetStockCode(stock_code);
            }
        }
    }
}

void GuiMain::OnSellReqSyncStockInfo(size_t id, const QString &stock_code, const QString &stock_name) const
{
    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                sell->SetStockCode(stock_code);
                sell->SetStockName(stock_name);
                sell->SetSellableQty();

                sell->SellReqPosition(sell->GetID(), stock_code);
            }
        }
    }
}

void GuiMain::OnSellReqSyncStockPrice(size_t id, double price) const
{
    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                sell->SetSellPrice(price);
            }
        }
    }
}

void GuiMain::OnSellReqSyncSellQty(size_t id, int index) const
{
    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                sell->SetSellQty(index);
            }
        }
    }
}

void GuiMain::OnSellReqSyncSellQtyText(size_t id, const QString &text) const
{
    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                sell->SetSellQty(text);
            }
        }
    }
}

void GuiMain::OnQuoteError() const
{
    if (QMessageBox::information(nullptr, "行情断线", QStringLiteral("是否关闭系统"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        exit(EXIT_SUCCESS);
    }
}

void GuiMain::OnTraderError() const
{
    if (QMessageBox::information(nullptr, "交易断线", QStringLiteral("是否关闭系统"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        exit(EXIT_SUCCESS);
    }
}
#pragma endregion