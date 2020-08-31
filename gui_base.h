#ifndef GUI_BASE_H
#define GUI_BASE_H

#include <QTableWidget>

namespace Ui
{
    class GuiBase;
}

class GuiBase
{
protected:
    size_t id;

public:
    static void AddReadOnlySpinBox(QTableWidget &table, int row, int column, long v);
    static void AddReadOnlyDoubleSpinBox(QTableWidget &table, int row, int column, double v);
};
#endif