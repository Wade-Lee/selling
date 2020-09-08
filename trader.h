#ifndef TRADER_H
#define TRADER_H

#include "xtp_trader_api.h"

#include "base.h"
Q_DECLARE_LOGGING_CATEGORY(XTPTrader)

#include <QObject>
#include <QThread>
#include <vector>
#include <map>

namespace HuiFu
{
    struct RestingOrder
    {
        XTP_SIDE_TYPE side;
        int64_t quantity;
        double price;
    };
    struct Position
    {
        XTP_MARKET_TYPE market = XTP_MKT_INIT;
        int64_t total_qty = 0;
        int64_t sellable_qty = 0;
        double last_price = 0.0;

        // 订单号 -> 挂单信息
        std::map<uint64_t, RestingOrder> resting_orders;
    };
    using Positions = std::map<StockCode, Position>;
    struct Asset
    {
        double buying_power;
        double withholding_amount;
        double security_asset;
        Positions positions;
    };

    class Trader : public QObject,
                   public XTP::API::TraderSpi
    {
        Q_OBJECT

    private:
        XTP::API::TraderApi *pTraderApi;
        bool is_start_late = false;

        size_t nAccounts;
        std::vector<uint64_t> mSessionIDs;
        std::vector<Asset> mAsset;
        size_t get_account_index(uint64_t, uint32_t) const;

    public slots:
        void OnPositionQuoteReceived(const TraderMarketData &);
        void OnSellReqSelling(size_t, const QString &, double, int64_t);
        void OnReqCancelOrder(size_t, uint64_t) const;

    signals:
        void TraderError() const;
        void TraderLogin(size_t) const;
        void AccountPositionReceived(size_t, const PositionData &);
        void OrderReceived(size_t, const OrderData &) const;
        void OrderTraded(size_t, const TradeData &) const;
        void OrderSellTraded(size_t, const TradeData &) const;
        void OrderBuyTraded(size_t, const TradeData &) const;
        void OrderCanceled(size_t, const CancelData &) const;
        void AssetReceived(size_t, const AssetData &) const;

#pragma region 故障检测
    private:
        void onerror(const QString &) const;

    public:
        virtual void OnDisconnected(uint64_t, int) override;
        virtual void OnError(XTPRI *) override;
#pragma endregion

#pragma region 登录登出
    public:
        void login(size_t);
        void logout(size_t);
#pragma endregion

#pragma region 查询持仓 / 报单 / 资金
    private:
        void query_positions(uint64_t);
        // void query_orders(uint64_t) const;
        void query_asset(uint64_t);

    public:
        void ReqAccountInfo(size_t);
        virtual void OnQueryPosition(XTPQueryStkPositionRsp *, XTPRI *, int, bool, uint64_t) override;
        // virtual void OnQueryOrder(XTPQueryOrderRsp *, XTPRI *, int, bool, uint64_t) override;
        virtual void OnQueryAsset(XTPQueryAssetRsp *, XTPRI *, int, bool, uint64_t) override;
#pragma endregion

#pragma region 报单相关
    private:
        XTPOrderInsertInfo mOrder;

        static QString format_time(int64_t);
        void order_event(uint64_t, const XTPOrderInfo &);
        void order_sell_inited(uint64_t, const XTPOrderInfo &);
        void order_buy_inited(uint64_t, const XTPOrderInfo &);
        void order_sell_canceled(uint64_t, const XTPOrderInfo &);
        void order_buy_canceled(uint64_t, const XTPOrderInfo &);
        void order_sell_traded(uint32_t, const XTPTradeReport &);
        void order_buy_traded(uint32_t, const XTPTradeReport &);

    public:
        virtual void OnOrderEvent(XTPOrderInfo *, XTPRI *, uint64_t) override;
        virtual void OnTradeEvent(XTPTradeReport *, uint64_t) override;
        virtual void OnCancelOrderError(XTPOrderCancelInfo *, XTPRI *, uint64_t) override;
#pragma endregion

    public:
        Trader(QObject *parent = nullptr);
        ~Trader();
    };

    class TraderController : public QObject
    {
        Q_OBJECT

    private:
        QThread traderThread;
        Trader *pTrader;

    public:
        TraderController();
        ~TraderController();
        Trader *GetTrader() const { return pTrader; }
    };
} // namespace HuiFu
#endif