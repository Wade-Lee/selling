﻿#ifndef BASE_H
#define BASE_H

#include <unordered_map>
#include <QString>
#include <QMetaType>
#include <QLoggingCategory>

#include "xtp_api_struct.h"

namespace HuiFu
{
    using StockCode = QString;

    class StockStaticInfo
    {
    public:
        StockStaticInfo(const StockStaticInfo &) = delete;
        StockStaticInfo &operator=(const StockStaticInfo &) = delete;

        static StockStaticInfo &GetInstance()
        {
            static StockStaticInfo todayQSI;
            return todayQSI;
        }

        void InsertQSI(const XTPQSI &ticker_info)
        {
            mapQSI.insert(std::pair<StockCode, XTPQSI>(QString(ticker_info.ticker), ticker_info));
        }

        XTPQSI *GetQSI(const StockCode &stock_code)
        {
            if (mapQSI.find(stock_code) != mapQSI.end())
                return &mapQSI.at(stock_code);
            else
                return nullptr;
        }

        XTP_MARKET_TYPE GetMarket(const StockCode &stock_code) const
        {
            if (mapQSI.find(stock_code) != mapQSI.end())
            {
                switch (mapQSI.at(stock_code).exchange_id)
                {
                case XTP_EXCHANGE_SH:
                    return XTP_MKT_SH_A;
                case XTP_EXCHANGE_SZ:
                    return XTP_MKT_SZ_A;
                case XTP_EXCHANGE_UNKNOWN:
                    return XTP_MKT_UNKNOWN;
                default:
                    return XTP_MKT_INIT;
                }
            }
            else
                return XTP_MKT_INIT;
        }

        int GetQSISize() const { return mapQSI.size(); }

    private:
        StockStaticInfo() = default;
        ~StockStaticInfo() = default;

        std::unordered_map<StockCode, XTPQSI> mapQSI;
    };

    struct StockSellInfo
    {
        StockCode stock_code;
        QString stock_name;
        double price;
        int64_t sell_qty;
        int64_t total_qty;

        StockSellInfo(
            const StockCode &code,
            const QString &name,
            double p,
            int64_t sell,
            int64_t total) : stock_code(code), stock_name(name), price(p), sell_qty(sell), total_qty(total)
        {
            qRegisterMetaType<StockSellInfo>("StockSellInfo");
            qRegisterMetaType<StockSellInfo>("StockSellInfo&");
        }

        StockSellInfo() : stock_code(""), stock_name(""){};
    };

    struct MarketData
    {
        StockCode stock_code;
        double pre_close_price;
        double bid_price[5]; //5档申买价
        double ask_price[5]; //5档申卖价
        int64_t bid_qty[5];  //5档申买量
        int64_t ask_qty[5];  //5档申卖量
        double last_price;

        MarketData( //传入的数组必须含有5个以上的元素
            const char *code,
            double pre_close,
            double bid_p[],
            double ask_p[],
            int64_t bid_q[],
            int64_t ask_q[],
            double last) : stock_code(code), pre_close_price(pre_close), last_price(last)
        {
            qRegisterMetaType<MarketData>("MarketData");
            qRegisterMetaType<MarketData>("MarketData&");

            for (size_t i = 0; i < 5; i++)
            {
                bid_price[i] = bid_p[i];
                ask_price[i] = ask_p[i];
                bid_qty[i] = bid_q[i];
                ask_qty[i] = ask_q[i];
            }
        }

        MarketData() : stock_code(""){};
    };

    struct TraderMarketData
    {
        StockCode stock_code;
        double last_price;
        double bid_price;
        double pre_close_price;
        double high_price;

        TraderMarketData(
            const char *code,
            double last,
            double bid,
            double pre_close,
            double high) : stock_code(code),
                           last_price(last),
                           bid_price(bid),
                           pre_close_price(pre_close),
                           high_price(high)
        {
            qRegisterMetaType<TraderMarketData>("TraderMarketData");
            qRegisterMetaType<TraderMarketData>("TraderMarketData&");
        }

        TraderMarketData() : stock_code(""){};
    };

    struct AssetData
    {
        double buying_power;
        double withholding_amount;
        double security_asset;

