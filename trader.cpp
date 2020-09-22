#include "trader.h"
#include "config.h"
#include <time.h>
#include <QDateTime>

#define ORDER_QUNTITY_LIMIT 999900
#define WITHHOLD_BUY_SPECIAL_RATE 0.00032
#define WITHHOLD_SELL_SPECIAL_RATE 0.00052
#define WITHHOLD_BUY_RATE 0.00004
#define WITHHOLD_SELL_RATE 0.00104
#define ORDER_SELL_CLIENT_ID 100

using namespace std;
using namespace XTP::API;

Q_LOGGING_CATEGORY(XTPTrader, "xtp.trader")

namespace HuiFu
{
#pragma region 登录登出
    void Trader::login(size_t acc_index)
    {
        auto cfg = Config::get_instance().get_trader_config();
        auto account = Config::get_instance().get_accounts_config()[acc_index];

        TraderLogin(acc_index);

        mSessionIDs[acc_index] = pTraderApi->Login(
            cfg.ip.c_str(),
            cfg.port,
            account.user.c_str(),
            account.password.c_str(),
            XTP_PROTOCOL_TCP);
        if (mSessionIDs[acc_index] == 0)
        {
            onerror(QStringLiteral("登录账户"));
            login(acc_index);
        }
        else
        {
            ReqAccountInfo(acc_index);
        }
    }

    void Trader::logout(size_t acc_index)
    {
        if (pTraderApi->Logout(mSessionIDs[acc_index]) == 0)
        {
            mSessionIDs[acc_index] = 0;
        }
        else
        {
            auto account = Config::get_instance().get_accounts_config()[acc_index];
            onerror(QStringLiteral("登出账户"));
        }
    }

    size_t Trader::get_account_index(uint64_t session_id, uint32_t order_client_id) const
    {
        for (size_t i = 0; i < nAccounts; i++)
        {
            if (session_id == mSessionIDs[i])
            {
                return i;
            }
        }

        // 重要约定：如果是由全自动程序产生的订单，则用户自定义报单引用号即为账户索引号
        return static_cast<size_t>(order_client_id);
    }
#pragma endregion

#pragma region 查询持仓 / 报单 / 资金
    void Trader::ReqAccountInfo(size_t acc_index)
    {
        // 1.查询持仓
        query_positions(mSessionIDs[acc_index]);
        // 2.查询资产
        query_asset(mSessionIDs[acc_index]);
    }

    void Trader::query_positions(uint64_t session_id)
    {
        for (size_t i = 0; i < nAccounts; i++)
        {
            if (session_id == mSessionIDs[i])
            {
                qCInfo(XTPTrader) << QStringLiteral("查询全部持仓");

                while (pTraderApi->QueryPosition(nullptr, mSessionIDs[i], 0) != 0)
                {
                    onerror(QStringLiteral("查询全部持仓"));
                    logout(i);
                    login(i);
                }
            }
        }
    }

