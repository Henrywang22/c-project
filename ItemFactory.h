#pragma once
#include <string>
#include <vector>
#include "Item.h"
#include "Weapon.h"
#include "GameConfig.h"

/**
 * ItemFactory - 物品工厂类
 * 职责：统一管理所有物品的实例化逻辑，将数值配置与业务逻辑隔离。
 */
class ItemFactory {
public:
    // ==========================================
    // 1. 消耗品工厂方法
    // ==========================================

    // 创建基础食物 (干粮)
    static Item* createFood() {
        return new FoodItem("航海干粮", Config::PRICE_FOOD_RATION, Config::HEAL_FOOD_RATION);
    }

    // 创建修理包 (支持 Tier 1, 2, 3)
    static Item* createRepairKit(int tier) {
        if (tier == 2) {
            return new RepairKit("中级修理包", Config::PRICE_REPAIR_T2, Config::HEAL_REPAIR_T2);
        } else if (tier == 3) {
            return new RepairKit("高级修理包", Config::PRICE_REPAIR_T3, Config::HEAL_REPAIR_T3);
        }
        // 默认为 T1
        return new RepairKit("初级修理包", Config::PRICE_REPAIR_T1, Config::HEAL_REPAIR_T1);
    }

    // ==========================================
    // 2. 武器工厂方法
    // ==========================================

    /**
     * 根据类型名称和等级创建武器
     * @param type: "Rod", "Net", "Harpoon", "Pistol", "Shotgun"
     * @param tier: 1, 2, 3
     */
    static Weapon* createWeapon(const std::string& type, int tier) {
        if (type == "Rod") {
            if (tier == 2) return new Weapon("进阶鱼竿", Config::PRICE_ROD_T2, Config::DMG_ROD_T2, Config::DUR_ROD_T2, Config::RNG_ROD, Config::CONS_ROD);
            if (tier == 3) return new Weapon("传世鱼竿", Config::PRICE_ROD_T3, Config::DMG_ROD_T3, Config::DUR_ROD_T3, Config::RNG_ROD, Config::CONS_ROD);
            return new Weapon("基础鱼竿", Config::PRICE_ROD_T1, Config::DMG_ROD_T1, Config::DUR_ROD_T1, Config::RNG_ROD, Config::CONS_ROD);
        }
        
        if (type == "Net") {
            if (tier == 2) return new Weapon("加固渔网", Config::PRICE_NET_T2, Config::DMG_NET_T2, Config::DUR_NET_T2, Config::RNG_NET, Config::CONS_NET);
            if (tier == 3) return new Weapon("捕捉大师渔网", Config::PRICE_NET_T3, Config::DMG_NET_T3, Config::DUR_NET_T3, Config::RNG_NET, Config::CONS_NET);
            return new Weapon("普通渔网", Config::PRICE_NET_T1, Config::DMG_NET_T1, Config::DUR_NET_T1, Config::RNG_NET, Config::CONS_NET);
        }

        if (type == "Harpoon") {
            if (tier == 2) return new Weapon("合金鱼叉", Config::PRICE_HARPOON_T2, Config::DMG_HARPOON_T2, Config::DUR_HARPOON_T2, Config::RANGE_HARPOON, Config::CONS_HARPOON);
            if (tier == 3) return new Weapon("海王鱼叉", Config::PRICE_HARPOON_T3, Config::DMG_HARPOON_T3, Config::DUR_HARPOON_T3, Config::RANGE_HARPOON, Config::CONS_HARPOON);
            return new Weapon("铁制鱼叉", Config::PRICE_HARPOON_T1, Config::DMG_HARPOON_T1, Config::DUR_HARPOON_T1, Config::RANGE_HARPOON, Config::CONS_HARPOON);
        }

        if (type == "Pistol") {
            if (tier == 2) return new Weapon("改良手枪", Config::PRICE_PISTOL_T2, Config::DMG_PISTOL_T2, Config::DUR_PISTOL_T2, Config::RANGE_PISTOL, Config::CONS_PISTOL);
            if (tier == 3) return new Weapon("执法者手枪", Config::PRICE_PISTOL_T3, Config::DMG_PISTOL_T3, Config::DUR_PISTOL_T3, Config::RANGE_PISTOL, Config::CONS_PISTOL);
            return new Weapon("旧式手枪", Config::PRICE_PISTOL_T1, Config::DMG_PISTOL_T1, Config::DUR_PISTOL_T1, Config::RANGE_PISTOL, Config::CONS_PISTOL);
        }

        if (type == "Shotgun") {
            if (tier == 2) return new Weapon("双管猎枪", Config::PRICE_SHOTGUN_T2, Config::DMG_SHOTGUN_T2, Config::DUR_SHOTGUN_T2, Config::RANGE_SHOTGUN, Config::CONS_SHOTGUN);
            if (tier == 3) return new Weapon("破灭者猎枪", Config::PRICE_SHOTGUN_T3, Config::DMG_SHOTGUN_T3, Config::DUR_SHOTGUN_T3, Config::RANGE_SHOTGUN, Config::CONS_SHOTGUN);
            return new Weapon("锈蚀猎枪", Config::PRICE_SHOTGUN_T1, Config::DMG_SHOTGUN_T1, Config::DUR_SHOTGUN_T1, Config::RANGE_SHOTGUN, Config::CONS_SHOTGUN);
        }

        return nullptr;
    }

