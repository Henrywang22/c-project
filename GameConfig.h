#pragma once
#include <string>
#include <QtGlobal>

// ==========================================
// 渔途 (Fishing Voyage) - 全局数值配置表
//
// 原则：
// 1. 所有和数值有关的东西都放在这里。
// 2. 后期调平衡时，只改 GameConfig.h，不到处改 cpp。
// 3. T1=初级，T2=中级，T3=高级。
// ==========================================

namespace Config {

    // ==========================================
    // 0. 背包系统
    // ==========================================

    const int MAX_WEAPON_BACKPACK = 3;

    // 当前阶段先视为“不限制物品背包”
    // 最终版本如果要限制容量，直接改成 10 / 12 / 20 即可。
    const int MAX_ITEM_BACKPACK = 999;

    // ==========================================
    // 1. 装备类型与捕鱼模式
    // ==========================================

    enum class EquipmentRole {
        FishingTool,     // 只能捕鱼：鱼竿、渔网
        AttackWeapon,    // 只能攻击：手枪、猎枪
        HybridTool       // 捕鱼 + 攻击：鱼叉
    };

    enum class FishingMode {
        None,
        QTE,
        Calibration
    };

    enum class FishingResult {
        Fail,
        Normal,
        Perfect
    };

    // ==========================================
    // 2. 消耗品
    // ==========================================

    // ---- 食物类 ----
    const int PRICE_FOOD_RATION = 20;
    const int HEAL_FOOD_RATION = 30;

    // ---- 船体修理包 ----
    const int PRICE_REPAIR_T1 = 30;
    const int HEAL_REPAIR_T1 = 20;

    const int PRICE_REPAIR_T2 = 50;
    const int HEAL_REPAIR_T2 = 40;

    const int PRICE_REPAIR_T3 = 100;
    const int HEAL_REPAIR_T3 = 100;

    // ---- 紧急装备修理工具：放入背包，游戏途中使用 ----
    const int PRICE_EMERGENCY_WEAPON_REPAIR = 40;

    // 恢复所选装备最大耐久的百分比。
    // 这是紧急修复，所以数值不要太高。
    const int EMERGENCY_WEAPON_REPAIR_PERCENT = 25;

    // ---- 商店装备修复服务：只在商店中使用 ----
    const int PRICE_SHOP_WEAPON_REPAIR = 80;
    const int SHOP_WEAPON_REPAIR_PERCENT = 80;

    // ==========================================
    // 3. 装备系统
    // ==========================================

    // ---- 鱼竿：捕鱼工具，QTE 捕鱼，不能攻击 ----
    const int RANGE_ROD = 60;
    const int CONS_ROD = 1;

    const int PRICE_ROD_T1 = 60;
    const int DMG_ROD_T1 = 0;
    const int DUR_ROD_T1 = 50;

    const int PRICE_ROD_T2 = 120;
    const int DMG_ROD_T2 = 0;
    const int DUR_ROD_T2 = 60;

    const int PRICE_ROD_T3 = 240;
    const int DMG_ROD_T3 = 0;
    const int DUR_ROD_T3 = 80;

    // ---- 渔网：捕鱼工具，校准捕鱼，不能攻击 ----
    const int RANGE_NET = 80;
    const int CONS_NET = 1;

    const int PRICE_NET_T1 = 80;
    const int DMG_NET_T1 = 0;
    const int DUR_NET_T1 = 40;

    const int PRICE_NET_T2 = 160;
    const int DMG_NET_T2 = 0;
    const int DUR_NET_T2 = 50;

    const int PRICE_NET_T3 = 300;
    const int DMG_NET_T3 = 0;
    const int DUR_NET_T3 = 60;

    // ---- 鱼叉：双用工具，校准捕鱼，也能攻击 ----
    const int RANGE_HARPOON = 120;
    const int CONS_HARPOON = 1;

    const int PRICE_HARPOON_T1 = 100;
    const int DMG_HARPOON_T1 = 30;
    const int DUR_HARPOON_T1 = 25;