    void Trader::OnQueryPosition(XTPQueryStkPositionRsp *position, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id)
    {
        if (error_info == nullptr || error_info->error_id == 0)
        {
            qCInfo(XTPTrader) << session_id << position->ticker_name << position->total_qty << position->sellable_qty << is_last;
            // 应避免加已卖光的昨仓
            if (position->total_qty == 0)
            {
                if (is_last)
                {
                    AccountPositionFinished();
                    start_late_count++;
                    if (start_late_count == nAccounts)
                        is_start_late = false;
                }
                return;
            }

            for (size_t i = 0; i < nAccounts; i++)
            {
                // 只在启动程序时查询一次持仓
                if (session_id == mSessionIDs[i])
                {
                    qCInfo(XTPTrader) << QString::number(i) << QStringLiteral(" 加入持仓：") << position->ticker_name << position->total_qty << position->sellable_qty;
                    if (mAsset[i].positions.find(position->ticker) != mAsset[i].positions.end())
                    {
                        auto &p = mAsset[i].positions.at(position->ticker);
                        p.market = position->market;
                        p.total_qty = position->total_qty;
                        p.sellable_qty = position->sellable_qty;
                        p.last_price = position->avg_price;
                    }
                    else
                        mAsset[i].positions[position->ticker] = Position{
                            position->market,
                            position->total_qty,
                            position->sellable_qty,
                            position->avg_price};
                }

                // 应避免加今仓
                if (session_id == mSessionIDs[i] && position->yesterday_position != 0)
                {
                    AccountPositionReceived(i, PositionData{
                                                   position->ticker,
                                                   position->ticker_name,
                                                   (position->market == XTP_MKT_SZ_A) ? XTP_EXCHANGE_SZ : XTP_EXCHANGE_SH,
                                                   position->total_qty,
                                                   position->sellable_qty});
                }
            }

            if (is_last)
            {
                AccountPositionFinished();
                start_late_count++;
                if (start_late_count == nAccounts)
                    is_start_late = false;
            }
        }
        else
        {
            if (error_info->error_id == 11000350)
            {
                AccountPositionFinished();
                start_late_count++;
                if (start_late_count == nAccounts)
                    is_start_late = false;
                qCInfo(XTPTrader) << session_id << QStringLiteral("没有持仓");
            }
            else
                qCCritical(XTPTrader) << QStringLiteral("查询持仓错误：") << error_info->error_id << error_info->error_msg;
        }
    }

    // void Trader::OnQueryOrder(XTPQueryOrderRsp *order_info, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id)
    // {
    //     if (error_info != nullptr && error_info->error_id != 0)
    //     {
    //         if (error_info->error_id == 11000350)
    //             qCInfo(XTPTrader) << session_id << QStringLiteral("没有报单");
    //         else
    //             qCCritical(XTPTrader) << QStringLiteral("收到查询报单事件错误信息：") << error_info->error_id << " " << error_info->error_msg;
    //         return;
    //     }
    //     qCInfo(XTPTrader) << QStringLiteral("报单状态变化：") << order_info->order_xtp_id << order_info->ticker;
    //     if (order_info->business_type == XTP_BUSINESS_TYPE_CASH)
    //     {
    //         order_event(session_id, *order_info);
    //     }
    // };

    void Trader::query_asset(uint64_t session_id)
    {
        for (size_t i = 0; i < nAccounts; i++)
        {
            if (session_id == mSessionIDs[i])
            {
                qCInfo(XTPTrader) << QStringLiteral("查询资产");
                while (pTraderApi->QueryAsset(mSessionIDs[i], 0) != 0)
                {
                    onerror(QStringLiteral("查询资产"));
                    logout(i);
                    login(i);
                }
            }
        }
    }

    void Trader::OnQueryAsset(XTPQueryAssetRsp *asset, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id)
    {
        if (error_info == nullptr || error_info->error_id == 0)
        {
            for (size_t i = 0; i < nAccounts; i++)
            {
                if (session_id == mSessionIDs[i])
                {
                    mAsset[i].buying_power = asset->buying_power;
                    mAsset[i].withholding_amount = asset->withholding_amount;
                    mAsset[i].security_asset = 0;
                    qCInfo(XTPTrader) << QStringLiteral("仓位列表：");
                    for (auto &position : mAsset[i].positions)
                    {
                        mAsset[i].security_asset += position.second.total_qty * position.second.last_price;
                        qCInfo(XTPTrader) << position.first << position.second.total_qty << position.second.last_price;
                    }
                    qCInfo(XTPTrader) << QStringLiteral("账户资金初始值：") << mAsset[i].buying_power << mAsset[i].withholding_amount << mAsset[i].security_asset;
                    AssetReceived(i, AssetData{mAsset[i].buying_power, mAsset[i].withholding_amount, mAsset[i].security_asset});
                }
            }
        }
        else
        {
            qCCritical(XTPTrader) << QStringLiteral("查询资产错误：") << error_info->error_id << error_info->error_msg;
        }
    }

