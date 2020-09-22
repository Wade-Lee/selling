#include "gui_dialog.h"
#include "ui_gui_dialog.h"

#include <QScreen>
#include <QDebug>

TipsDialog::TipsDialog(const QString &msg, QWidget *parent) : QDialog(parent),
                                                              ui(new Ui::TipsDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint & Qt::WindowStaysOnTopHint);

    ui->label->setText(msg);
    mRect = QGuiApplication::primaryScreen()->geometry();
    move((mRect.width() - this->width()), mRect.height() - this->height());
    pTimer = new QTimer(this);
    pTimer->start(2000);
    pTimer->setSingleShot(true);
    connect(pTimer, &QTimer::timeout, this, [=]() { this->close(); });

    setAttribute(Qt::WA_DeleteOnClose);

    // showAnimation();
}

TipsDialog::~TipsDialog()
{
    if (pTimer != Q_NULLPTR)
        pTimer->deleteLater();

    delete ui;
}

// void TipsDialog::showAnimation()
// {
//     pAnimation = new QPropertyAnimation(this, "pos");
//     pAnimation->setDuration(2000);
//     pAnimation->setStartValue(QPoint(this->x(), this->y()));
//     mRect = QGuiApplication::primaryScreen()->geometry();
//     pAnimation->setEndValue(QPoint((mRect.width() - this->width()), (mRect.height() - this->height())));
//     pAnimation->start();

//     pTimer = new QTimer();
//     connect(pTimer, SIGNAL(timeout()), this, SLOT(closeAnimation()));
//     pTimer->start(4000);
// }

// void TipsDialog::closeAnimation()
// {
//     pTimer->stop();
//     disconnect(pTimer, SIGNAL(timeout()), this, SLOT(closeAnimation()));
//     delete pTimer;
//     pTimer = NULL;

//     pAnimation->setStartValue(QPoint(this->x(), this->y()));
//     mRect = QGuiApplication::primaryScreen()->geometry();
//     pAnimation->setEndValue(QPoint((mRect.width() - this->width()), mRect.height()));
//     pAnimation->start();

//     connect(pAnimation, SIGNAL(finished()), this, SLOT(clearAll()));
// }

// void TipsDialog::clearAll()
// {
//     disconnect(pAnimation, SIGNAL(finished()), this, SLOT(clearAll()));
//     delete pAnimation;
//     pAnimation = NULL;
//     close();
// }