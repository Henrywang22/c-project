#pragma once
#include <string>

// ==========================================
// 渔途 (Fishing Voyage) - 全局数值配置表
// 所有的物品价格、数值都在这里统一调整
// T1=小/初级, T2=中/进阶, T3=大/终极
// ==========================================

namespace Config {

    // ==========================================
    // 1. 消耗品 (食物 & 修理包)
    // ==========================================
    
    // ---- 食物类 ----
    const int PRICE_FOOD_RATION = 20;    
    const int HEAL_FOOD_RATION  = 30;    

    // ---- 修理包 (大中小) ----
    const int PRICE_REPAIR_T1 = 30;   const int HEAL_REPAIR_T1 = 20;
    const int PRICE_REPAIR_T2 = 50;   const int HEAL_REPAIR_T2 = 40;  
    const int PRICE_REPAIR_T3 = 100;  const int HEAL_REPAIR_T3 = 100;

    // ==========================================
    // 2. 武器系统 (5种武器 x 3等级)
    // CONS = 每次攻击损耗的耐久度
    // ==========================================

    // ---- 1. 鱼竿 (高耐久，低伤害，低损耗) ----
    const int RANGE_ROD = 60; 
    const int CONS_ROD = 1; // 每次开火扣1点
    const int PRICE_ROD_T1 = 60;  const int DMG_ROD_T1 = 5;  const int DUR_ROD_T1 = 50;
    const int PRICE_ROD_T2 = 120; const int DMG_ROD_T2 = 12; const int DUR_ROD_T2 = 60;
    const int PRICE_ROD_T3 = 240; const int DMG_ROD_T3 = 25; const int DUR_ROD_T3 = 80;

    // ---- 2. 渔网 (中射程，中低伤害) ----
    const int RANGE_NET = 80;
    const int CONS_NET = 1;
    const int PRICE_NET_T1 = 80;  const int DMG_NET_T1 = 8;  const int DUR_NET_T1 = 40;
    const int PRICE_NET_T2 = 160; const int DMG_NET_T2 = 18; const int DUR_NET_T2 = 50;
    const int PRICE_NET_T3 = 300; const int DMG_NET_T3 = 35; const int DUR_NET_T3 = 60;

    // ---- 3. 鱼叉 (远射程，中高伤害) ----
    const int RANGE_HARPOON = 120;
    const int CONS_HARPOON = 2; // 鱼叉较重，损耗稍大
    const int PRICE_HARPOON_T1 = 100; const int DMG_HARPOON_T1 = 30;  const int DUR_HARPOON_T1 = 25;
    const int PRICE_HARPOON_T2 = 220; const int DMG_HARPOON_T2 = 55;  const int DUR_HARPOON_T2 = 30;
    const int PRICE_HARPOON_T3 = 450; const int DMG_HARPOON_T3 = 100; const int DUR_HARPOON_T3 = 40;

    // ---- 4. 手枪 (极远射程，高伤害) ----
    const int RANGE_PISTOL = 200;
    const int CONS_PISTOL = 3; 
    const int PRICE_PISTOL_T1 = 180; const int DMG_PISTOL_T1 = 50;  const int DUR_PISTOL_T1 = 15;
    const int PRICE_PISTOL_T2 = 380; const int DMG_PISTOL_T2 = 90;  const int DUR_PISTOL_T2 = 20;
    const int PRICE_PISTOL_T3 = 750; const int DMG_PISTOL_T3 = 150; const int DUR_PISTOL_T3 = 25;

    // ---- 5. 猎枪 (高伤害，极高损耗) ----
    const int RANGE_SHOTGUN = 150;
    const int CONS_SHOTGUN = 5; // 威力大，但容易坏
    const int PRICE_SHOTGUN_T1 = 250;  const int DMG_SHOTGUN_T1 = 80;  const int DUR_SHOTGUN_T1 = 10;
    const int PRICE_SHOTGUN_T2 = 550;  const int DMG_SHOTGUN_T2 = 140; const int DUR_SHOTGUN_T2 = 12;
    const int PRICE_SHOTGUN_T3 = 1200; const int DMG_SHOTGUN_T3 = 250; const int DUR_SHOTGUN_T3 = 15;


    // ==========================================
    // 3. 属性升级系统 (3等级)
    // ==========================================

    // ---- 船速强化 ----
    const int PRICE_UPG_SPEED_T1 = 120; const float VAL_UPG_SPEED_T1 = 1.0f;
    const int PRICE_UPG_SPEED_T2 = 250; const float VAL_UPG_SPEED_T2 = 2.0f;
    const int PRICE_UPG_SPEED_T3 = 500; const float VAL_UPG_SPEED_T3 = 3.5f;

    // ---- 耐久上限强化 ----
    const int PRICE_UPG_DUR_T1 = 100; const int VAL_UPG_DUR_T1 = 20;
    const int PRICE_UPG_DUR_T2 = 220; const int VAL_UPG_DUR_T2 = 50;
    const int PRICE_UPG_DUR_T3 = 450; const int VAL_UPG_DUR_T3 = 100;

    // ---- 体力上限强化 ----
    const int PRICE_UPG_STAMINA_T1 = 100; const int VAL_UPG_STAMINA_T1 = 20;
    const int PRICE_UPG_STAMINA_T2 = 220; const int VAL_UPG_STAMINA_T2 = 50;
    const int PRICE_UPG_STAMINA_T3 = 450; const int VAL_UPG_STAMINA_T3 = 100;

    // ---- 当前武器强化 ----
    const int PRICE_UPG_WEAPON_T1 = 80;  const int VAL_UPG_WPN_DMG_T1 = 5;  const int VAL_UPG_WPN_DUR_T1 = 10;
    const int PRICE_UPG_WEAPON_T2 = 180; const int VAL_UPG_WPN_DMG_T2 = 15; const int VAL_UPG_WPN_DUR_T2 = 25;
    const int PRICE_UPG_WEAPON_T3 = 400; const int VAL_UPG_WPN_DMG_T3 = 40; const int VAL_UPG_WPN_DUR_T3 = 50;

    namespace Messages {
        const std::string SUCCESS_USE    = "【使用成功】";
        const std::string SUCCESS_UPGRADE = "【属性提升】";
        const std::string SUCCESS_WEAPON  = "【装备系统】";
        const std::string FAIL_WEAPON     = "【强化失败】你当前未装备任何武器。";
        const std::string WARN_BROKEN     = "【战斗警告】武器已损坏，无法继续攻击！";
        const std::string PREFIX_ITEM     = "使用了：";
        const std::string PREFIX_WEAPON   = "已装备：";
    }
}