    void Trader::OnPositionQuoteReceived(const TraderMarketData &d)
    {
        for (size_t i = 0; i < nAccounts; i++)
        {
            auto it = mAsset[i].positions.find(d.stock_code);
            if (it != mAsset[i].positions.end())
            {
                it->second.last_price = d.last_price;
                mAsset[i].security_asset = 0;
                for (auto &position : mAsset[i].positions)
                {
                    mAsset[i].security_asset += position.second.total_qty * position.second.last_price;
                }
                // qCInfo(XTPTrader) << QStringLiteral("账户资金在证券资产行情更新时：") << mAsset[i].buying_power << mAsset[i].withholding_amount << mAsset[i].security_asset;
                AssetReceived(i, AssetData{mAsset[i].buying_power, mAsset[i].withholding_amount, mAsset[i].security_asset});
            }
        }
    }
#pragma endregion

#pragma region 报单相关
    void Trader::OnSellReqSelling(size_t id, const QString &stock_code, double price, int64_t quantity)
    {
        XTP_MARKET_TYPE market = StockStaticInfo::GetInstance().GetMarket(stock_code);
        if (market == XTP_MKT_INIT || market == XTP_MKT_UNKNOWN)
        {
            qCInfo(XTPTrader) << QStringLiteral("提交报单找不到股票名称：") << stock_code;
            return;
        }

        strcpy(mOrder.ticker, stock_code.toStdString().c_str());
        mOrder.market = market;
        mOrder.price = price;
        while (quantity > 0)
        {
            mOrder.quantity = min(quantity, static_cast<int64_t>(ORDER_QUNTITY_LIMIT));
            mOrder.order_xtp_id = pTraderApi->InsertOrder(&mOrder, mSessionIDs[id]);
            if (mOrder.order_xtp_id == 0)
            {
                onerror(QStringLiteral("提交报单"));
                return;
            }
            quantity -= ORDER_QUNTITY_LIMIT;
        }
    }

