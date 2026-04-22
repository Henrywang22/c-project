#include "Item.h"
#include "Player.h"
#include "Weapon.h"
#include "GameConfig.h"
#include <iostream>

// ==========================================
// 1. 消耗品类实现
// ==========================================

void FoodItem::use(Player& player) {
    // 执行恢复逻辑
    player.restoreStamina(staminaRestore);
    // 引用 Config 中的统一成功文案
    std::cout << Config::Messages::SUCCESS_USE 
              << Config::Messages::PREFIX_ITEM << name << std::endl;
}

void RepairKit::use(Player& player) {
    player.restoreDurability(durabilityRestore);
    std::cout << Config::Messages::SUCCESS_USE 
              << Config::Messages::PREFIX_ITEM << name << std::endl;
}

// ==========================================
// 2. 升级凭证类实现
// ==========================================

void SpeedUpgrade::use(Player& player) {
    player.upgradeBaseSpeed(speedBoost);
    std::cout << Config::Messages::SUCCESS_UPGRADE 
              << name << " (" << speedBoost << ")" << std::endl;
}

void MaxDurabilityUpgrade::use(Player& player) {
    player.upgradeMaxDurability(upgradeAmount);
    std::cout << Config::Messages::SUCCESS_UPGRADE 
              << name << " +" << upgradeAmount << std::endl;
}

void MaxStaminaUpgrade::use(Player& player) {
    player.upgradeMaxStamina(upgradeAmount);
    std::cout << Config::Messages::SUCCESS_UPGRADE 
              << name << " +" << upgradeAmount << std::endl;
}

void WeaponUpgrade::use(Player& player) {
    // 获取当前装备
    Weapon* currentWeapon = player.getCurrentWeapon();

    if (currentWeapon != nullptr) {
        // 执行强化
        currentWeapon->upgradeStats(damageBoost, durabilityBoost);
        std::cout << Config::Messages::SUCCESS_USE 
                  << "当前武器强化完成。" << std::endl;
    } else {
        // 引用统一的失败文案
        std::cout << Config::Messages::FAIL_WEAPON << std::endl;
    }
}