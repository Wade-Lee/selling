#ifndef GUI_POSITION_H
#define GUI_POSITION_H

#include "base.h"

#include <map>
#include <QWidget>
#include <QTableWidgetItem>

namespace Ui
{
    class GuiPosition;
}

class GuiPosition : public QWidget
{
    Q_OBJECT

public slots:
    void OnTraderLogin(size_t);
    void OnPositionReceived(size_t, const HuiFu::PositionData &);
    void OnPositionQuoteReceived(const HuiFu::TraderMarketData &);
    void OnOrderSellReceived(size_t, const HuiFu::OrderData &);
    void OnOrderSellCanceled(size_t, const HuiFu::CancelData &);
    void OnOrderSellTraded(size_t, const HuiFu::TradeData &);
    void OnOrderBuyTraded(size_t, const HuiFu::TradeData &);

signals:
    void UserSelectPosition(const HuiFu::PositionData &, double) const;
    void PositionReqSubscribe(XTP_EXCHANGE_TYPE, const QString &) const;

#pragma region 属性成员
private:
    // 对应账户id
    static size_t ID;
    size_t id;

    std::map<HuiFu::StockCode, QTableWidgetItem *> mPositions;

public:
    int GetID() const { return id; }
#pragma endregion

public:
    explicit GuiPosition(QWidget *parent = nullptr);
    ~GuiPosition();

private:
    Ui::GuiPosition *ui;

    void insert_row(const HuiFu::StockCode &stock_code, const QString &stock_name, int64_t total_qty, int64_t sellable_qty, double price = 0.0);
};

#endif // GUI_POSITION_H