    const int PRICE_HARPOON_T2 = 220;
    const int DMG_HARPOON_T2 = 55;
    const int DUR_HARPOON_T2 = 30;

    const int PRICE_HARPOON_T3 = 450;
    const int DMG_HARPOON_T3 = 100;
    const int DUR_HARPOON_T3 = 40;

    // ---- 手枪：攻击武器，不能捕鱼 ----
    const int RANGE_PISTOL = 200;
    const int CONS_PISTOL = 1;

    const int PRICE_PISTOL_T1 = 180;
    const int DMG_PISTOL_T1 = 50;
    const int DUR_PISTOL_T1 = 15;

    const int PRICE_PISTOL_T2 = 380;
    const int DMG_PISTOL_T2 = 90;
    const int DUR_PISTOL_T2 = 20;

    const int PRICE_PISTOL_T3 = 750;
    const int DMG_PISTOL_T3 = 150;
    const int DUR_PISTOL_T3 = 25;

    // ---- 猎枪：攻击武器，不能捕鱼，高伤害高损耗 ----
    const int RANGE_SHOTGUN = 150;
    const int CONS_SHOTGUN = 2;

    const int PRICE_SHOTGUN_T1 = 250;
    const int DMG_SHOTGUN_T1 = 80;
    const int DUR_SHOTGUN_T1 = 10;

    const int PRICE_SHOTGUN_T2 = 550;
    const int DMG_SHOTGUN_T2 = 140;
    const int DUR_SHOTGUN_T2 = 12;

    const int PRICE_SHOTGUN_T3 = 1200;
    const int DMG_SHOTGUN_T3 = 250;
    const int DUR_SHOTGUN_T3 = 15;

    // ==========================================
    // 4. 捕鱼耐久消耗
    // 捕鱼结束后，根据结果扣耐久
    // ==========================================

    // 鱼竿：QTE 捕鱼
    const int ROD_FISH_COST_PERFECT = 1;
    const int ROD_FISH_COST_NORMAL = 2;
    const int ROD_FISH_COST_FAIL = 1;

    // 渔网：校准捕鱼
    const int NET_FISH_COST_PERFECT = 2;
    const int NET_FISH_COST_NORMAL = 4;
    const int NET_FISH_COST_FAIL = 2;

    // 鱼叉：校准捕鱼
    const int HARPOON_FISH_COST_PERFECT = 2;
    const int HARPOON_FISH_COST_NORMAL = 3;
    const int HARPOON_FISH_COST_FAIL = 2;

    // ==========================================
    // 5. 攻击冷却
    // 注意：命中敌人才扣耐久，但无论是否命中都进入冷却
    // ==========================================

    const int ATTACK_COOLDOWN_DEFAULT_MS = 500;
    const int ATTACK_COOLDOWN_ROD_MS = 0;
    const int ATTACK_COOLDOWN_NET_MS = 0;
    const int ATTACK_COOLDOWN_HARPOON_MS = 600;
    const int ATTACK_COOLDOWN_PISTOL_MS = 400;
    const int ATTACK_COOLDOWN_SHOTGUN_MS = 800;

    // ==========================================
    // 6. 属性升级系统
    // ==========================================

    const int PRICE_UPG_SPEED_T1 = 120;
    const float VAL_UPG_SPEED_T1 = 1.0f;

    const int PRICE_UPG_SPEED_T2 = 250;
    const float VAL_UPG_SPEED_T2 = 2.0f;

    const int PRICE_UPG_SPEED_T3 = 500;
    const float VAL_UPG_SPEED_T3 = 3.5f;

    const int PRICE_UPG_DUR_T1 = 100;
    const int VAL_UPG_DUR_T1 = 20;

    const int PRICE_UPG_DUR_T2 = 220;
    const int VAL_UPG_DUR_T2 = 50;

    const int PRICE_UPG_DUR_T3 = 450;
    const int VAL_UPG_DUR_T3 = 100;

