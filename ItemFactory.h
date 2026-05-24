#pragma once

#include <string>
#include "Item.h"
#include "Weapon.h"
#include "GameConfig.h"

// ============================================================
// ItemFactory
// 物品 / 装备工厂
//
// 作用：
// 1. 所有商品创建都从这里走。
// 2. 所有数值来自 GameConfig.h。
// 3. ShopDialog 不直接写死武器数值。
// ============================================================

class ItemFactory {
public:
    // ==========================================
    // 1. 消耗品
    // ==========================================

    static Item* createFood()
    {
        return new FoodItem(
            "航海干粮",
            Config::PRICE_FOOD_RATION,
            Config::HEAL_FOOD_RATION
        );
    }

    static Item* createRepairKit(int tier)
    {
        if (tier == 2) {
            return new RepairKit(
                "中级船体修理包",
                Config::PRICE_REPAIR_T2,
                Config::HEAL_REPAIR_T2
            );
        }

        if (tier == 3) {
            return new RepairKit(
                "高级船体修理包",
                Config::PRICE_REPAIR_T3,
                Config::HEAL_REPAIR_T3
            );
        }

        return new RepairKit(
            "初级船体修理包",
            Config::PRICE_REPAIR_T1,
            Config::HEAL_REPAIR_T1
        );
    }

    // ==========================================
    // 2. 装备
    // ==========================================

