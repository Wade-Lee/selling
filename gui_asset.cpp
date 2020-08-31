#include "gui_asset.h"
#include "ui_gui_asset.h"

using namespace HuiFu;

size_t GuiAsset::ID = 0;

GuiAsset::GuiAsset(QWidget *parent) : QWidget(parent),
                                      ui(new Ui::GuiAsset),
                                      id(GuiAsset::ID++)
{
    ui->setupUi(this);
}

GuiAsset::~GuiAsset()
{
    delete ui;
}

void GuiAsset::OnAssetReceived(size_t id_, const HuiFu::AssetData &d) const
{
    if (id != id_)
    {
        return;
    }

    ui->totalAsset->setText(QString::number(d.buying_power + d.security_asset + d.withholding_amount, 'f', 2));
    ui->buyingPower->setText(QString::number(d.buying_power, 'f', 2));
    ui->totalStockValue->setText(QString::number(d.security_asset, 'f', 2));
}