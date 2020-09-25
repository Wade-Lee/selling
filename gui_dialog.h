#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include <QDialog>
#include <QRect>
#include <QTimer>

namespace Ui
{
    class TipsDialog;
}

class TipsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TipsDialog(const QString &msg, QWidget *parent = nullptr);
    ~TipsDialog();

private:
    Ui::TipsDialog *ui;

    QRect mRect;
    QTimer *pTimer;
};

#endif // TIPS_DIALOG_H