    // ==========================================
    // 3. 升级凭证工厂方法
    // ==========================================

    // 创建属性升级 (Speed, Durability, Stamina)
    static Item* createAttributeUpgrade(const std::string& attr, int tier) {
        if (attr == "Speed") {
            if (tier == 2) return new SpeedUpgrade("中级螺旋桨", Config::PRICE_UPG_SPEED_T2, Config::VAL_UPG_SPEED_T2);
            if (tier == 3) return new SpeedUpgrade("核动力引擎", Config::PRICE_UPG_SPEED_T3, Config::VAL_UPG_SPEED_T3);
            return new SpeedUpgrade("初级润滑油", Config::PRICE_UPG_SPEED_T1, Config::VAL_UPG_SPEED_T1);
        }
        
        if (attr == "Durability") {
            if (tier == 2) return new MaxDurabilityUpgrade("钢板加固", Config::PRICE_UPG_DUR_T2, Config::VAL_UPG_DUR_T2);
            if (tier == 3) return new MaxDurabilityUpgrade("钛合金装甲", Config::PRICE_UPG_DUR_T3, Config::VAL_UPG_DUR_T3);
            return new MaxDurabilityUpgrade("木板补强", Config::PRICE_UPG_DUR_T1, Config::VAL_UPG_DUR_T1);
        }

        if (attr == "Stamina") {
            if (tier == 2) return new MaxStaminaUpgrade("中级体能训练", Config::PRICE_UPG_STAMINA_T2, Config::VAL_UPG_STAMINA_T2);
            if (tier == 3) return new MaxStaminaUpgrade("海军陆战队特训", Config::PRICE_UPG_STAMINA_T3, Config::VAL_UPG_STAMINA_T3);
            return new MaxStaminaUpgrade("初级耐力跑步", Config::PRICE_UPG_STAMINA_T1, Config::VAL_UPG_STAMINA_T1);
        }

        return nullptr;
    }

    // 创建武器强化套件 (针对当前手持武器)
    static Item* createWeaponUpgrade(int tier) {
        if (tier == 2) return new WeaponUpgrade("中级武器打磨", Config::PRICE_UPG_WEAPON_T2, Config::VAL_UPG_WPN_DMG_T2, Config::VAL_UPG_WPN_DUR_T2);
        if (tier == 3) return new WeaponUpgrade("大师级武器重铸", Config::PRICE_UPG_WEAPON_T3, Config::VAL_UPG_WPN_DMG_T3, Config::VAL_UPG_WPN_DUR_T3);
        return new WeaponUpgrade("初级武器磨刀石", Config::PRICE_UPG_WEAPON_T1, Config::VAL_UPG_WPN_DMG_T1, Config::VAL_UPG_WPN_DUR_T1);
    }
};