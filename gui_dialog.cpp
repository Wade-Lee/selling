#include "gui_dialog.h"
#include "ui_gui_dialog.h"

#include <QScreen>
#include <QDebug>

TipsDialog::TipsDialog(const QString &msg, QWidget *parent) : QDialog(parent),
                                                              ui(new Ui::TipsDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);

    ui->label->setText(msg);
    mRect = QGuiApplication::primaryScreen()->geometry();
    move(0, mRect.height() - this->height());
    pTimer = new QTimer(this);
    pTimer->start(5000);
    pTimer->setSingleShot(true);
    connect(pTimer, &QTimer::timeout, this, [=]() { this->close(); });
}

TipsDialog::~TipsDialog()
{
    if (pTimer != Q_NULLPTR)
        pTimer->deleteLater();

    delete ui;
}