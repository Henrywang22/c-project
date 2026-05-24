#include "Weapon.h"
#include "Player.h"
#include "GameConfig.h"

#include <iostream>

// ============================================================
// 构造函数
// ============================================================

Weapon::Weapon(
    std::string specificName,
    int price,
    int dmg,
    int dur,
    int rng,
    int cons,
    std::string typeCode_,
    int tier_,
    Config::EquipmentRole role_,
    Config::FishingMode fishingMode_,
    bool canFish_,
    bool canAttack_,
    int fishCostFail_,
    int fishCostNormal_,
    int fishCostPerfect_,
    int attackCooldownMs_
)
    : Item(specificName, price),
      typeCode(typeCode_),
      tier(tier_),
      role(role_),
      fishingMode(fishingMode_),
      canFishFlag(canFish_),
      canAttackFlag(canAttack_),
      damage(dmg),
      maxDurability(dur),
      currentDurability(dur),
      range(rng),
      durabilityConsumption(cons),
      fishCostFail(fishCostFail_),
      fishCostNormal(fishCostNormal_),
      fishCostPerfect(fishCostPerfect_),
      attackCooldownMs(attackCooldownMs_)
{
}

// ============================================================
// Item 接口
// ============================================================

void Weapon::use(Player& player)
{
    // 兼容旧代码：购买或使用武器时直接装备到 Player
    // 后续正式逻辑中，装备应主要进入 InventorySystem 的装备背包
    player.equipWeapon(this);

    std::cout << Config::Messages::SUCCESS_WEAPON
              << Config::Messages::PREFIX_WEAPON
              << name << std::endl;
}

// ============================================================
// 攻击接口
// ============================================================

int Weapon::fire()
{
    if (!canAttackFlag) {
        return 0;
    }

    if (isBroken()) {
        std::cout << Config::Messages::WARN_BROKEN << std::endl;
        return 0;
    }

    // 旧逻辑：fire() 会立即扣耐久
    // 后续鼠标攻击联调时，不建议继续用 fire()
    if (!consumeAttackDurability()) {
        return 0;
    }

    return damage;
}

bool Weapon::consumeAttackDurability()
{
    if (!canAttackFlag) {
        return false;
    }

    return consumeDurability(durabilityConsumption);
}

// ============================================================
// 捕鱼接口
// ============================================================

bool Weapon::consumeFishingDurability(Config::FishingResult result)
{
    if (!canFishFlag) {
        return false;
    }

    int cost = getFishingDurabilityCost(result);
    return consumeDurability(cost);
}

int Weapon::getFishingDurabilityCost(Config::FishingResult result) const
{
    switch (result) {
    case Config::FishingResult::Perfect:
        return fishCostPerfect;

    case Config::FishingResult::Normal:
        return fishCostNormal;

    case Config::FishingResult::Fail:
        return fishCostFail;
    }

    return fishCostFail;
}

// ============================================================
// 通用耐久接口
// ============================================================

bool Weapon::consumeDurability(int amount)
{
    if (amount <= 0) {
        return true;
    }

    if (isBroken()) {
        std::cout << Config::Messages::WARN_BROKEN << std::endl;
        return false;
    }

    currentDurability -= amount;

    if (currentDurability < 0) {
        currentDurability = 0;
    }

    return true;
}

void Weapon::upgradeStats(int dmgBoost, int durBoost)
{
    damage += dmgBoost;
    maxDurability += durBoost;

    // 强化时顺带恢复一部分耐久，但不直接修满
    // 这样不会完全替代“装备修复”系统
    currentDurability += durBoost / 2;

    if (currentDurability > maxDurability) {
        currentDurability = maxDurability;
    }

    if (currentDurability < 0) {
        currentDurability = 0;
    }
}

void Weapon::repairByPercent(int percent)
{
    if (percent <= 0) {
        return;
    }

    int amount = maxDurability * percent / 100;

    // 防止百分比太小导致 0 修复
    if (amount <= 0) {
        amount = 1;
    }

    repairFixed(amount);
}

void Weapon::repairFixed(int amount)
{
    if (amount <= 0) {
        return;
    }

    currentDurability += amount;

    if (currentDurability > maxDurability) {
        currentDurability = maxDurability;
    }
}

void Weapon::repairToFull()
{
    currentDurability = maxDurability;
}

void Weapon::loadRuntimeState(
    int savedDamage,
    int savedMaxDurability,
    int savedCurrentDurability,
    int savedRange,
    int savedDurabilityConsumption
)
{
    if (savedDamage >= 0) {
        damage = savedDamage;
    }

    if (savedMaxDurability > 0) {
        maxDurability = savedMaxDurability;
    }

    if (savedCurrentDurability < 0) {
        currentDurability = 0;
    }
    else if (savedCurrentDurability > maxDurability) {
        currentDurability = maxDurability;
    }
    else {
        currentDurability = savedCurrentDurability;
    }

    if (savedRange > 0) {
        range = savedRange;
    }

    if (savedDurabilityConsumption >= 0) {
        durabilityConsumption = savedDurabilityConsumption;
    }
}

// ============================================================
// 状态查询
// ============================================================

bool Weapon::isBroken() const
{
    return currentDurability <= 0;
}

bool Weapon::canFish() const
{
    return canFishFlag;
}

bool Weapon::canAttack() const
{
    return canAttackFlag;
}

int Weapon::getCurrentDur() const
{
    return currentDurability;
}

int Weapon::getMaxDur() const
{
    return maxDurability;
}

int Weapon::getDamage() const
{
    return damage;
}

int Weapon::getRange() const
{
    return range;
}

int Weapon::getDurabilityConsumption() const
{
    return durabilityConsumption;
}

int Weapon::getAttackCooldownMs() const
{
    return attackCooldownMs;
}

int Weapon::getTier() const
{
    return tier;
}

Config::EquipmentRole Weapon::getRole() const
{
    return role;
}

Config::FishingMode Weapon::getFishingMode() const
{
    return fishingMode;
}

std::string Weapon::getTypeCode() const
{
    return typeCode;
}

std::string Weapon::getRoleName() const
{
    switch (role) {
    case Config::EquipmentRole::FishingTool:
        return "捕鱼工具";

    case Config::EquipmentRole::AttackWeapon:
        return "攻击武器";

    case Config::EquipmentRole::HybridTool:
        return "双用工具";
    }

    return "未知装备";
}

std::string Weapon::getFishingModeName() const
{
    switch (fishingMode) {
    case Config::FishingMode::None:
        return "不可捕鱼";

    case Config::FishingMode::QTE:
        return "QTE捕鱼";

    case Config::FishingMode::Calibration:
        return "校准捕鱼";
    }

    return "未知";
}