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

    size_t Trader::get_account_index(uint64_t session_id) const
    {
        for (size_t i = 0; i < nAccounts; i++)
        {
            if (session_id == mSessionIDs[i])
            {
                return i;
            }
        }

        return nAccounts;
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
        // 找出账户id
        size_t acc_index = get_account_index(session_id);
        assert(acc_index < nAccounts);
        auto cfg = Config::get_instance().get_accounts_config()[acc_index];

        if (error_info == nullptr || error_info->error_id == 0)
        {
            qCInfo(XTPTrader) << cfg.user.c_str() << QStringLiteral("收到仓位：") << position->ticker_name << position->total_qty << position->sellable_qty << is_last;

            // 只向界面传递昨仓信息
            if (position->yesterday_position != 0)
            {
                AccountPositionReceived(acc_index, PositionData{
                                                       position->ticker,
                                                       position->ticker_name,
                                                       (position->market == XTP_MKT_SZ_A) ? XTP_EXCHANGE_SZ : XTP_EXCHANGE_SH,
                                                       position->total_qty,
                                                       position->sellable_qty,
                                                       position->yesterday_position,
                                                       position->avg_price});
            }
            if (is_last)
                AccountPositionFinished();

            // 记录仓位信息
            auto &positions = mAsset[acc_index].positions;
            if (positions.find(position->ticker) != positions.end())
            {
                auto &p = positions.at(position->ticker);
                p.market = position->market;
                p.total_qty = position->total_qty;
                p.sellable_qty = position->sellable_qty;
                p.cost_price = position->avg_price;
                // 中途启动，最新价格、已成交数量、金额和均价会在仓位事件之前接收到订单事件时计算
            }
            else
                positions[position->ticker] = Position{
                    position->market,
                    position->total_qty,
                    position->sellable_qty,
                    position->avg_price,
                    position->avg_price, 0, 0.0, 0.0};
        }
        else
        {
            if (error_info->error_id == 11000350)
            {
                AccountPositionFinished();
                qCInfo(XTPTrader) << cfg.user.c_str() << QStringLiteral("没有持仓");
            }
            else
                qCCritical(XTPTrader) << cfg.user.c_str() << QStringLiteral("查询持仓错误：") << error_info->error_id << error_info->error_msg;
        }
    }

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

    void Trader::OnQueryAsset(XTPQueryAssetRsp *asset_data, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id)
    {
        // 找出账户id
        size_t acc_index = get_account_index(session_id);
        assert(acc_index < nAccounts);
        auto cfg = Config::get_instance().get_accounts_config()[acc_index];

        if (error_info == nullptr || error_info->error_id == 0)
        {
            auto &asset = mAsset[acc_index];
            asset.buying_power = asset_data->buying_power;
            asset.withholding_amount = asset_data->withholding_amount;
            asset.security_asset = 0;
            qCInfo(XTPTrader) << cfg.user.c_str() << QStringLiteral("仓位列表：");
            for (auto &position : asset.positions)
            {
                asset.security_asset += position.second.total_qty * position.second.last_price;
                qCInfo(XTPTrader) << position.first << position.second.total_qty << position.second.last_price;
            }
            qCInfo(XTPTrader) << QStringLiteral("账户资金初始值：") << asset.buying_power << asset.withholding_amount << asset.security_asset;
            AssetReceived(acc_index, AssetData{asset.buying_power, asset.withholding_amount, asset.security_asset});
        }
        else
        {
            qCCritical(XTPTrader) << cfg.user.c_str() << QStringLiteral("查询资产错误：") << error_info->error_id << error_info->error_msg;
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
            // 不是本程序的卖单错误不处理
            if (order_info->order_client_id != ORDER_SELL_CLIENT_ID)
                return;

            int64_t current_date_time = QDateTime::currentDateTime().toString("yyyyMMddhhmmss").toLongLong() * 1000;
            if (current_date_time > order_info->insert_time + 30000)
                return;

            // 找出账户id
            size_t acc_index = get_account_index(session_id);
            assert(acc_index < nAccounts);
            auto cfg = Config::get_instance().get_accounts_config()[acc_index];

            if (order_info != nullptr)
                qCCritical(XTPTrader) << cfg.user.c_str() << QStringLiteral("收到报单事件错误消息：") << order_info->order_xtp_id << error_info->error_id << error_info->error_msg;
            else
                qCCritical(XTPTrader) << cfg.user.c_str() << QStringLiteral("收到报单事件错误消息：") << error_info->error_id << error_info->error_msg;

            OrderError(acc_index, order_info->ticker, error_info->error_msg, error_info->error_id);
            return;
        }

        if (order_info->business_type == XTP_BUSINESS_TYPE_CASH)
        {
            // 找出账户id
            size_t acc_index = get_account_index(session_id);
            assert(acc_index < nAccounts);
            auto cfg = Config::get_instance().get_accounts_config()[acc_index];
            qCInfo(XTPTrader) << cfg.user.c_str() << QStringLiteral("报单状态变化：") << order_info->order_xtp_id << order_info->ticker;
            order_event(acc_index, *order_info);
        }
    }

    void Trader::OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id)
    {
        if (trade_info->business_type != XTP_BUSINESS_TYPE_CASH)
            return;

        // 找出账户id
        size_t acc_index = get_account_index(session_id);
        assert(acc_index < nAccounts);
        auto cfg = Config::get_instance().get_accounts_config()[acc_index];
        qCInfo(XTPTrader) << cfg.user.c_str() << QStringLiteral("报单成交：") << trade_info->order_xtp_id << trade_info->ticker << QStringLiteral("成交价格") << trade_info->price << QStringLiteral("成交数量") << trade_info->quantity;

        if (trade_info->side == XTP_SIDE_BUY)
            order_buy_traded(acc_index, *trade_info);
        else
            order_sell_traded(acc_index, *trade_info);
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

    QString Trader::format_time(int64_t t)
    {
        QString tstr = QString::number(t);
        return QString("%1:%2:%3").arg(tstr.mid(8, 2)).arg(tstr.mid(10, 2)).arg(tstr.mid(12, 2));
    }

    void Trader::order_event(size_t acc_index, const XTPOrderInfo &order)
    {
        switch (order.order_status)
        {
        case XTP_ORDER_STATUS_INIT:
            qCInfo(XTPTrader) << QStringLiteral("初始化");
            return;
        case XTP_ORDER_STATUS_ALLTRADED:
            qCInfo(XTPTrader) << QStringLiteral("全部成交");
            if (order.side == XTP_SIDE_BUY)
                order_buy_traded(acc_index, order);
            break;
        case XTP_ORDER_STATUS_PARTTRADEDQUEUEING:
            qCInfo(XTPTrader) << QStringLiteral("部分成交");
            break;
        case XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING:
            qCInfo(XTPTrader) << QStringLiteral("部分撤单");
            if (order.side == XTP_SIDE_BUY)
                order_buy_canceled(acc_index, order);
            else
                order_sell_canceled(acc_index, order);
            return;
        case XTP_ORDER_STATUS_NOTRADEQUEUEING:
            qCInfo(XTPTrader) << QStringLiteral("未成交");
            if (order.side == XTP_SIDE_BUY)
                order_buy_inited(acc_index, order);
            else
                order_sell_inited(acc_index, order);
            return;
        case XTP_ORDER_STATUS_CANCELED:
            qCInfo(XTPTrader) << QStringLiteral("已撤单");
            if (order.side == XTP_SIDE_BUY)
                order_buy_canceled(acc_index, order);
            else
                order_sell_canceled(acc_index, order);
            break;
        case XTP_ORDER_STATUS_REJECTED:
            qCInfo(XTPTrader) << QStringLiteral("已拒绝");
            OrderRefused(acc_index, order.ticker);
            return;
        case XTP_ORDER_STATUS_UNKNOWN:
            qCInfo(XTPTrader) << QStringLiteral("未知订单状态");
            return;
        default:
            qCInfo(XTPTrader) << QStringLiteral("未知订单状态");
            return;
        }
    }

    void Trader::order_buy_inited(size_t acc_index, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString insert_time = format_time(order.insert_time);

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
            positions[stock_code] = Position{order.market, 0, 0, order.price, order.price, 0, 0.0, 0.0};
        }
        // 在仓位下新建一个挂单
        positions.at(stock_code)
            .resting_orders[order.order_xtp_id] = RestingOrder{
            order.side,
            order.quantity,
            order.price, 0.0};

        // 计算挂单预扣费用
        double order_amount = order.price * order.quantity;
        double holding_amount = max(order_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order_amount * WITHHOLD_BUY_RATE;
        // 更新资金信息
        auto &asset = mAsset[acc_index];
        asset.buying_power -= (order_amount + holding_amount);
        asset.withholding_amount += (order_amount + holding_amount);
        // 刷新Asset界面显示
        qCInfo(XTPTrader) << QStringLiteral("买入证券资产挂单时：") << asset.buying_power << asset.withholding_amount << asset.security_asset;
        AssetReceived(acc_index, AssetData{asset.buying_power, asset.withholding_amount, asset.security_asset});
    }

    void Trader::order_sell_inited(size_t acc_index, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString insert_time = format_time(order.insert_time);

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
        if (positions.find(stock_code) == positions.end())
        {
            // 中途重启收到该股票的第一个挂单，新建一个
            positions[stock_code] = Position{order.market, 0, order.quantity, order.price, order.price, 0, 0.0, 0.0};
        }
        // 减去仓位的可卖数量
        positions.at(stock_code).sellable_qty -= order.quantity;
        // 在仓位下新建一个挂单
        positions.at(stock_code)
            .resting_orders[order.order_xtp_id] = RestingOrder{
            order.side,
            order.quantity,
            order.price, 0.0};
    }

    void Trader::order_buy_canceled(size_t acc_index, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString cancel_time = format_time(order.cancel_time);

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

        try
        {
            // 找到账户仓位
            QString stock_code{order.ticker};
            auto &positions = mAsset[acc_index].positions;
            auto &position = positions.at(stock_code);
            // 找到仓位下撤销的挂单
            auto &resting_order = position.resting_orders.at(order.order_xtp_id);
            resting_order.quantity -= order.qty_left; // 减少挂单数量
            // 挂单全部撤销时删除
            if (resting_order.quantity == 0)
                position.resting_orders.erase(order.order_xtp_id);
            // 买入仓位全部撤销且为空时删除
            if (position.total_qty == 0 && position.resting_orders.size() == 0)
                positions.erase(stock_code);

            // 计算挂单预扣费用
            double order_amount = order.price * order.quantity;
            double holding_amount = max(order_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order_amount * WITHHOLD_BUY_RATE;
            // 更新资金信息
            auto &asset = mAsset[acc_index];
            asset.buying_power += (order_amount + holding_amount);
            asset.withholding_amount -= (order_amount + holding_amount);

            if (order.qty_traded != 0)
            {
                // 计算买单成交预扣费用
                holding_amount = max(order.trade_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order.trade_amount * WITHHOLD_BUY_RATE;
                // 更新资金信息
                asset.buying_power -= (order.trade_amount + holding_amount);
            }

            // 刷新Asset界面显示
            qCInfo(XTPTrader) << QStringLiteral("买入证券资产撤单时：") << asset.buying_power << asset.withholding_amount << asset.security_asset;
            AssetReceived(acc_index, AssetData{asset.buying_power, asset.withholding_amount, asset.security_asset});
        }
        catch (const std::exception &e)
        {
            qCCritical(XTPTrader) << QStringLiteral("买入证券资产撤单时越界错误：") << e.what();
        }
    }

    void Trader::order_sell_canceled(size_t acc_index, const XTPOrderInfo &order)
    {
        // 清洗时间
        QString cancel_time = format_time(order.cancel_time);

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

        try
        {
            // 找到账户仓位
            QString stock_code{order.ticker};
            auto &positions = mAsset[acc_index].positions;
            auto &position = positions.at(stock_code);
            // 找到仓位下撤销的挂单
            auto &resting_order = position.resting_orders.at(order.order_xtp_id);
            resting_order.quantity -= order.qty_left; // 减少挂单数量
            // 挂单全部撤销时删除
            if (resting_order.quantity == 0)
                position.resting_orders.erase(order.order_xtp_id);
            // 增加仓位的可卖数量
            position.sellable_qty += order.qty_left;
        }
        catch (const std::exception &e)
        {
            qCCritical(XTPTrader) << QStringLiteral("卖出证券资产撤单时越界错误：") << e.what();
        }
    }

    void Trader::order_buy_traded(size_t acc_index, const XTPTradeReport &trade)
    {
        // 清洗时间
        QString trade_time = format_time(trade.trade_time);

        // 读取股票名称
        QString stock_code{trade.ticker};

        try
        {
            // 找到账户仓位
            auto &position = mAsset[acc_index].positions.at(stock_code);
            // 找到仓位下成交的挂单
            auto &resting_order = position.resting_orders.at(trade.order_xtp_id);
            resting_order.quantity -= trade.quantity; // 减少挂单数量
            // 更新仓位信息
            position.total_qty += trade.quantity;                             // 增加仓位数量
            position.last_price = trade.price;                                // 更新仓位价格
            position.trade_qty += trade.quantity;                             // 增加成交数量
            position.trade_amount += trade.trade_amount;                      // 增加成交总金额
            position.cost_price = position.trade_amount / position.trade_qty; // 计算成本价
            position.trade_avg_price = position.cost_price;                   // 计算成交均价

            // 刷新Trade界面显示
            OrderTraded(acc_index, TradeData{
                                       trade.order_xtp_id,
                                       trade.ticker,
                                       trade.side,
                                       trade_time,
                                       trade.price,
                                       trade.quantity,
                                       trade.trade_amount,
                                       position.trade_avg_price});

            // 更新资金信息
            auto &asset = mAsset[acc_index];
            double order_amount = trade.quantity * resting_order.price;
            asset.buying_power += order_amount;
            asset.buying_power -= trade.trade_amount;
            asset.withholding_amount -= order_amount;

            // 挂单全部成交时删除
            if (resting_order.quantity == 0)
                position.resting_orders.erase(trade.order_xtp_id);

            // 刷新Asset界面显示
            asset.security_asset = 0;
            for (auto &p : asset.positions)
                asset.security_asset += p.second.total_qty * p.second.last_price;
            qCInfo(XTPTrader) << QStringLiteral("买入证券成交时：") << asset.buying_power << asset.withholding_amount << asset.security_asset;
            AssetReceived(acc_index, AssetData{asset.buying_power, asset.withholding_amount, asset.security_asset});
        }
        catch (const std::exception &e)
        {
            qCCritical(XTPTrader) << QStringLiteral("买入证券资产成交时越界错误：") << e.what();
        }
    }

    void Trader::order_buy_traded(size_t acc_index, const XTPOrderInfo &order)
    {
        // 计算预扣费用
        double order_amount = order.quantity * order.price;
        double holding_amount = max(order_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order_amount * WITHHOLD_BUY_RATE;
        // 更新资金信息
        auto &asset = mAsset[acc_index];
        asset.buying_power += holding_amount;
        asset.withholding_amount -= holding_amount;
        // 计算买入实际预扣费用
        holding_amount = max(order.trade_amount * WITHHOLD_BUY_SPECIAL_RATE, 6.0) + order.trade_amount * WITHHOLD_BUY_RATE;
        // 更新资金信息
        asset.buying_power -= holding_amount;

        // 刷新Asset界面显示
        qCInfo(XTPTrader) << QStringLiteral("买入证券全部成交时：") << asset.buying_power << asset.withholding_amount << asset.security_asset;
        AssetReceived(acc_index, AssetData{asset.buying_power, asset.withholding_amount, asset.security_asset});
    }

    void Trader::order_sell_traded(size_t acc_index, const XTPTradeReport &trade)
    {
        // 清洗时间
        QString trade_time = format_time(trade.trade_time);

        // 读取股票名称
        QString stock_code{trade.ticker};

        try
        {
            // 找到账户仓位
            auto &position = mAsset[acc_index].positions.at(stock_code);
            // 找到仓位下成交的挂单
            auto &resting_order = position.resting_orders.at(trade.order_xtp_id);
            // 减少挂单数量
            resting_order.quantity -= trade.quantity;
            // 更新仓位信息
            position.total_qty -= trade.quantity;                                  // 减少仓位数量
            position.last_price = trade.price;                                     // 更新仓位价格
            position.trade_qty += trade.quantity;                                  // 增加成交数量
            position.trade_amount += trade.trade_amount;                           // 增加成交总金额
            position.trade_avg_price = position.trade_amount / position.trade_qty; // 计算成交均价

            // 刷新Order界面显示
            OrderTraded(acc_index, TradeData{
                                       trade.order_xtp_id,
                                       trade.ticker,
                                       trade.side,
                                       trade_time,
                                       trade.price,
                                       trade.quantity,
                                       trade.trade_amount,
                                       position.trade_avg_price});

            // 计算之前卖出预扣费用
            double holding_amount = 0.0;
            if (resting_order.trade_amount > numeric_limits<double>::epsilon())
                holding_amount = max(resting_order.trade_amount * WITHHOLD_SELL_SPECIAL_RATE, 8.0) + resting_order.trade_amount * WITHHOLD_SELL_RATE;
            // 更新资金信息
            auto &asset = mAsset[acc_index];
            asset.buying_power += holding_amount + trade.trade_amount;

            // 增加挂单成交总额
            resting_order.trade_amount += trade.trade_amount;
            // 计算累计卖出预扣费用
            holding_amount = max(resting_order.trade_amount * WITHHOLD_SELL_SPECIAL_RATE, 8.0) + resting_order.trade_amount * WITHHOLD_SELL_RATE;
            // 更新资金信息
            asset.buying_power -= holding_amount;

            // 刷新Asset界面显示
            asset.security_asset = 0;
            for (auto &p : asset.positions)
                asset.security_asset += p.second.total_qty * p.second.last_price;
            qCInfo(XTPTrader) << QStringLiteral("卖出证券成交时：") << asset.buying_power << asset.withholding_amount << asset.security_asset;
            AssetReceived(acc_index, AssetData{asset.buying_power, asset.withholding_amount, asset.security_asset});
        }
        catch (const std::exception &e)
        {
            qCCritical(XTPTrader) << QStringLiteral("卖出证券资产成交时越界错误：") << e.what();
        }
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
        mOrder.order_client_id = ORDER_SELL_CLIENT_ID;
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