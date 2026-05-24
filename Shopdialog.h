#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "Player.h"
#include "Weapon.h"
#include "Item.h"
#include "ItemFactory.h"
#include "InventorySystem.h"
#include "GameConfig.h"

// ============================================================
// ShopDialog — 补给商店
//
// 当前实现：
// 1. 消耗品购买后进入物品背包，不立即使用。
// 2. 装备购买后进入装备背包。
// 3. 装备背包最多 3 把，满了可以选择替换。
// 4. 商店中可以选择装备强化对象。
// 5. 商店中可以进行装备修复服务。
// 6. 右侧显示当前物品背包和装备背包。
// ============================================================

class ShopDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShopDialog(QWidget* parent = nullptr);
    ~ShopDialog();

private:
    // UI 构建
    void setupUI();
    QWidget* makeConsumableSection();
    QWidget* makeWeaponSection();
    QWidget* makeUpgradeSection();
    QWidget* makeBackpackSection();

    // 购买逻辑
    void buyBackpackItem(InventoryItemType type, int price, const QString& displayName);
    void buyWeapon(Weapon* weapon);
    void buyAndUseAttributeUpgrade(Item* item);

    // 商店服务
    void buyWeaponUpgrade(int tier);
    void buyShopWeaponRepair();

    // 背包使用
    void useFoodFromBackpack();
    void useShipRepairFromBackpack(int tier);
    void useEmergencyWeaponRepairFromBackpack();

    // 装备选择
    int askWeaponIndex(const QString& title, const QString& label, bool allowBroken = true);
    int askReplaceWeaponIndex();
    void selectWeaponFromBackpack(int index);

    // UI 更新
    void updateCoinsLabel();
    void refreshBackpackUI();

    QString weaponDisplayText(const Weapon* weapon, int index) const;

private:
    QLabel* m_coinsLabel = nullptr;

    QLabel* m_itemBagLabel = nullptr;
    QVBoxLayout* m_itemBagLayout = nullptr;

    QLabel* m_weaponBagLabel = nullptr;
    QVBoxLayout* m_weaponBagLayout = nullptr;
};