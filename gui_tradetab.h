#ifndef GUI_TRADETAB_H
#define GUI_TRADETAB_H

#include <QTabWidget>
#include <QKeyEvent>

namespace Ui
{
    class GuiTradeTab;
}

class GuiTradeTab : public QTabWidget
{
    Q_OBJECT

public:
    explicit GuiTradeTab(QWidget *parent = nullptr);
    ~GuiTradeTab();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    Ui::GuiTradeTab *ui;
};

#endif // GUI_TRADETAB_H