    void Trader::OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info, uint64_t session_id)
    {
        if (error_info != nullptr && error_info->error_id != 0)
        {
            if (order_info->order_client_id != ORDER_SELL_CLIENT_ID)
                return;

            int64_t current_date_time = QDateTime::currentDateTime().toString("yyyyMMddhhmmss").toLongLong() * 1000;
            if (current_date_time > order_info->insert_time + 30000)
                return;

            if (order_info != nullptr)
                qCCritical(XTPTrader) << QStringLiteral("收到报单事件错误消息：") << order_info->order_xtp_id << error_info->error_id << error_info->error_msg;
            else
                qCCritical(XTPTrader) << QStringLiteral("收到报单事件错误消息：") << error_info->error_id << error_info->error_msg;

            // 找出账户id
            size_t acc_index = get_account_index(session_id, order_info->order_client_id);
            OrderError(acc_index, order_info->ticker, error_info->error_msg, error_info->error_id);
            return;
        }
        qCInfo(XTPTrader) << QStringLiteral("报单状态变化：") << order_info->order_xtp_id << order_info->ticker;
        if (order_info->business_type == XTP_BUSINESS_TYPE_CASH)
        {
            order_event(session_id, *order_info);
        }
    }

    void Trader::OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id)
    {
        if (trade_info->business_type != XTP_BUSINESS_TYPE_CASH)
        {
            return;
        }

        qCInfo(XTPTrader) << QStringLiteral("报单成交：") << trade_info->order_xtp_id << trade_info->ticker << QStringLiteral("成交价格") << trade_info->price << QStringLiteral("成交数量") << trade_info->quantity;

        if (trade_info->side == XTP_SIDE_BUY)
        {
            order_buy_traded(session_id, *trade_info);
        }
        else
        {
            order_sell_traded(session_id, *trade_info);
        }
    }

    void Trader::OnReqCancelOrder(size_t acc_index, uint64_t order_xtp_id) const
    {
        if (pTraderApi->CancelOrder(order_xtp_id, mSessionIDs[acc_index]) == 0)
        {
            onerror(QStringLiteral("撤销报单"));
        }
    }

    void Trader::OnCancelOrderError(XTPOrderCancelInfo *cancel_info, XTPRI *error_info, uint64_t session_id)
    {
        qCCritical(XTPTrader) << QStringLiteral("撤销报单失败：") << cancel_info->order_xtp_id;
        if (error_info != nullptr && error_info->error_id != 0)
        {
            qCCritical(XTPTrader) << QStringLiteral("错误信息：") << error_info->error_id << " " << error_info->error_msg;
        }
    }

    // void Trader::query_orders(uint64_t session_id) const
    // {
    //     XTPQueryOrderReq req{NULL, 0, 0};
    //     if (pTraderApi->QueryOrders(&req, session_id, 0) != 0)
    //     {
    //         onerror(QStringLiteral("查询全部报单"));
    //     }
    // }

    QString Trader::format_time(int64_t t)
    {
        QString tstr = QString::number(t);
        return QString("%1:%2:%3").arg(tstr.mid(8, 2)).arg(tstr.mid(10, 2)).arg(tstr.mid(12, 2));
    }

    void Trader::order_event(uint64_t session_id, const XTPOrderInfo &order)
    {
        switch (order.order_status)
        {
        case XTP_ORDER_STATUS_INIT:
            qCInfo(XTPTrader) << QStringLiteral("初始化");
            return;
        case XTP_ORDER_STATUS_ALLTRADED:
            qCInfo(XTPTrader) << QStringLiteral("全部成交");
            break;
        case XTP_ORDER_STATUS_PARTTRADEDQUEUEING:
            qCInfo(XTPTrader) << QStringLiteral("部分成交");
            break;
        case XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING:
            qCInfo(XTPTrader) << QStringLiteral("部分撤单");
            if (order.side == XTP_SIDE_BUY)
                order_buy_canceled(session_id, order);
            else
                order_sell_canceled(session_id, order);
            return;
        case XTP_ORDER_STATUS_NOTRADEQUEUEING:
            qCInfo(XTPTrader) << QStringLiteral("未成交");
            if (order.side == XTP_SIDE_BUY)
                order_buy_inited(session_id, order);
            else
                order_sell_inited(session_id, order);
            return;
        case XTP_ORDER_STATUS_CANCELED:
            qCInfo(XTPTrader) << QStringLiteral("已撤单");
            if (order.side == XTP_SIDE_BUY)
                order_buy_canceled(session_id, order);
            else
                order_sell_canceled(session_id, order);
            break;
        case XTP_ORDER_STATUS_REJECTED:
        {
            qCInfo(XTPTrader) << QStringLiteral("已拒绝");
            // 找出账户id
            size_t acc_index = get_account_index(session_id, order.order_client_id);
            OrderRefused(acc_index, order.ticker);
        }
            return;
        case XTP_ORDER_STATUS_UNKNOWN:
            qCInfo(XTPTrader) << QStringLiteral("未知订单状态");
            return;
        default:
            qCInfo(XTPTrader) << QStringLiteral("未知订单状态");
            return;
        }
    }

    void Trader::order_buy_inited(uint64_t session_id, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString insert_time = format_time(order.insert_time);

        // 找出账户id
        size_t acc_index = get_account_index(session_id, order.order_client_id);

        // 刷新Trade界面显示
        OrderReceived(acc_index, OrderData{
                                     order.order_xtp_id,
                                     order.ticker,
                                     order.side,
                                     insert_time,
                                     order.price,
                                     order.quantity,
                                     order.qty_traded,
                                     order.trade_amount});

        // 找到账户仓位
        QString stock_code{order.ticker};
        auto &positions = mAsset[acc_index].positions;

        if (positions.find(stock_code) == positions.end())
        {
            // 新建一个仓位
            positions[stock_code] = Position{order.market, 0, 0, order.price};
        }
        // 在仓位下新建一个挂单
        positions.at(stock_code)
            .resting_orders[order.order_xtp_id] = RestingOrder{
            order.side,
            order.quantity,
            order.price};

        // 计算预扣费用
        double order_amount = order.price * order.quantity;
        double holding_amount = max(order_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order_amount * WITHHOLD_BUY_RATE;

        // 更新资金信息
        mAsset[acc_index].buying_power -= order_amount + holding_amount;
        mAsset[acc_index].withholding_amount += order_amount + holding_amount;
        // 刷新Asset界面显示
        mAsset[acc_index].security_asset = 0;
        for (auto &position : positions)
        {
            mAsset[acc_index].security_asset += position.second.total_qty * position.second.last_price;
        }
        qCInfo(XTPTrader) << QStringLiteral("账户资金在买入证券资产挂单时：") << mAsset[acc_index].buying_power << mAsset[acc_index].withholding_amount << mAsset[acc_index].security_asset;
        AssetReceived(acc_index, AssetData{mAsset[acc_index].buying_power, mAsset[acc_index].withholding_amount, mAsset[acc_index].security_asset});
    }

    void Trader::order_sell_inited(uint64_t session_id, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString insert_time = format_time(order.insert_time);

        // 找出账户id
        size_t acc_index = get_account_index(session_id, order.order_client_id);

        // 刷新Trade和Sell界面显示
        OrderReceived(acc_index, OrderData{
                                     order.order_xtp_id,
                                     order.ticker,
                                     order.side,
                                     insert_time,
                                     order.price,
                                     order.quantity,
                                     order.qty_traded,
                                     order.trade_amount});

        // 找到账户仓位
        QString stock_code{order.ticker};
        auto &positions = mAsset[acc_index].positions;
        if (is_start_late)
        {
            if (positions.find(stock_code) == positions.end())
            {
                // 中途重启收到该股票的第一个挂单，新建一个
                positions[stock_code] = Position{order.market, order.quantity, order.quantity, order.price};
            }
            else
            {
                auto &position = positions.at(stock_code);
                position.total_qty += order.quantity;
                position.sellable_qty += order.quantity;
                position.last_price = order.price;
            }
        }

        // 减去仓位的可卖数量
        positions.at(stock_code).sellable_qty -= order.quantity;

        // 在仓位下新建一个挂单
        positions.at(stock_code)
            .resting_orders[order.order_xtp_id] = RestingOrder{
            order.side,
            order.quantity,
            order.price};

        // 计算预扣费用
        double order_amount = order.price * order.quantity;
        double holding_amount = max(order_amount * WITHHOLD_SELL_SPECIAL_RATE, 8.0) + order_amount * WITHHOLD_SELL_RATE;

        // 更新资金信息
        mAsset[acc_index].buying_power -= holding_amount;
        mAsset[acc_index].withholding_amount += holding_amount;
        // 刷新Asset界面显示
        mAsset[acc_index].security_asset = 0;
        for (auto &position : positions)
        {
            mAsset[acc_index].security_asset += position.second.total_qty * position.second.last_price;
        }
        qCInfo(XTPTrader) << QStringLiteral("账户资金在卖出证券资产挂单时：") << mAsset[acc_index].buying_power << mAsset[acc_index].withholding_amount << mAsset[acc_index].security_asset;
        AssetReceived(acc_index, AssetData{mAsset[acc_index].buying_power, mAsset[acc_index].withholding_amount, mAsset[acc_index].security_asset});
    }

    void Trader::order_buy_canceled(uint64_t session_id, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString cancel_time = format_time(order.cancel_time);

        // 找出账户id
        size_t acc_index = get_account_index(session_id, order.order_client_id);

        // 刷新Trade界面显示
        OrderCanceled(acc_index, CancelData{
                                     order.order_xtp_id,
                                     order.ticker,
                                     order.side,
                                     cancel_time,
                                     order.price,
                                     order.quantity,
                                     order.qty_left,
                                     order.trade_amount});

        // 找到账户仓位
        QString stock_code{order.ticker};
        auto &positions = mAsset[acc_index].positions;
        auto &position = positions.at(stock_code);

        // 找到仓位下撤销的挂单
        auto &resting_order = position.resting_orders.at(order.order_xtp_id);
        resting_order.quantity -= order.qty_left; // 减少挂单数量
        // 挂单全部撤销时删除
        if (resting_order.quantity == 0)
        {
            position.resting_orders.erase(order.order_xtp_id);
        }

        // 买入仓位全部撤销且为空时删除
        if (position.total_qty == 0 && position.resting_orders.size() == 0)
        {
            positions.erase(stock_code);
        }

        // 计算预扣费用
        double order_amount = order.price * order.qty_left;
        double holding_amount = max(order_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order_amount * WITHHOLD_BUY_RATE;

        // 更新资金信息
        mAsset[acc_index].buying_power += order_amount + holding_amount;
        mAsset[acc_index].withholding_amount -= order_amount + holding_amount;
        // 刷新Asset界面显示
        mAsset[acc_index].security_asset = 0;
        for (auto &position : mAsset[acc_index].positions)
        {
            mAsset[acc_index].security_asset += position.second.total_qty * position.second.last_price;
        }
        qCInfo(XTPTrader) << QStringLiteral("账户资金在买入证券资产撤单时：") << mAsset[acc_index].buying_power << mAsset[acc_index].withholding_amount << mAsset[acc_index].security_asset;
        AssetReceived(acc_index, AssetData{mAsset[acc_index].buying_power, mAsset[acc_index].withholding_amount, mAsset[acc_index].security_asset});
    }

    void Trader::order_sell_canceled(uint64_t session_id, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString cancel_time = format_time(order.cancel_time);

        // 找出账户id
        size_t acc_index = get_account_index(session_id, order.order_client_id);

        // 刷新Trade和Sell界面显示
        OrderCanceled(acc_index, CancelData{
                                     order.order_xtp_id,
                                     order.ticker,
                                     order.side,
                                     cancel_time,
                                     order.price,
                                     order.quantity,
                                     order.qty_left,
                                     order.trade_amount});

        // 找到账户仓位
        QString stock_code{order.ticker};
        auto &positions = mAsset[acc_index].positions;
        auto &position = positions.at(stock_code);

        // 找到仓位下撤销的挂单
        auto &resting_order = position.resting_orders.at(order.order_xtp_id);
        resting_order.quantity -= order.qty_left; // 减少挂单数量
        // 挂单全部撤销时删除
        if (resting_order.quantity == 0)
        {
            position.resting_orders.erase(order.order_xtp_id);
        }

        position.sellable_qty += order.qty_left;
        if (is_start_late)
        {
            // 盘中重启时要剔除掉撤单的仓位
            position.total_qty -= order.qty_left;
            if (position.total_qty == 0)
            {
                positions.erase(stock_code);
            }
        }

        // 计算预扣费用
        double order_amount = order.price * order.qty_left;
        double holding_amount = max(order_amount * WITHHOLD_SELL_SPECIAL_RATE, 8.0) + order_amount * WITHHOLD_SELL_RATE;

        // 更新资金信息
        mAsset[acc_index].buying_power += holding_amount;
        mAsset[acc_index].withholding_amount -= holding_amount;
        // 刷新Asset界面显示
        mAsset[acc_index].security_asset = 0;
        for (auto &position : positions)
        {
            mAsset[acc_index].security_asset += position.second.total_qty * position.second.last_price;
        }
        qCInfo(XTPTrader) << QStringLiteral("账户资金在卖出证券资产撤单时：") << mAsset[acc_index].buying_power << mAsset[acc_index].withholding_amount << mAsset[acc_index].security_asset;
        AssetReceived(acc_index, AssetData{mAsset[acc_index].buying_power, mAsset[acc_index].withholding_amount, mAsset[acc_index].security_asset});
    }

    void Trader::order_buy_traded(uint32_t session_id, const XTPTradeReport &trade)
    {
        // 清洗时间
        QString trade_time = format_time(trade.trade_time);

        // 找出账户id
        size_t acc_index = get_account_index(session_id, trade.order_client_id);

        // 读取股票名称
        QString stock_code{trade.ticker};

        // 找到账户仓位
        auto &position = mAsset[acc_index].positions.at(stock_code);

        // 找到仓位下成交的挂单
        auto &resting_order = position.resting_orders.at(trade.order_xtp_id);
        resting_order.quantity -= trade.quantity; // 减少挂单数量

        // 更新仓位信息
        position.total_qty += trade.quantity;
        position.last_price = trade.price;

        // 更新资金信息
        mAsset[acc_index].buying_power += trade.quantity * resting_order.price;
        mAsset[acc_index].buying_power -= trade.trade_amount;
        mAsset[acc_index].withholding_amount -= trade.quantity * resting_order.price;

        // 挂单全部成交时删除
        if (resting_order.quantity == 0)
        {
            position.resting_orders.erase(trade.order_xtp_id);
        }

        // 刷新Asset界面显示
        mAsset[acc_index].security_asset = 0;
        for (auto &position : mAsset[acc_index].positions)
        {
            mAsset[acc_index].security_asset += position.second.total_qty * position.second.last_price;
        }
        qCInfo(XTPTrader) << QStringLiteral("账户资金在证券资产成交时：") << mAsset[acc_index].buying_power << mAsset[acc_index].withholding_amount << mAsset[acc_index].security_asset;
        AssetReceived(acc_index, AssetData{mAsset[acc_index].buying_power, mAsset[acc_index].withholding_amount, mAsset[acc_index].security_asset});

        // 刷新Order界面显示
        OrderTraded(acc_index, TradeData{
                                   trade.order_xtp_id,
                                   trade.ticker,
                                   trade.side,
                                   trade_time,
                                   trade.price,
                                   trade.quantity,
                                   trade.trade_amount});

        // 刷新Position界面显示
        OrderBuyTraded(acc_index, TradeData{
                                      trade.order_xtp_id,
                                      trade.ticker,
                                      trade.side,
                                      trade_time,
                                      trade.price,
                                      trade.quantity,
                                      trade.trade_amount});
    }

    void Trader::order_sell_traded(uint32_t session_id, const XTPTradeReport &trade)
    {
        // 清洗时间
        QString trade_time = format_time(trade.trade_time);

        // 找出账户id
        size_t acc_index = get_account_index(session_id, trade.order_client_id);

        // 读取股票名称
        QString stock_code{trade.ticker};

        // 找到账户仓位
        auto &position = mAsset[acc_index].positions.at(stock_code);

        // 找到仓位下成交的挂单
        auto &resting_order = position.resting_orders.at(trade.order_xtp_id);
        resting_order.quantity -= trade.quantity; // 减少挂单数量
        position.total_qty -= trade.quantity;     // 减少仓位数量

        // 更新资金信息
        mAsset[acc_index].buying_power += trade.trade_amount;

        // 挂单全部成交时删除
        if (resting_order.quantity == 0)
        {
            position.resting_orders.erase(trade.order_xtp_id);
        }

        // 仓位全部卖出时删除
        if (position.total_qty == 0)
        {
            mAsset[acc_index].positions.erase(stock_code);
        }

        // 刷新Asset界面显示
        mAsset[acc_index].security_asset = 0;
        for (auto &position : mAsset[acc_index].positions)
        {
            mAsset[acc_index].security_asset += position.second.total_qty * position.second.last_price;
        }
        qCInfo(XTPTrader) << QStringLiteral("账户资金在证券资产成交时：") << mAsset[acc_index].buying_power << mAsset[acc_index].withholding_amount << mAsset[acc_index].security_asset;
        AssetReceived(acc_index, AssetData{mAsset[acc_index].buying_power, mAsset[acc_index].withholding_amount, mAsset[acc_index].security_asset});

        // 刷新Order界面显示
        OrderTraded(acc_index, TradeData{
                                   trade.order_xtp_id,
                                   trade.ticker,
                                   trade.side,
                                   trade_time,
                                   trade.price,
                                   trade.quantity,
                                   trade.trade_amount});

        // 刷新Position界面显示
        OrderSellTraded(acc_index, TradeData{
                                       trade.order_xtp_id,
                                       trade.ticker,
                                       trade.side,
                                       trade_time,
                                       trade.price,
                                       trade.quantity,
                                       trade.trade_amount});
    }
#pragma endregion

#pragma region 故障检测
    void Trader::onerror(const QString &action) const
    {
        XTPRI *error_info = pTraderApi->GetApiLastError();
        qCCritical(XTPTrader) << action << QStringLiteral("交易连接出错") << error_info->error_id << error_info->error_msg;
        // TODO：错误弹窗提示
    }

    void Trader::OnDisconnected(uint64_t session_id, int reason)
    {
        TraderError();
        qCCritical(XTPTrader) << QStringLiteral("错误原因") << reason << QStringLiteral("连接中断，需要重新登录交易服务器并获取最新持仓和挂单信息");
    }

    void Trader::OnError(XTPRI *error_info)
    {
        TraderError();
        qCCritical(XTPTrader) << QStringLiteral("交易服务器错误:") << error_info->error_id << error_info->error_msg;
    }
#pragma endregion

    Trader::Trader(QObject *parent) : QObject(parent)
    {
        auto cfg = Config::get_instance().get_system_config();
        pTraderApi = TraderApi::CreateTraderApi(
            cfg.client_id,
            cfg.log_path.c_str(),
            static_cast<XTP_LOG_LEVEL>(cfg.log_level));
        assert(pTraderApi != nullptr);
        pTraderApi->RegisterSpi(this);

        pTraderApi->SetHeartBeatInterval(cfg.heart_beat_interval);
        pTraderApi->SubscribePublicTopic(static_cast<XTP_TE_RESUME_TYPE>(cfg.resume_type));

        auto cfg_trader = Config::get_instance().get_trader_config();
        pTraderApi->SetSoftwareVersion(cfg_trader.version.c_str());
        pTraderApi->SetSoftwareKey(cfg_trader.software_key.c_str());

        nAccounts = Config::get_instance().get_account_num();
        mSessionIDs.resize(nAccounts);
        mAsset.resize(nAccounts);

        mOrder.business_type = XTP_BUSINESS_TYPE_CASH;
        mOrder.side = XTP_SIDE_SELL;
        mOrder.price_type = XTP_PRICE_LIMIT;
        mOrder.quantity = 0;
        mOrder.position_effect = XTP_POSITION_EFFECT_INIT;
        mOrder.order_client_id = 100;

        // 判断当前是否为中途启动
        time_t crtTime;
        time(&crtTime);

        time_t startTime;
        time(&startTime);
        struct tm *pstm = gmtime(&startTime);
        pstm->tm_hour = 9;
        pstm->tm_min = 25;
        pstm->tm_sec = 0;
        startTime = mktime(pstm);

        is_start_late = crtTime > startTime;
    }

    Trader::~Trader()
    {
        if (pTraderApi)
        {
            pTraderApi->Release();
            pTraderApi = nullptr;
        }
    }

    TraderController::TraderController()
    {
        pTrader = new Trader;
        pTrader->moveToThread(&traderThread);
        connect(&traderThread, &QThread::finished, pTrader, &QObject::deleteLater);
        traderThread.start();
    }

    TraderController::~TraderController()
    {
        traderThread.quit();
        traderThread.wait();
    }
} // namespace HuiFu