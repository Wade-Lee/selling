#include "gui_base.h"

#include <QSpinBox>
#include <QDoubleSpinBox>

void GuiBase::AddReadOnlySpinBox(QTableWidget &table, int row, int column, long v)
{
    QSpinBox *rosb = new QSpinBox();
    rosb->setButtonSymbols(QAbstractSpinBox::NoButtons);
    rosb->setEnabled(false);
    rosb->setWrapping(false);
    rosb->setFrame(false);
    rosb->setReadOnly(true);
    rosb->setKeyboardTracking(false);
    rosb->setMaximum(999999999);
    rosb->setValue(v);
    table.setCellWidget(row, column, rosb);
}

void GuiBase::AddReadOnlyDoubleSpinBox(QTableWidget &table, int row, int column, double v)
{
    QDoubleSpinBox *rodsb = new QDoubleSpinBox();
    rodsb->setButtonSymbols(QAbstractSpinBox::NoButtons);
    rodsb->setEnabled(false);
    rodsb->setWrapping(false);
    rodsb->setFrame(false);
    rodsb->setReadOnly(true);
    rodsb->setKeyboardTracking(false);
    rodsb->setMaximum(999999999);
    rodsb->setValue(v);
    table.setCellWidget(row, column, rodsb);
}
