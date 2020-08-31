#ifndef GUI_ORDER_INSERT_H
#define GUI_ORDER_INSERT_H

#include "base.h"

#include <QWidget>
#include <map>

namespace Ui
{
    class GuiOrderInsert;
}

class GuiOrderInsert : public QWidget
{
    Q_OBJECT

public slots:
    void OnTraderLogin(size_t);
    void OnOrderReceived(size_t, const HuiFu::OrderData &);
    void OnOrderTraded(size_t, const HuiFu::TradeData &);
    void OnOrderCanceled(size_t, const HuiFu::CancelData &);

#pragma region 属性成员
private:
    // 对应账户id
    static size_t ID;
    size_t id;
    std::map<uint64_t, int> mOrders;

public:
    int GetID() const { return id; }
#pragma endregion

public:
    explicit GuiOrderInsert(QWidget *parent = nullptr);
    ~GuiOrderInsert();

private:
    Ui::GuiOrderInsert *ui;
};

#endif // GUI_ORDER_INSERT_H
