#include "gui_main.h"
#include "config.h"
#include "./ui_gui_main.h"

#include "gui_market.h"
#include "gui_asset.h"

#include <QKeyEvent>
#include <QMessageBox>

using namespace std;

GuiMain::GuiMain(const QuoteController &quoteController, const TraderController &traderController, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GuiMain),
      pTrader(traderController.GetTrader()),
      pQuote(quoteController.GetQuote()),
      accounts_connected(true)
{
    ui->setupUi(this);

    gui_sells = ui->Middle->findChildren<GuiSell *>();
    gui_trades = ui->Lower->findChildren<GuiTradeTab *>();

    nAccounts = Config::get_instance().get_account_num();

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

#pragma region 键盘事件
void GuiMain::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_F9:
        connect_accounts();
        break;
    case Qt::Key_F1:
        if (!accounts_connected)
        {
            for (size_t i = 0; i < gui_sells.size(); i++)
            {
                gui_sells[i]->Activate(false);
            }
        }
        gui_sells[0]->SetFocus();
        break;
    case Qt::Key_F2:
        if (!accounts_connected)
        {
            for (size_t i = 0; i < gui_sells.size(); i++)
            {
                gui_sells[i]->Activate(false);
            }
        }
        if (gui_sells.size() > 0)
            gui_sells[1]->SetFocus();
        break;
    case Qt::Key_D:
        gui_trades[0]->SetFocus(0);
        break;
    case Qt::Key_F:
        if (gui_trades.size() > 0)
            gui_trades[1]->SetFocus(0);
        break;
    case Qt::Key_S:
        gui_trades[0]->SetFocus(1);
        break;
    case Qt::Key_G:
        if (gui_trades.size() > 0)
            gui_trades[1]->SetFocus(1);
        break;
    case Qt::Key_W:
        for (size_t i = 0; i < gui_trades.size(); i++)
            gui_trades[i]->setCurrentIndex(2);
        break;
    case Qt::Key_E:
        for (size_t i = 0; i < gui_trades.size(); i++)
            gui_trades[i]->setCurrentIndex(3);
        break;
    case Qt::Key_R:
        for (size_t i = 0; i < gui_trades.size(); i++)
            gui_trades[i]->setCurrentIndex(4);
        break;
    default:
        break;
    }
}
#pragma endregion

#pragma region 关联事件
void GuiMain::register_bind() const
{
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<size_t>("size_t&");
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<int64_t>("int64_t&");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<uint64_t>("uint64_t&");
    qRegisterMetaType<XTP_EXCHANGE_TYPE>("XTP_EXCHANGE_TYPE");
    qRegisterMetaType<XTP_EXCHANGE_TYPE>("XTP_EXCHANGE_TYPE&");

    QObject::connect(pQuote, &Quote::QuoteError, this, &GuiMain::OnQuoteError);
    QObject::connect(pTrader, &Trader::TraderError, this, &GuiMain::OnTraderError);

    // Trader <-> Quote
    QObject::connect(pQuote, &Quote::PositionQuoteReceived, pTrader, &Trader::OnPositionQuoteReceived);

    // Market <- Quote
    QObject::connect(pQuote, &Quote::Subscribed, ui->market, &GuiMarket::OnSubscribed);
    QObject::connect(pQuote, &Quote::MarketDataReceived, ui->market, &GuiMarket::OnMarketDataReceived);

    QList<GuiAsset *> gui_assets = ui->Upper->findChildren<GuiAsset *>();

    for (size_t i = 0; i < nAccounts; i++)
    {
        auto &sell = *gui_sells[i];
        // Sell <-> Quote
        QObject::connect(&sell, &GuiSell::MarketReqSubscribe, pQuote, &Quote::OnMarketReqSubscribe);
        // Sell <-> Trader
        QObject::connect(&sell, &GuiSell::SellReqPosition, pTrader, &Trader::OnSellReqPosition);
        QObject::connect(pTrader, &Trader::SellPositionReceived, &sell, &GuiSell::OnPositionReceived);
        QObject::connect(&sell, &GuiSell::SellReqSelling, pTrader, &Trader::OnSellReqSelling);
        QObject::connect(pTrader, &Trader::OrderReceived, &sell, &GuiSell::OnOrderReceived);
        QObject::connect(pTrader, &Trader::OrderCanceled, &sell, &GuiSell::OnOrderCanceled);
        // Sell -> Sell
        QObject::connect(&sell, &GuiSell::SellReqSyncStockCode, this, &GuiMain::OnSellReqSyncStockCode);
        QObject::connect(&sell, &GuiSell::SellReqSyncStockInfo, this, &GuiMain::OnSellReqSyncStockInfo);
        QObject::connect(&sell, &GuiSell::SellReqSyncStockPrice, this, &GuiMain::OnSellReqSyncStockPrice);
        QObject::connect(&sell, &GuiSell::SellReqSyncSellQty, this, &GuiMain::OnSellReqSyncSellQty);
        QObject::connect(&sell, &GuiSell::SellReqSyncSellQtyText, this, &GuiMain::OnSellReqSyncSellQtyText);

        auto &trade = *gui_trades[i];
        // Trade <- Trader
        QObject::connect(pTrader, &Trader::TraderLogin, &trade, &GuiTradeTab::OnTraderLogin);
        QObject::connect(pTrader, &Trader::AccountPositionReceived, &trade, &GuiTradeTab::OnPositionReceived);
        QObject::connect(pTrader, &Trader::OrderReceived, &trade, &GuiTradeTab::OnOrderReceived);
        QObject::connect(pTrader, &Trader::OrderTraded, &trade, &GuiTradeTab::OnOrderTraded);
        QObject::connect(pTrader, &Trader::OrderCanceled, &trade, &GuiTradeTab::OnOrderCanceled);
        QObject::connect(&trade, &GuiTradeTab::PositionReqSubscribe, pQuote, &Quote::OnPositionReqSubscribe);
        QObject::connect(&trade, &GuiTradeTab::ReqCancelOrder, pTrader, &Trader::OnReqCancelOrder);
        // Trade <- Quote
        QObject::connect(pQuote, &Quote::PositionQuoteReceived, &trade, &GuiTradeTab::OnPositionQuoteReceived);
        // Trade -> Sell
        QObject::connect(&trade, &GuiTradeTab::UserReqSellPosition, &sell, &GuiSell::OnUserReqSellPosition);

        auto &asset = *gui_assets[i];
        // Asset <- Trader
        QObject::connect(pTrader, &Trader::AssetReceived, &asset, &GuiAsset::OnAssetReceived);

        //初始化交易连接
        pTrader->login(i);
    }
}
#pragma endregion

#pragma region 关联账户
void GuiMain::connect_accounts()
{
    accounts_connected = !accounts_connected;
    for (size_t i = 0; i < nAccounts; i++)
    {
        auto &sell = *gui_sells[i];
        if (accounts_connected)
            sell.Activate(accounts_connected);
        else
        {
            if (sell.GetID() == 0)
                sell.Activate(true);
            else
                sell.Activate(false);
        }
    }

    if (accounts_connected)
        ui->accountsConnected->setText(QString(QStringLiteral("关联")));
    else
        ui->accountsConnected->setText(QString(QStringLiteral("账户未关联")));
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