    static Weapon* createWeapon(const std::string& type, int tier)
    {
        if (type == "Rod") {
            if (tier == 2) {
                return new FishingRod(
                    "进阶鱼竿",
                    Config::PRICE_ROD_T2,
                    Config::DMG_ROD_T2,
                    Config::DUR_ROD_T2,
                    Config::RANGE_ROD,
                    Config::CONS_ROD,
                    "Rod",
                    2,
                    Config::EquipmentRole::FishingTool,
                    Config::FishingMode::QTE,
                    true,
                    false,
                    Config::ROD_FISH_COST_FAIL,
                    Config::ROD_FISH_COST_NORMAL,
                    Config::ROD_FISH_COST_PERFECT,
                    Config::ATTACK_COOLDOWN_ROD_MS
                );
            }

            if (tier == 3) {
                return new FishingRod(
                    "传世鱼竿",
                    Config::PRICE_ROD_T3,
                    Config::DMG_ROD_T3,
                    Config::DUR_ROD_T3,
                    Config::RANGE_ROD,
                    Config::CONS_ROD,
                    "Rod",
                    3,
                    Config::EquipmentRole::FishingTool,
                    Config::FishingMode::QTE,
                    true,
                    false,
                    Config::ROD_FISH_COST_FAIL,
                    Config::ROD_FISH_COST_NORMAL,
                    Config::ROD_FISH_COST_PERFECT,
                    Config::ATTACK_COOLDOWN_ROD_MS
                );
            }

            return new FishingRod(
                "基础鱼竿",
                Config::PRICE_ROD_T1,
                Config::DMG_ROD_T1,
                Config::DUR_ROD_T1,
                Config::RANGE_ROD,
                Config::CONS_ROD,
                "Rod",
                1,
                Config::EquipmentRole::FishingTool,
                Config::FishingMode::QTE,
                true,
                false,
                Config::ROD_FISH_COST_FAIL,
                Config::ROD_FISH_COST_NORMAL,
                Config::ROD_FISH_COST_PERFECT,
                Config::ATTACK_COOLDOWN_ROD_MS
            );
        }

        if (type == "Net") {
            if (tier == 2) {
                return new FishingNet(
                    "加固渔网",
                    Config::PRICE_NET_T2,
                    Config::DMG_NET_T2,
                    Config::DUR_NET_T2,
                    Config::RANGE_NET,
                    Config::CONS_NET,
                    "Net",
                    2,
                    Config::EquipmentRole::FishingTool,
                    Config::FishingMode::Calibration,
                    true,
                    false,
                    Config::NET_FISH_COST_FAIL,
                    Config::NET_FISH_COST_NORMAL,
                    Config::NET_FISH_COST_PERFECT,
                    Config::ATTACK_COOLDOWN_NET_MS
                );
            }

            if (tier == 3) {
                return new FishingNet(
                    "捕捉大师渔网",
                    Config::PRICE_NET_T3,
                    Config::DMG_NET_T3,
                    Config::DUR_NET_T3,
                    Config::RANGE_NET,
                    Config::CONS_NET,
                    "Net",
                    3,
                    Config::EquipmentRole::FishingTool,
                    Config::FishingMode::Calibration,
                    true,
                    false,
                    Config::NET_FISH_COST_FAIL,
                    Config::NET_FISH_COST_NORMAL,
                    Config::NET_FISH_COST_PERFECT,
                    Config::ATTACK_COOLDOWN_NET_MS
                );
            }

            return new FishingNet(
                "普通渔网",
                Config::PRICE_NET_T1,
                Config::DMG_NET_T1,
                Config::DUR_NET_T1,
                Config::RANGE_NET,
                Config::CONS_NET,
                "Net",
                1,
                Config::EquipmentRole::FishingTool,
                Config::FishingMode::Calibration,
                true,
                false,
                Config::NET_FISH_COST_FAIL,
                Config::NET_FISH_COST_NORMAL,
                Config::NET_FISH_COST_PERFECT,
                Config::ATTACK_COOLDOWN_NET_MS
            );
        }

        if (type == "Harpoon") {
            if (tier == 2) {
                return new Harpoon(
                    "合金鱼叉",
                    Config::PRICE_HARPOON_T2,
                    Config::DMG_HARPOON_T2,
                    Config::DUR_HARPOON_T2,
                    Config::RANGE_HARPOON,
                    Config::CONS_HARPOON,
                    "Harpoon",
                    2,
                    Config::EquipmentRole::HybridTool,
                    Config::FishingMode::Calibration,
                    true,
                    true,
                    Config::HARPOON_FISH_COST_FAIL,
                    Config::HARPOON_FISH_COST_NORMAL,
                    Config::HARPOON_FISH_COST_PERFECT,
                    Config::ATTACK_COOLDOWN_HARPOON_MS
                );
            }

            if (tier == 3) {
                return new Harpoon(
                    "海王鱼叉",
                    Config::PRICE_HARPOON_T3,
                    Config::DMG_HARPOON_T3,
                    Config::DUR_HARPOON_T3,
                    Config::RANGE_HARPOON,
                    Config::CONS_HARPOON,
                    "Harpoon",
                    3,
                    Config::EquipmentRole::HybridTool,
                    Config::FishingMode::Calibration,
                    true,
                    true,
                    Config::HARPOON_FISH_COST_FAIL,
                    Config::HARPOON_FISH_COST_NORMAL,
                    Config::HARPOON_FISH_COST_PERFECT,
                    Config::ATTACK_COOLDOWN_HARPOON_MS
                );
            }

            return new Harpoon(
                "铁制鱼叉",
                Config::PRICE_HARPOON_T1,
                Config::DMG_HARPOON_T1,
                Config::DUR_HARPOON_T1,
                Config::RANGE_HARPOON,
                Config::CONS_HARPOON,
                "Harpoon",
                1,
                Config::EquipmentRole::HybridTool,
                Config::FishingMode::Calibration,
                true,
                true,
                Config::HARPOON_FISH_COST_FAIL,
                Config::HARPOON_FISH_COST_NORMAL,
                Config::HARPOON_FISH_COST_PERFECT,
                Config::ATTACK_COOLDOWN_HARPOON_MS
            );
        }

        if (type == "Pistol") {
            if (tier == 2) {
                return new Pistol(
                    "改良手枪",
                    Config::PRICE_PISTOL_T2,
                    Config::DMG_PISTOL_T2,
                    Config::DUR_PISTOL_T2,
                    Config::RANGE_PISTOL,
                    Config::CONS_PISTOL,
                    "Pistol",
                    2,
                    Config::EquipmentRole::AttackWeapon,
                    Config::FishingMode::None,
                    false,
                    true,
                    0,
                    0,
                    0,
                    Config::ATTACK_COOLDOWN_PISTOL_MS
                );
            }

            if (tier == 3) {
                return new Pistol(
                    "执法者手枪",
                    Config::PRICE_PISTOL_T3,
                    Config::DMG_PISTOL_T3,
                    Config::DUR_PISTOL_T3,
                    Config::RANGE_PISTOL,
                    Config::CONS_PISTOL,
                    "Pistol",
                    3,
                    Config::EquipmentRole::AttackWeapon,
                    Config::FishingMode::None,
                    false,
                    true,
                    0,
                    0,
                    0,
                    Config::ATTACK_COOLDOWN_PISTOL_MS
                );
            }

            return new Pistol(
                "旧式手枪",
                Config::PRICE_PISTOL_T1,
                Config::DMG_PISTOL_T1,
                Config::DUR_PISTOL_T1,
                Config::RANGE_PISTOL,
                Config::CONS_PISTOL,
                "Pistol",
                1,
                Config::EquipmentRole::AttackWeapon,
                Config::FishingMode::None,
                false,
                true,
                0,
                0,
                0,
                Config::ATTACK_COOLDOWN_PISTOL_MS
            );
        }

        if (type == "Shotgun") {
            if (tier == 2) {
                return new Shotgun(
                    "双管猎枪",
                    Config::PRICE_SHOTGUN_T2,
                    Config::DMG_SHOTGUN_T2,
                    Config::DUR_SHOTGUN_T2,
                    Config::RANGE_SHOTGUN,
                    Config::CONS_SHOTGUN,
                    "Shotgun",
                    2,
                    Config::EquipmentRole::AttackWeapon,
                    Config::FishingMode::None,
                    false,
                    true,
                    0,
                    0,
                    0,
                    Config::ATTACK_COOLDOWN_SHOTGUN_MS
                );
            }

            if (tier == 3) {
                return new Shotgun(
                    "破灭者猎枪",
                    Config::PRICE_SHOTGUN_T3,
                    Config::DMG_SHOTGUN_T3,
                    Config::DUR_SHOTGUN_T3,
                    Config::RANGE_SHOTGUN,
                    Config::CONS_SHOTGUN,
                    "Shotgun",
                    3,
                    Config::EquipmentRole::AttackWeapon,
                    Config::FishingMode::None,
                    false,
                    true,
                    0,
                    0,
                    0,
                    Config::ATTACK_COOLDOWN_SHOTGUN_MS
                );
            }

            return new Shotgun(
                "锈蚀猎枪",
                Config::PRICE_SHOTGUN_T1,
                Config::DMG_SHOTGUN_T1,
                Config::DUR_SHOTGUN_T1,
                Config::RANGE_SHOTGUN,
                Config::CONS_SHOTGUN,
                "Shotgun",
                1,
                Config::EquipmentRole::AttackWeapon,
                Config::FishingMode::None,
                false,
                true,
                0,
                0,
                0,
                Config::ATTACK_COOLDOWN_SHOTGUN_MS
            );
        }

        return nullptr;
    }

