#include "gui_main.h"
#include "config.h"
#include "./ui_gui_main.h"

#include "gui_market.h"
#include "gui_asset.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <algorithm>

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
        if (gui_trades.size() > 0)
            gui_trades[1]->setCurrentIndex(0);
        gui_trades[0]->SetFocus(0);
        break;
    case Qt::Key_F:
        gui_trades[0]->setCurrentIndex(0);
        if (gui_trades.size() > 0)
            gui_trades[1]->SetFocus(0);
        break;
    case Qt::Key_S:
        if (gui_trades.size() > 0)
            gui_trades[1]->setCurrentIndex(1);
        gui_trades[0]->SetFocus(1);
        break;
    case Qt::Key_G:
        gui_trades[0]->setCurrentIndex(1);
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
    case Qt::Key_J:
        user_select_position_by_key(1);
        break;
    case Qt::Key_K:
        user_select_position_by_key(2);
        break;
    case Qt::Key_L:
        user_select_position_by_key(3);
        break;
    case Qt::Key_U:
        user_select_position_by_key(4);
        break;
    case Qt::Key_I:
        user_select_position_by_key(5);
        break;
    case Qt::Key_O:
        user_select_position_by_key(6);
        break;
    case Qt::Key_P:
        user_select_position_by_key(7);
        break;
    case Qt::Key_M:
        for (size_t i = 0; i < gui_trades.size(); i++)
            gui_trades[i]->UserChooseOrder(true);
        break;
    case Qt::Key_N:
        for (size_t i = 0; i < gui_trades.size(); i++)
            gui_trades[i]->UserChooseOrder(false);
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
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<int32_t>("int32_t&");
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<int64_t>("int64_t&");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<uint64_t>("uint64_t&");
    qRegisterMetaType<XTP_EXCHANGE_TYPE>("XTP_EXCHANGE_TYPE");
    qRegisterMetaType<XTP_EXCHANGE_TYPE>("XTP_EXCHANGE_TYPE&");

    QObject::connect(pQuote, &Quote::QuoteError, this, &GuiMain::OnQuoteError);
    QObject::connect(pTrader, &Trader::TraderError, this, &GuiMain::OnTraderError);

    // Main <-> Trader
    QObject::connect(this, &GuiMain::SellReqSelling, pTrader, &Trader::OnSellReqSelling);
    QObject::connect(pTrader, &Trader::AccountPositionFinished, this, &GuiMain::OnAccountPositionFinished);

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
        QObject::connect(pQuote, &Quote::MarketDataReceived, &sell, &GuiSell::OnMarketDataReceived);
        // Sell <-> Trader
        QObject::connect(pTrader, &Trader::AccountPositionReceived, &sell, &GuiSell::OnPositionReceived);
        QObject::connect(&sell, &GuiSell::SellReqSelling, this, &GuiMain::OnSellReqSelling);
        QObject::connect(pTrader, &Trader::OrderReceived, &sell, &GuiSell::OnOrderReceived);
        QObject::connect(pTrader, &Trader::OrderTraded, &sell, &GuiSell::OnOrderTraded);
        QObject::connect(pTrader, &Trader::OrderCanceled, &sell, &GuiSell::OnOrderCanceled);
        QObject::connect(pTrader, &Trader::OrderError, &sell, &GuiSell::OnOrderError);
        QObject::connect(pTrader, &Trader::OrderRefused, &sell, &GuiSell::OnOrderRefused);
        // Sell -> Sell
        QObject::connect(&sell, &GuiSell::SellReqSyncStockCode, this, &GuiMain::OnSellReqSyncStockCode);
        QObject::connect(&sell, &GuiSell::SellReqSyncStockPrice, this, &GuiMain::OnSellReqSyncStockPrice);
        QObject::connect(&sell, &GuiSell::SellReqSyncStockInfo, this, &GuiMain::OnSellReqSyncStockInfo);

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
            auto &trade = *gui_trades[i];
            if (sell.HasFocus() || trade.HasFocus())
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

void GuiMain::user_select_position_by_key(size_t index) const
{
    QWidget *w = QApplication::focusWidget();
    if (w->objectName() == "sellablePositionTable")
    {
        if (w->parent()->parent()->parent()->objectName() == "tabL")
            gui_trades[0]->UserSelectPosition(index);
        else if (w->parent()->parent()->parent()->objectName() == "tabR")
            gui_trades[1]->UserSelectPosition(index);
        return;
    }

    if (w->parent()->objectName() == "sellL")
        gui_trades[0]->UserSelectPosition(index);
    else if (w->parent()->objectName() == "sellR")
        gui_trades[1]->UserSelectPosition(index);
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

void GuiMain::OnSellReqSyncStockInfo(size_t id, const StockSellInfo &stock_info) const
{
    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                sell->SyncStockInfo(stock_info);
            }
        }
    }
}

void GuiMain::OnSellReqSelling(size_t id, const QString &stock_code, const QString &stock_name, double price, int64_t quantity) const
{
    auto &cfg = Config::get_instance().get_accounts_config();
    QString text = QString(cfg[id].user.c_str());
    text += QStringLiteral("是否卖出：\n");
    text += stock_name;
    text += "\n";
    text += QStringLiteral("价格：");
    text += QString::number(price, 'f', 2);
    text += "\n";
    text += QStringLiteral("数量：");
    text += QString::number(quantity);

    if (accounts_connected)
    {
        for (auto sell : gui_sells)
        {
            if (sell->GetID() != id)
            {
                double price_ = sell->GetSellPrice();
                int quantity_ = sell->GetSellQty();
                if (quantity_ != 0)
                {
                    text += "\n";
                    text += "\n";
                    text += QString(cfg[sell->GetID()].user.c_str());
                    text += QStringLiteral("是否卖出：\n");
                    text += stock_name;
                    text += "\n";
                    text += QStringLiteral("价格：");
                    text += QString::number(price_, 'f', 2);
                    text += "\n";
                    text += QStringLiteral("数量：");
                    text += QString::number(quantity_);
                }

                if (QMessageBox::information(nullptr, "Req Selling", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                {
                    SellReqSelling(id, stock_code, price, quantity);
                    if (quantity_ != 0)
                    {
                        SellReqSelling(sell->GetID(), stock_code, price_, quantity_);
                    }
                }
            }
        }
    }
    else
    {
        if (QMessageBox::information(nullptr, "Req Selling", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            SellReqSelling(id, stock_code, price, quantity);
        }
    }
}

void GuiMain::OnAccountPositionFinished()
{
    nFinishedPositions++;
    if (nFinishedPositions == nAccounts)
    {
        set<StockCode> total_positions, common_positions, v;
        for (auto &trade : gui_trades)
        {
            auto p = trade->GetSellPositions();
            total_positions.insert(p.begin(), p.end());
            if (common_positions.size() == 0)
                common_positions = p;
            else
            {
                set_intersection(common_positions.begin(), common_positions.end(), p.begin(), p.end(), inserter(v, v.begin()));
                common_positions = v;
                v.clear();
            }
        }
        for (auto &trade : gui_trades)
        {
            auto p = trade->GetSellPositions();
            set<StockCode> diff;
            set_difference(p.begin(), p.end(), common_positions.begin(), common_positions.end(), inserter(diff, diff.begin()));
            vector<StockCode> sell_positions;
            for (auto &stock_code : common_positions)
            {
                sell_positions.push_back(stock_code);
            }
            for (auto &stock_code : diff)
            {
                sell_positions.push_back(stock_code);
            }
            for (size_t i = 0; i < sell_positions.size(); i++)
            {
                trade->SetSellPositionIndex(sell_positions[i], i + 1);
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