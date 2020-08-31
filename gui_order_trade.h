#ifndef GUI_ORDER_TRADE_H
#define GUI_ORDER_TRADE_H

#include "base.h"

#include <QWidget>

namespace Ui
{
    class GuiOrderTrade;
}

class GuiOrderTrade : public QWidget
{
    Q_OBJECT

public slots:
    void OnTraderLogin(size_t);
    void OnOrderTraded(size_t, const HuiFu::TradeData &);

#pragma region 属性成员
private:
    // 对应账户id
    static size_t ID;
    size_t id;

public:
    int GetID() const { return id; }
#pragma endregion

public:
    explicit GuiOrderTrade(QWidget *parent = nullptr);
    ~GuiOrderTrade();

private:
    Ui::GuiOrderTrade *ui;
};

#endif // GUI_ORDER_TRADE_H