    const int PRICE_UPG_STAMINA_T1 = 100;
    const int VAL_UPG_STAMINA_T1 = 20;

    const int PRICE_UPG_STAMINA_T2 = 220;
    const int VAL_UPG_STAMINA_T2 = 50;

    const int PRICE_UPG_STAMINA_T3 = 450;
    const int VAL_UPG_STAMINA_T3 = 100;

    const int PRICE_UPG_WEAPON_T1 = 80;
    const int VAL_UPG_WPN_DMG_T1 = 5;
    const int VAL_UPG_WPN_DUR_T1 = 10;

    const int PRICE_UPG_WEAPON_T2 = 180;
    const int VAL_UPG_WPN_DMG_T2 = 15;
    const int VAL_UPG_WPN_DUR_T2 = 25;

    const int PRICE_UPG_WEAPON_T3 = 400;
    const int VAL_UPG_WPN_DMG_T3 = 40;
    const int VAL_UPG_WPN_DUR_T3 = 50;

    // ==========================================
    // 7. 排行榜综合得分权重
    // ==========================================

    const int SCORE_STAGE_WEIGHT = 1000;
    const int SCORE_FISH_VALUE_WEIGHT = 2;
    const int SCORE_FISH_COUNT_WEIGHT = 50;
    const int SCORE_KILL_WEIGHT = 100;
    const int SCORE_COIN_WEIGHT = 1;
    const int SCORE_DURABILITY_WEIGHT = 5;
    const int SCORE_STAMINA_WEIGHT = 3;
    const int SCORE_TIME_PENALTY = 2;

    // ==========================================
    // 8. 文字消息
    // ==========================================

    namespace Messages {
        const std::string SUCCESS_USE = "【使用成功】";
        const std::string SUCCESS_UPGRADE = "【属性提升】";
        const std::string SUCCESS_WEAPON = "【装备系统】";
        const std::string FAIL_WEAPON = "【强化失败】你当前未装备任何装备。";
        const std::string WARN_BROKEN = "【战斗警告】当前装备已损坏，无法继续使用！";
        const std::string PREFIX_ITEM = "使用了：";
        const std::string PREFIX_WEAPON = "已装备：";
    }

    // ==========================================
    // 9. 游戏系统数值（海浪/天气/玩家/障碍）
    // 保留原来的 inline namespace，避免影响其他成员文件。
    // ==========================================

    inline namespace GameConfig {
        const qreal SHIP_BASE_SPEED = 150.0;
        const qreal SHIP_BOOST_SPEED = 250.0;
        const int   MAX_STAMINA = 100;
        const int   BOOST_STAMINA_COST_PER_FRAME = 1;
        const int   TOP_BORDER = 60;
        const int   BOTTOM_BORDER = 700;
        const int   RIGHT_BORDER = 10000;

        const int   WAVE_WARNING_MS = 3000;
        const int   WAVE_DURATION_MS = 8000;
        const qreal WAVE_SPEED_UP_MULTIPLIER = 1.5;
        const qreal WAVE_SPEED_DOWN_MULTIPLIER = 0.6;

        const int   WEATHER_MIN_FRAMES = 1800;
        const int   WEATHER_MAX_FRAMES = 3600;
        const qreal FOG_VISION_REDUCTION = 0.3;
        const qreal STORM_FISH_VALUE_BONUS = 1.5;
        const int   STORM_LIGHTNING_DAMAGE = 15;

        const int   REEF_MIN_SIZE = 20;
        const int   REEF_MAX_SIZE = 40;
        const int   REEF_DAMAGE = 10;
        const int   STUN_DURATION_MS = 500;
        const qreal REEF_REBOUND_FACTOR = 1.5;
        const qreal WHIRLPOOL_MAX_SPEED_REDUCTION = 0.7;

        const int   VISION_RANGE = 800;
        const int   WINDOW_HEIGHT = 720;
    }
}