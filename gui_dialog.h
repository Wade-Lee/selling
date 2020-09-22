#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include <QDialog>
// #include <QPropertyAnimation>
#include <QRect>
// #include <QPoint>
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

    // QPropertyAnimation *pAnimation;
    QRect mRect;
    QTimer *pTimer;

    //     void showAnimation();
    // private slots:
    //     void closeAnimation();
    //     void clearAll();
};

#endif // TIPS_DIALOG_H
