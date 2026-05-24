#pragma once

#include "Item.h"
#include "GameConfig.h"
#include <string>

class Player;

// ============================================================
// Weapon
//
// 现在这里虽然仍然叫 Weapon，但逻辑上已经是“装备 Equipment”。
// 它可以表示：
// 1. 鱼竿、渔网：捕鱼工具
// 2. 鱼叉：捕鱼 + 攻击双用工具
// 3. 手枪、猎枪：攻击武器
// ============================================================

class Weapon : public Item {
protected:
    std::string typeCode;
    int tier;

    Config::EquipmentRole role;
    Config::FishingMode fishingMode;

    bool canFishFlag;
    bool canAttackFlag;

    int damage;
    int maxDurability;
    int currentDurability;
    int range;

    // 攻击命中后消耗的耐久
    int durabilityConsumption;

    // 捕鱼不同结果消耗的耐久
    int fishCostFail;
    int fishCostNormal;
    int fishCostPerfect;

    // 攻击冷却，后续 GameWindow / GameManager 联调时使用
    int attackCooldownMs;

public:
    Weapon(
        std::string specificName,
        int price,
        int dmg,
        int dur,
        int rng,
        int cons,
        std::string typeCode = "Unknown",
        int tier = 1,
        Config::EquipmentRole role = Config::EquipmentRole::AttackWeapon,
        Config::FishingMode fishingMode = Config::FishingMode::None,
        bool canFish = false,
        bool canAttack = true,
        int fishCostFail = 0,
        int fishCostNormal = 0,
        int fishCostPerfect = 0,
        int attackCooldownMs = Config::ATTACK_COOLDOWN_DEFAULT_MS
    );

    virtual ~Weapon() = default;

    // Item 接口
    void use(Player& player) override;

    // 旧攻击接口：为了兼容 GameManager 当前代码保留。
    // 注意：这个接口会立刻扣耐久。
    // 后续联调鼠标攻击时，建议改用 getDamage() + consumeAttackDurability()。
    int fire();

    // 新攻击接口：命中敌人后再调用这个扣耐久。
    bool consumeAttackDurability();

    // 捕鱼接口：捕鱼结束后根据结果扣耐久。
    bool consumeFishingDurability(Config::FishingResult result);
    int getFishingDurabilityCost(Config::FishingResult result) const;

    // 通用耐久扣除
    bool consumeDurability(int amount);

    // 强化 / 修理
    void upgradeStats(int dmgBoost, int durBoost);
    void repairByPercent(int percent);
    void repairFixed(int amount);
    void repairToFull();

    // 状态查询
    bool isBroken() const;
    bool canFish() const;
    bool canAttack() const;

    int getCurrentDur() const;
    int getMaxDur() const;
    int getDamage() const;
    int getRange() const;
    int getDurabilityConsumption() const;
    int getAttackCooldownMs() const;
    int getTier() const;

    Config::EquipmentRole getRole() const;
    Config::FishingMode getFishingMode() const;

    std::string getTypeCode() const;
    std::string getRoleName() const;
    std::string getFishingModeName() const;
};

// ==========================================
// 具体装备子类
// ==========================================

class FishingRod : public Weapon {
public:
    using Weapon::Weapon;
};

class FishingNet : public Weapon {
public:
    using Weapon::Weapon;
};

class Harpoon : public Weapon {
public:
    using Weapon::Weapon;
};

class Pistol : public Weapon {
public:
    using Weapon::Weapon;
};

class Shotgun : public Weapon {
public:
    using Weapon::Weapon;
};