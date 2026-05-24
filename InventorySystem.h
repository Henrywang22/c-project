#pragma once

#include <vector>
#include <string>
#include "Weapon.h"
#include "GameConfig.h"

class Player;

// ============================================================
// InventorySystem
// D 模块专用背包系统
//
// 当前负责：
// 1. 物品背包：干粮、船体修理包、紧急装备修理工具
// 2. 装备背包：鱼竿、渔网、鱼叉、手枪、猎枪
// 3. 当前装备选择
// 4. 装备强化 / 修复
//
// 说明：
// 目前先不改 Player / GameManager 的接口。
// 但 InventorySystem 在选择当前装备时，会顺手调用 Player::equipWeapon()
// 这样可以尽量兼容旧代码。
// ============================================================

enum class InventoryItemType {
    Food,
    ShipRepairT1,
    ShipRepairT2,
    ShipRepairT3,
    EmergencyWeaponRepair
};

class InventorySystem
{
public:
    static InventorySystem& instance();

    // 禁止复制
    InventorySystem(const InventorySystem&) = delete;
    InventorySystem& operator=(const InventorySystem&) = delete;

    ~InventorySystem();

    // -------------------------
    // 初始化
    // -------------------------
    void initDefaultWeaponIfNeeded();

    // -------------------------
    // 物品背包
    // -------------------------
    bool canAddItem(int count = 1) const;
    bool addItem(InventoryItemType type, int count = 1);
    bool useFood(Player& player);
    bool useShipRepairKit(Player& player, int tier);
    bool useEmergencyWeaponRepair(int weaponIndex);

    int getItemCount(InventoryItemType type) const;
    int getTotalItemCount() const;

    // -------------------------
    // 装备背包
    // -------------------------
    bool canAddWeapon() const;
    bool addWeapon(Weapon* weapon);
    bool replaceWeapon(int index, Weapon* weapon);

    bool selectWeapon(int index);
    Weapon* currentWeapon();
    const Weapon* currentWeapon() const;

    int currentWeaponIndex() const;
    int weaponCount() const;
    int maxWeaponCapacity() const;

    const std::vector<Weapon*>& weapons() const;

    // -------------------------
    // 装备修复 / 强化
    // -------------------------
    bool repairWeaponByPercent(int index, int percent);
    bool repairWeaponToFull(int index);
    bool upgradeWeapon(int index, int damageBoost, int durabilityBoost);

    // -------------------------
    // 工具
    // -------------------------
    void clearAll();

private:
    InventorySystem();

private:
    std::vector<Weapon*> m_weapons;
    int m_currentWeaponIndex = -1;

    int m_foodCount = 0;
    int m_shipRepairT1Count = 0;
    int m_shipRepairT2Count = 0;
    int m_shipRepairT3Count = 0;
    int m_emergencyWeaponRepairCount = 0;
};