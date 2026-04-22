#pragma once
#include <string>
#include <iostream>

class Player; 
class Weapon;// 前向声明，防止头文件相互包含报错

// ---------------- 基础物品类 ----------------
class Item {
protected:
    std::string name;
    int economicValue; // 卖出能得多少金币

public:
    Item(std::string n, int val) : name(n), economicValue(val) {}
    virtual ~Item() = default; // 虚析构函数，防止多态删除时内存泄漏

    // 纯虚函数：使用物品。由具体子类实现
    virtual void use(Player& player) = 0; 
    
    int getValue() const { return economicValue; }
    std::string getName() const { return name; }
};

// ---------------- 食物类 ----------------
class FoodItem : public Item {
private:
    int staminaRestore;

public:
    FoodItem(std::string n, int val, int stamina) 
        : Item(n, val), staminaRestore(stamina) {}

    // 恢复体力 
    void use(Player& player) override; 
};

// ---------------- 修理包类 ----------------
class RepairKit : public Item {
private:
    int durabilityRestore;

public:
    RepairKit(std::string n, int val, int durRes) 
        : Item(n, val), durabilityRestore(durRes) {}

    // 恢复船只耐久 
    void use(Player& player) override; 
};

// ---------------- 升级物品类 ----------------
// 船速上限升级
class SpeedUpgrade : public Item {
private:
    float speedBoost;
public:
    // 构造函数接收：商品名称、价格、提升的数值
    SpeedUpgrade(std::string specificName, int price, float boost) 
        : Item(specificName, price), speedBoost(boost) {}
    
    void use(Player& player) override; 
};

// 耐久度上限升级
class MaxDurabilityUpgrade : public Item {
private:
    int upgradeAmount;
public:
    MaxDurabilityUpgrade(std::string specificName, int price, int amount) 
        : Item(specificName, price), upgradeAmount(amount) {}
    
    void use(Player& player) override; 
};

// 体力上限升级
class MaxStaminaUpgrade : public Item {
private:
    int upgradeAmount;
public:
    MaxStaminaUpgrade(std::string specificName, int price, int amount) 
        : Item(specificName, price), upgradeAmount(amount) {}
    
    void use(Player& player) override; 
};

// 当前武器强化 (攻击力 & 耐久上限)
class WeaponUpgrade : public Item {
private:
    int damageBoost;
    int durabilityBoost;
public:
    WeaponUpgrade(std::string specificName, int price, int dmgBoost, int durBoost) 
        : Item(specificName, price), damageBoost(dmgBoost), durabilityBoost(durBoost) {}
    
    void use(Player& player) override; 
};