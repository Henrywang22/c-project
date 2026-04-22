#include "Weapon.h"
#include "Player.h"
#include "GameConfig.h"
#include <iostream>

/**
 * 构造函数实现
 */
Weapon::Weapon(std::string specificName, int price, int dmg, int dur, int rng, int cons)
    : Item(specificName, price), 
      damage(dmg), 
      maxDurability(dur), 
      currentDurability(dur), 
      range(rng), 
      durabilityConsumption(cons) {}

/**
 * 装备武器逻辑
 */
void Weapon::use(Player& player) {
    // 提示：具体 equipWeapon 逻辑由成员 C 在 Player 类中实现
    // player.equipWeapon(this); 
    std::cout << Config::Messages::SUCCESS_WEAPON 
              << Config::Messages::PREFIX_WEAPON << name << std::endl;
}

/**
 * 攻击开火逻辑：支持自定义损耗率
 */
int Weapon::fire() {
    if (currentDurability > 0) {
        // 根据配置的损耗率扣除耐久
        currentDurability -= durabilityConsumption;
        
        // 防呆处理：防止耐久出现负数
        if (currentDurability < 0) {
            currentDurability = 0;
        }
        
        return damage;
    }
    
    // 引用统一的损坏警告文案
    std::cout << Config::Messages::WARN_BROKEN << std::endl;
    return 0; 
}

/**
 * 强化接口实现
 */
void Weapon::upgradeStats(int dmgBoost, int durBoost) {
    damage += dmgBoost;
    maxDurability += durBoost;
    
    // 提升上限的同时修复等额耐久
    currentDurability += durBoost; 
}