#pragma once
#include "Item.h"
#include "GameConfig.h"
#include <string>

class Player;  // 前向声明

// ---------------- 武器基类 ----------------
class Weapon : public Item {
protected:
    int damage;
    int maxDurability;
    int currentDurability;
    int range;
    int durabilityConsumption;

public:
    // 构造函数 — 仅声明，实现在 Weapon.cpp 中
    Weapon(std::string specificName, int price, int dmg, int dur, int rng, int cons);

    virtual ~Weapon() = default;

    // 装备逻辑 — 仅声明
    void use(Player& player) override;

    // 攻击开火 — 仅声明
    int fire();

    // 强化接口 — 仅声明
    void upgradeStats(int dmgBoost, int durBoost);

    // --------------------------------------------------
    // 状态查询接口（简单内联，保留在头文件中）
    // --------------------------------------------------
    bool isBroken() const {
        return currentDurability <= 0;
    }

    int getCurrentDur() const {
        return currentDurability;
    }

    int getMaxDur() const {
        return maxDurability;
    }

    int getDamage() const {
        return damage;
    }

    int getRange() const {
        return range;
    }
};

// ==========================================
// 具体武器子类（继承基类构造函数即可）
// ==========================================

class FishingRod : public Weapon {
public:
    using Weapon::Weapon;  // 继承基类构造函数，由工厂传入具体参数
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