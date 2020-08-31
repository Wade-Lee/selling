#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <QWidget>
#include <QTableWidget>

#include "trader.h"
#include "quote.h"
#include "gui_sell.h"

using namespace HuiFu;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class GuiMain;
}
QT_END_NAMESPACE

class GuiMain : public QWidget
{
    Q_OBJECT

#pragma region 关联账户
private:
    bool accounts_connected;
    size_t nAccounts;
    QList<GuiSell *> gui_sells;

public slots:
    void OnQuoteError() const;
    void OnTraderError() const;
    void OnAccountConnected(bool);
    void OnSellReqSyncStockCode(size_t, const QString &) const;
    void OnSellReqSyncStockInfo(size_t, const QString &, const QString &) const;
    void OnSellReqSyncStockPrice(size_t, double) const;
    void OnSellReqSyncSellQty(size_t, int) const;
    void OnSellReqSyncSellQtyText(size_t, const QString &) const;
#pragma endregion

#pragma region 关联事件
private:
    void register_bind() const;
#pragma endregion

public:
    explicit GuiMain(QWidget *parent = nullptr);
    ~GuiMain();

private:
    Ui::GuiMain *ui;

    std::unique_ptr<Trader> pTrader;
    std::unique_ptr<Quote> pQuote;
};
#endif // GUI_MAIN_H
