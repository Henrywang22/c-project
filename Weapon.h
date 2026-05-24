#pragma once

#include "Item.h"
#include "GameConfig.h"
#include <string>

class Player;

// ============================================================
// Weapon
//
// 注意：虽然类名仍然叫 Weapon，方便兼容原项目，
// 但现在它实际承担的是“装备 Equipment”的功能。
//
// 它可以表示：
// 1. 鱼竿、渔网：捕鱼工具
// 2. 鱼叉：捕鱼 + 攻击双用工具
// 3. 手枪、猎枪：攻击武器
// ============================================================

class Weapon : public Item {
protected:
    // 装备类型代码，例如 Rod / Net / Harpoon / Pistol / Shotgun
    // 存档时依靠这个字段重建装备
    std::string typeCode;

    // 装备等级，例如 T1 / T2 / T3
    int tier;

    // 装备定位：捕鱼工具 / 攻击武器 / 双用工具
    Config::EquipmentRole role;

    // 捕鱼方式：None / QTE / Calibration
    Config::FishingMode fishingMode;

    // 是否可以捕鱼 / 攻击
    bool canFishFlag;
    bool canAttackFlag;

    // 攻击相关数值
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

    // 攻击冷却时间，后续联调鼠标攻击时使用
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

    // ========================================================
    // Item 接口
    // ========================================================

    // 旧接口：直接装备到 Player 身上
    // 后续真正联调时，建议主要由 InventorySystem 管理当前装备
    void use(Player& player) override;

    // ========================================================
    // 攻击接口
    // ========================================================

    // 旧攻击接口：兼容当前 GameManager 里的旧逻辑
    // 注意：fire() 会立刻扣耐久。
    //
    // 我们新设计是：
    // 鼠标左键攻击 → 先判断是否命中 → 命中后再扣耐久。
    //
    // 所以后续联调时，建议不要继续用 fire()，
    // 而是使用：
    // getDamage() + consumeAttackDurability()
    int fire();

    // 新攻击接口：命中敌人后再调用这个函数扣耐久
    bool consumeAttackDurability();

    // ========================================================
    // 捕鱼接口
    // ========================================================

    // 捕鱼结束后，根据结果扣耐久
    bool consumeFishingDurability(Config::FishingResult result);

    // 查询某个捕鱼结果对应的耐久消耗
    int getFishingDurabilityCost(Config::FishingResult result) const;

    // ========================================================
    // 通用耐久接口
    // ========================================================

    // 扣除指定耐久
    bool consumeDurability(int amount);

    // 强化：提升伤害和耐久上限
    void upgradeStats(int dmgBoost, int durBoost);

    // 修复：按最大耐久百分比修复
    void repairByPercent(int percent);

    // 修复：固定数值修复
    void repairFixed(int amount);

    // 修复：直接修满
    void repairToFull();

    // 存档读取时恢复强化后的运行时数值
    void loadRuntimeState(
        int savedDamage,
        int savedMaxDurability,
        int savedCurrentDurability,
        int savedRange,
        int savedDurabilityConsumption
    );

    // ========================================================
    // 状态查询
    // ========================================================

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

// ============================================================
// 具体装备子类
//
// 目前这些子类不额外增加逻辑，主要用于表达语义。
// 真正数值由 ItemFactory + GameConfig.h 统一生成。
// ============================================================

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