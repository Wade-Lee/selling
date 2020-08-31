#ifndef GUI_ASSET_H
#define GUI_ASSET_H

#include "base.h"

#include <QWidget>

namespace Ui
{
    class GuiAsset;
}

class GuiAsset : public QWidget
{
    Q_OBJECT

public:
    explicit GuiAsset(QWidget *parent = nullptr);
    ~GuiAsset();

public slots:
    void OnAssetReceived(size_t, const HuiFu::AssetData &) const;

#pragma region 属性成员
private:
    // 对应账户id
    static size_t ID;
    size_t id;

public:
    size_t GetID() const { return id; }
#pragma endregion

private:
    Ui::GuiAsset *ui;
};

#endif // GUI_ASSET_H