    // ==========================================
    // 3. 属性升级
    // ==========================================

    static Item* createAttributeUpgrade(const std::string& attr, int tier)
    {
        if (attr == "Speed") {
            if (tier == 2) {
                return new SpeedUpgrade("中级螺旋桨", Config::PRICE_UPG_SPEED_T2, Config::VAL_UPG_SPEED_T2);
            }

            if (tier == 3) {
                return new SpeedUpgrade("核动力引擎", Config::PRICE_UPG_SPEED_T3, Config::VAL_UPG_SPEED_T3);
            }

            return new SpeedUpgrade("初级润滑油", Config::PRICE_UPG_SPEED_T1, Config::VAL_UPG_SPEED_T1);
        }

        if (attr == "Durability") {
            if (tier == 2) {
                return new MaxDurabilityUpgrade("钢板加固", Config::PRICE_UPG_DUR_T2, Config::VAL_UPG_DUR_T2);
            }

            if (tier == 3) {
                return new MaxDurabilityUpgrade("钛合金装甲", Config::PRICE_UPG_DUR_T3, Config::VAL_UPG_DUR_T3);
            }

            return new MaxDurabilityUpgrade("木板补强", Config::PRICE_UPG_DUR_T1, Config::VAL_UPG_DUR_T1);
        }

        if (attr == "Stamina") {
            if (tier == 2) {
                return new MaxStaminaUpgrade("中级体能训练", Config::PRICE_UPG_STAMINA_T2, Config::VAL_UPG_STAMINA_T2);
            }

            if (tier == 3) {
                return new MaxStaminaUpgrade("海军陆战队特训", Config::PRICE_UPG_STAMINA_T3, Config::VAL_UPG_STAMINA_T3);
            }

            return new MaxStaminaUpgrade("初级耐力跑步", Config::PRICE_UPG_STAMINA_T1, Config::VAL_UPG_STAMINA_T1);
        }

        return nullptr;
    }

    static Item* createWeaponUpgrade(int tier)
    {
        if (tier == 2) {
            return new WeaponUpgrade(
                "中级装备强化",
                Config::PRICE_UPG_WEAPON_T2,
                Config::VAL_UPG_WPN_DMG_T2,
                Config::VAL_UPG_WPN_DUR_T2
            );
        }

        if (tier == 3) {
            return new WeaponUpgrade(
                "大师级装备重铸",
                Config::PRICE_UPG_WEAPON_T3,
                Config::VAL_UPG_WPN_DMG_T3,
                Config::VAL_UPG_WPN_DUR_T3
            );
        }

        return new WeaponUpgrade(
            "初级装备打磨",
            Config::PRICE_UPG_WEAPON_T1,
            Config::VAL_UPG_WPN_DMG_T1,
            Config::VAL_UPG_WPN_DUR_T1
        );
    }
};