﻿#include "quote.h"
#include "config.h"

using namespace std;
using namespace XTP::API;

Q_LOGGING_CATEGORY(XTPQuote, "xtp.quote")

namespace HuiFu
{
#pragma region 登录登出
    void Quote::login() const
    {
        auto cfg = Config::get_instance().get_quote_config();
        auto accounts = Config::get_instance().get_accounts_config();
        switch (pQuoteApi->Login(
            cfg.ip.c_str(),
            cfg.port,
            accounts[0].user.c_str(),
            accounts[0].password.c_str(),
            XTP_PROTOCOL_TCP))
        {
        case 0:
            Logined(true);
            if (!query_stocks_info()) //获取当日股票静态信息
            {
                qCCritical(XTPQuote) << QStringLiteral("登录行情服务器出错：查询不到今日可交易合约");
            }
            break;
        case -2:
            qCDebug(XTPQuote) << QStringLiteral("已存在行情连接");
            break;
        case -1:
            onerror(QStringLiteral("登录行情"));
            Logined(false);
            break;
        case -3:
            qCDebug(XTPQuote) << QStringLiteral("登录行情输入参数有错误");
            Logined(false);
            break;
        default:
            break;
        }
    }

    void Quote::logout() const
    {
        if (pQuoteApi->Logout() == 0)
        {
            Logined(false);
        }
        else
        {
            onerror(QStringLiteral("登出"));
        }
    }
#pragma endregion

#pragma region 订阅快照
    void Quote::OnMarketReqSubscribe(int exchange_id, const QString &stock_code)
    {
        if (subscribe(static_cast<XTP_EXCHANGE_TYPE>(exchange_id), stock_code))
        {
            if (mMarketStocks.find(stock_code) == mMarketStocks.end())
                mMarketStocks.insert(stock_code);
            Subscribed(stock_code, true);
        }
        else
        {
            Subscribed(stock_code, false);
        }
    }

    void Quote::OnPositionReqSubscribe(XTP_EXCHANGE_TYPE exchange_id, const QString &stock_code)
    {
        if (mTraderStocks.find(stock_code) == mTraderStocks.end())
        {
            if (subscribe(exchange_id, stock_code))
            {
                mTraderStocks.insert(stock_code);
            }
        }
    }

    bool Quote::subscribe(XTP_EXCHANGE_TYPE exchange_id, const StockCode &stock_code) const
    {
        if (mMarketStocks.find(stock_code) != mMarketStocks.end() || mTraderStocks.find(stock_code) != mTraderStocks.end())
        {
            return true;
        }

        strcpy(pStocks[0], stock_code.toStdString().c_str());
        if (pQuoteApi->SubscribeMarketData(pStocks, 1, exchange_id) == 0)
        {
            qCInfo(XTPQuote) << QStringLiteral("订阅股票行情：") << stock_code;
            return true;
        }
        else
        {
            onerror(QString("1% 2%").arg(QStringLiteral("订阅股票")).arg(stock_code));
            return false;
        }
    }

    void Quote::OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
    {
        if (error_info == nullptr || error_info->error_id == 0)
        {
            Subscribed(ticker->ticker, true);
        }
        else
        {
            Subscribed(ticker->ticker, false);
            qCCritical(XTPQuote) << QStringLiteral("订阅行情错误：") << ticker->ticker << error_info->error_id << error_info->error_msg;
            if (mTraderStocks.find(ticker->ticker) != mTraderStocks.end())
            {
                mTraderStocks.erase(ticker->ticker);
            }
            if (mMarketStocks.find(ticker->ticker) != mMarketStocks.end())
            {
                mMarketStocks.erase(ticker->ticker);
            }
        }
    }

    void Quote::OnDepthMarketData(
        XTPMD *market_data,
        int64_t bid1_qty[],
        int32_t bid1_count,
        int32_t max_bid1_count,
        int64_t ask1_qty[],
        int32_t ask1_count,
        int32_t max_ask1_count)
    {
        if (mMarketStocks.find(market_data->ticker) != mMarketStocks.end())
        {
            MarketDataReceived(MarketData{
                market_data->ticker,
                market_data->pre_close_price,
                market_data->bid,
                market_data->ask,
                market_data->bid_qty,
                market_data->ask_qty,
                market_data->last_price});
        }

        if (mTraderStocks.find(market_data->ticker) != mTraderStocks.end())
        {
            PositionQuoteReceived(TraderMarketData{
                market_data->ticker,
                market_data->last_price,
                market_data->bid[0],
                market_data->pre_close_price,
                market_data->high_price});
        }
    }
#pragma endregion

    Quote::Quote(QObject *parent) : QObject(parent)
    {
        auto cfg = Config::get_instance().get_system_config();
        pQuoteApi = QuoteApi::CreateQuoteApi(
            cfg.client_id,
            cfg.log_path.c_str(),
            static_cast<XTP_LOG_LEVEL>(cfg.log_level));
        assert(pQuoteApi != nullptr);
        pQuoteApi->RegisterSpi(this);
        pQuoteApi->SetHeartBeatInterval(Config::get_instance().get_system_config().heart_beat_interval);

        pStocks = new char *[1];
        pStocks[0] = new char[7];
        pStocks[0][6] = '\0';
    }

    Quote::~Quote()
    {
        if (pQuoteApi)
        {
            pQuoteApi->Release();
            pQuoteApi = nullptr;
        }

        delete[] pStocks[0];
        delete[] pStocks;
    }

#pragma region 故障检测
    void Quote::onerror(const QString &action) const
    {
        XTPRI *error_info = pQuoteApi->GetApiLastError();
        qCCritical(XTPQuote) << action << QStringLiteral("行情连接出错:") << error_info->error_id << error_info->error_msg;
    }

    void Quote::OnDisconnected(int reason)
    {
        QuoteError();
        qCCritical(XTPQuote) << QStringLiteral("连接中断，需要重新登录并重新订阅行情");
    }

    void Quote::OnError(XTPRI *error_info)
    {
        QuoteError();
        qCCritical(XTPQuote) << QStringLiteral("行情服务器错误:") << error_info->error_id << error_info->error_msg;
    }
#pragma endregion

#pragma region 查询当日可交易股票信息
    bool Quote::query_stocks_info() const
    {
        if (pQuoteApi->QueryAllTickers(XTP_EXCHANGE_SZ) == 0 && pQuoteApi->QueryAllTickers(XTP_EXCHANGE_SH) == 0)
        {
            return true;
        }
        else
        {
            qCCritical(XTPQuote) << QStringLiteral("查询当日可交易股票出错");
            return false;
        }
    }

    void Quote::OnQueryAllTickers(XTPQSI *ticker_info, XTPRI *error_info, bool is_last)
    {
        if (error_info == nullptr || error_info->error_id == 0)
        {
            StockStaticInfo::GetInstance().InsertQSI(*ticker_info);
        }
        else
        {
            qCCritical(XTPQuote) << QStringLiteral("查询今日可交易合约返回错误信息:") << error_info->error_id << error_info->error_msg;
        }
    }
#pragma endregion

    QuoteController::QuoteController()
    {
        pQuote = new Quote;
        pQuote->moveToThread(&quoteThread);
        connect(&quoteThread, &QThread::finished, pQuote, &QObject::deleteLater);
        quoteThread.start();
        pQuote->login();
    }

    QuoteController::~QuoteController()
    {
        quoteThread.quit();
        quoteThread.wait();
    }
} // namespace HuiFu