        AssetData(
            double buying_power_,
            double withholding_amount_,
            double security_asset_) : buying_power(buying_power_),
                                      withholding_amount(withholding_amount_),
                                      security_asset(security_asset_)
        {
            qRegisterMetaType<AssetData>("AssetData");
            qRegisterMetaType<AssetData>("AssetData&");
        }

        AssetData(){};
    };

    struct PositionData
    {
        StockCode stock_code;
        QString stock_name;
        XTP_EXCHANGE_TYPE exchange_id;
        int64_t total_qty;
        int64_t sellable_qty;
        int64_t yesterday_qty;
        double avg_price; // 持仓成本

        PositionData(
            const QString &code,
            const QString &name,
            XTP_EXCHANGE_TYPE exchange,
            int64_t total,
            int64_t sellable,
            int64_t yesterday,
            double price) : stock_code(code),
                            stock_name(name),
                            exchange_id(exchange),
                            total_qty(total),
                            sellable_qty(sellable),
                            yesterday_qty(yesterday),
                            avg_price(price)
        {
            qRegisterMetaType<PositionData>("PositionData");
            qRegisterMetaType<PositionData>("PositionData&");
        }

        PositionData() : stock_code(""), stock_name(""){};
    };

    struct OrderReq
    {
        StockCode stock_code;
        QString stock_name;
        double price;

        OrderReq(
            const QString &code,
            const QString &name,
            double p) : stock_code(code),
                        stock_name(name),
                        price(p)
        {
            qRegisterMetaType<OrderReq>("OrderReq");
            qRegisterMetaType<OrderReq>("OrderReq&");
        }

        OrderReq() : stock_code(""), stock_name(""){};
    };

    struct OrderData
    {
        uint64_t order_xtp_id;
        StockCode stock_code;
        XTP_SIDE_TYPE side;
        QString insert_time;
        double price;
        int64_t quantity;
        int64_t qty_traded;
        double trade_amount;

        OrderData(
            uint64_t id,
            const char *code,
            XTP_SIDE_TYPE side_,
            QString insert_time_,
            double price_,
            int64_t quantity_,
            int64_t qty_traded_,
            double trade_amount_) : order_xtp_id(id),
                                    stock_code(code),
                                    side(side_),
                                    insert_time(insert_time_),
                                    price(price_),
                                    quantity(quantity_),
                                    qty_traded(qty_traded_),
                                    trade_amount(trade_amount_)
        {
            qRegisterMetaType<OrderData>("OrderData");
            qRegisterMetaType<OrderData>("OrderData&");
        };

        OrderData() : stock_code(""){};
    };

    struct TradeData
    {
        uint64_t order_xtp_id;
        StockCode stock_code;
        XTP_SIDE_TYPE side;
        QString trade_time;
        double price;
        int64_t quantity;
        double trade_amount;
        double trade_avg_price;

        TradeData(
            uint64_t id,
            const char *code,
            XTP_SIDE_TYPE side_,
            QString trade_time_,
            double price_,
            int64_t quantity_,
            double trade_amount_,
            double avg_price = 0.0) : order_xtp_id(id),
                                      stock_code(code),
                                      side(side_),
                                      trade_time(trade_time_),
                                      price(price_),
                                      quantity(quantity_),
                                      trade_amount(trade_amount_),
                                      trade_avg_price(avg_price)
        {
            qRegisterMetaType<TradeData>("TradeData");
            qRegisterMetaType<TradeData>("TradeData&");
        };

        TradeData() : stock_code(""){};
    };

    struct CancelData
    {
        uint64_t order_xtp_id;
        StockCode stock_code;
        XTP_SIDE_TYPE side;
        QString cancel_time;
        double price;
        int64_t quantity;
        int64_t qty_left;
        double trade_amount;

        CancelData(
            uint64_t id,
            const char *code,
            XTP_SIDE_TYPE side_,
            QString cancel_time_,
            double price_,
            int64_t quantity_,
            int64_t qty_left_,
            double trade_amount_) : order_xtp_id(id),
                                    stock_code(code),
                                    side(side_),
                                    cancel_time(cancel_time_),
                                    price(price_),
                                    quantity(quantity_),
                                    qty_left(qty_left_),
                                    trade_amount(trade_amount_)
        {
            qRegisterMetaType<CancelData>("CancelData");
            qRegisterMetaType<CancelData>("CancelData&");
        };

        CancelData() : stock_code(""){};
    };

} // namespace HuiFu

#endif