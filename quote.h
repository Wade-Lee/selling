﻿#ifndef QUOTE_H
#define QUOTE_H

#include "xtp_quote_api.h"

#include "base.h"
Q_DECLARE_LOGGING_CATEGORY(XTPQuote)

#include <QObject>
#include <QThread>
#include <unordered_set>

namespace HuiFu
{
    class Quote : public QObject, public XTP::API::QuoteSpi
    {
        Q_OBJECT

    private:
        XTP::API::QuoteApi *pQuoteApi;
        char **pStocks;

        std::unordered_set<StockCode> mMarketStocks, mTraderStocks;

    public slots:
        void OnMarketReqSubscribe(int, const QString &);
        void OnPositionReqSubscribe(XTP_EXCHANGE_TYPE, const QString &);

    signals:
        void QuoteError() const;
        // TODO : 在界面状态栏中显示登录登出信息
        void Logined(bool) const;

        void Subscribed(const QString &stock_code = "", bool subbed = false) const;
        void MarketDataReceived(const MarketData &) const;
        void PositionQuoteReceived(const TraderMarketData &) const;

#pragma region 故障检测
    private:
        void onerror(const QString &) const;

    public:
        virtual void OnDisconnected(int) override;
        virtual void OnError(XTPRI *) override;
#pragma endregion

#pragma region 登录登出
    public:
        void login() const;
        void logout() const;
#pragma endregion

#pragma region 查询当日可交易股票信息
    private:
        bool query_stocks_info() const;
        virtual void OnQueryAllTickers(XTPQSI *, XTPRI *, bool) override;
#pragma endregion

#pragma region 订阅快照
    private:
        bool subscribe(XTP_EXCHANGE_TYPE, const StockCode &) const;

    public:
        virtual void OnSubMarketData(XTPST *, XTPRI *, bool) override;
        virtual void OnDepthMarketData(XTPMD *, int64_t[], int32_t, int32_t, int64_t[], int32_t, int32_t) override;
#pragma endregion

    public:
        Quote(QObject *parent = nullptr);
        ~Quote();
    };

    class QuoteController : public QObject
    {
        Q_OBJECT

    private:
        QThread quoteThread;
        Quote *pQuote;

    public:
        QuoteController();
        ~QuoteController();
        Quote *GetQuote() const { return pQuote; }
    };
} // namespace HuiFu

#endif