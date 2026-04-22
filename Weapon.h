#pragma once
#include "Item.h"
#include "GameConfig.h" // 必须引入配置表来获取数值
#include <string>

// ---------------- 武器基类 ----------------
class Weapon : public Item {
protected:
    int damage;             
    int maxDurability;      
    int currentDurability;  
    int range;              
    int durabilityConsumption; // 每次攻击损耗的耐久值

public:
    // 基类构造函数：接收 6 个参数
    Weapon(std::string specificName, int price, int dmg, int dur, int rng, int cons)
        : Item(specificName, price), damage(dmg), maxDurability(dur), 
          currentDurability(dur), range(rng), durabilityConsumption(cons) {}

    virtual ~Weapon() = default;

    void use(Player& player) override;

    // 修改：让 fire() 根据配置的损耗值扣除耐久
    virtual int fire() {
        if (currentDurability > 0) {
            currentDurability -= durabilityConsumption; // 使用自定义损耗率
            if (currentDurability < 0) currentDurability = 0; // 防呆：不扣成负数
            return damage;
        }
        return 0; 
    }

    void upgradeStats(int dmgBoost, int durBoost) {
        damage += dmgBoost;
        maxDurability += durBoost;
        currentDurability += durBoost; 
    }

    // 状态查询接口
    bool isBroken() const { return currentDurability <= 0; }
    int getCurrentDur() const { return currentDurability; }
    int getMaxDur() const { return maxDurability; }
    int getDamage() const { return damage; }
    int getRange() const { return range; }
};

// ==========================================
// 4. 具体武器子类实现 (自动对齐 GameConfig 中的数值)
// ==========================================

// 鱼竿
class FishingRod : public Weapon {
public:
    FishingRod() : Weapon("鱼竿", Config::PRICE_ROD_T1, Config::DMG_ROD_T1, 
                          Config::DUR_ROD_T1, Config::RNG_ROD, Config::CONS_ROD) {}
};

// 渔网
class FishingNet : public Weapon {
public:
    FishingNet() : Weapon("渔网", Config::PRICE_NET_T1, Config::DMG_NET_T1, 
                          Config::DUR_NET_T1, Config::RNG_NET, Config::CONS_NET) {}
};

// 鱼叉
class Harpoon : public Weapon {
public:
    Harpoon() : Weapon("鱼叉", Config::PRICE_HARPOON_T1, Config::DMG_HARPOON_T1, 
                       Config::DUR_HARPOON_T1, Config::RANGE_HARPOON, Config::CONS_HARPOON) {}
};

// 手枪
class Pistol : public Weapon {
public:
    Pistol() : Weapon("手枪", Config::PRICE_PISTOL_T1, Config::DMG_PISTOL_T1, 
                      Config::DUR_PISTOL_T1, Config::RANGE_PISTOL, Config::CONS_PISTOL) {}
};

// 猎枪
class Shotgun : public Weapon {
public:
    Shotgun() : Weapon("猎枪", Config::PRICE_SHOTGUN_T1, Config::DMG_SHOTGUN_T1, 
                       Config::DUR_SHOTGUN_T1, Config::RANGE_SHOTGUN, Config::CONS_SHOTGUN) {}
};