#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <QWidget>

#include "trader.h"
#include "quote.h"
#include "gui_sell.h"
#include "gui_tradetab.h"

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
    bool accounts_connected = true;
    size_t nAccounts;
    size_t nFinishedPositions = 0;
    QList<GuiSell *> gui_sells;
    QList<GuiTradeTab *> gui_trades;

    void connect_accounts();
    void user_select_position_by_key(size_t) const;

protected:
    void keyPressEvent(QKeyEvent *) override;

public slots:
    void OnQuoteError() const;
    void OnTraderError() const;
    void OnSellReqSyncStockCode(size_t, const QString &) const;
    void OnSellReqSyncStockPrice(size_t, double) const;
    void OnSellReqSyncStockInfo(size_t, const StockSellInfo &) const;
    void OnSellReqSelling(size_t, const QString &, const QString &, double, int64_t) const;
    void OnAccountPositionFinished();

signals:
    void SellReqSelling(size_t, const QString &, double, int64_t) const;
#pragma endregion

#pragma region 关联事件
private:
    void register_bind() const;
#pragma endregion

public:
    explicit GuiMain(const QuoteController &, const TraderController &, QWidget *parent = nullptr);
    ~GuiMain();

private:
    Ui::GuiMain *ui;

    Trader *pTrader;
    Quote *pQuote;
};
#endif // GUI_MAIN_H
