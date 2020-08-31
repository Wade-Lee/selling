#ifndef GUI_SELECTORDER_H
#define GUI_SELECTORDER_H

#include <QWidget>
#include <QMouseEvent>

namespace Ui
{
    class GuiSelectOrder;
}

class GuiSelectOrder : public QWidget
{
    Q_OBJECT

public:
    explicit GuiSelectOrder(QWidget *parent = nullptr);
    ~GuiSelectOrder();

    void SetStockCode(const QString &stock_code) const;
    void SetSelect(bool) const;
    bool IsSelected() const;

protected:
    virtual void mousePressEvent(QMouseEvent *);

private:
    Ui::GuiSelectOrder *ui;
};

#endif // GUI_SELECTORDER_H
