#ifndef GUI_TRADETAB_H
#define GUI_TRADETAB_H

#include "base.h"
#include "gui_dialog.h"

#include <QTabWidget>
#include <QTableWidgetItem>

#include <map>
#include <set>

namespace Ui
{
    class GuiTradeTab;
}

class GuiIndexItem : public QTableWidgetItem
{
public:
    explicit GuiIndexItem(const QString txt = QString::number(0)) {}

    bool operator<(const QTableWidgetItem &other) const
    {
        QString l = text();
        QString r = other.text();
        return l.toInt() < r.toInt();
    }
};

class GuiTradeTab : public QTabWidget
{
    Q_OBJECT

protected:
    bool eventFilter(QObject *, QEvent *) override;

public slots:
    void OnTraderLogin(size_t);
    void OnPositionReceived(size_t, const HuiFu::PositionData &);
    void OnPositionQuoteReceived(const HuiFu::TraderMarketData &);
    void OnOrderReceived(size_t, const HuiFu::OrderData &);
    void OnOrderTraded(size_t, const HuiFu::TradeData &);
    void OnOrderCanceled(size_t, const HuiFu::CancelData &);
    void UserSelectPosition(QTableWidgetItem *);

signals:
    void UserReqSellPosition(const HuiFu::OrderReq &) const;
    void PositionReqSubscribe(XTP_EXCHANGE_TYPE, const QString &) const;
    void ReqCancelOrder(size_t, uint64_t) const;

private:
    void add_buy_position(const HuiFu::TradeData &);
    void sub_sell_position(const HuiFu::TradeData &, bool);
    void req_cancel_orders() const;

#pragma region 属性成员
private:
    // 对应账户id
    static size_t ID;
    size_t id;
    std::map<HuiFu::StockCode, QTableWidgetItem *> mPositions, mSellPositions;
    std::map<uint64_t, QTableWidgetItem *> mOrders, mInsertOrders;

    // 是否拥有焦点
    bool focused = false;

public:
    int GetID() const { return id; }

    void SetFocus(int);
    bool HasFocus() const { return focused; }

    std::set<HuiFu::StockCode> GetSellPositions() const;
    void SetSellPositionIndex(HuiFu::StockCode, size_t);
    void UserSelectPosition(size_t) const;
    void UserChooseOrder(bool) const;
#pragma endregion

public:
    explicit GuiTradeTab(QWidget *parent = nullptr);
    ~GuiTradeTab();

private:
    Ui::GuiTradeTab *ui;
    TipsDialog *dlg;

    void insert_position(const HuiFu::StockCode &stock_code, const QString &stock_name, int64_t total_qty, double price = 0.0);
    void insert_position(const HuiFu::StockCode &stock_code, const QString &stock_name, int64_t total_qty, int64_t sellable_qty, double price = 0.0);
};
#endif // GUI_TRADETAB_H