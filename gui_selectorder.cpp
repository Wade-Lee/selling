#include "gui_selectorder.h"
#include "ui_gui_selectorder.h"

GuiSelectOrder::GuiSelectOrder(QWidget *parent) : QWidget(parent),
                                                  ui(new Ui::GuiSelectOrder)
{
    ui->setupUi(this);
}

GuiSelectOrder::~GuiSelectOrder()
{
    delete ui;
}

void GuiSelectOrder::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        bool checked = ui->checkBox->isChecked();
        ui->checkBox->setChecked(!checked);
    }
}

void GuiSelectOrder::SetStockCode(const QString &stock_code) const
{
    ui->stockCode->setText(stock_code);
}

void GuiSelectOrder::SetSelect(bool bChecked) const
{
    ui->checkBox->setChecked(bChecked);
}

bool GuiSelectOrder::IsSelected() const
{
    return ui->checkBox->isChecked();
}