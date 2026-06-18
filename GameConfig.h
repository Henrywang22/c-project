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

    const int MAX_WEAPON_BACKPACK = 32;
    const int INFINITE_WEAPON_DURABILITY = 999999;

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
    const int HARPOON_PROJECTILE_HIT_PADDING = 5;
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
    const float VAL_UPG_SPEED_T1 = 12.0f;

    const int PRICE_UPG_SPEED_T2 = 250;
    const float VAL_UPG_SPEED_T2 = 22.0f;

    const int PRICE_UPG_SPEED_T3 = 500;
    const float VAL_UPG_SPEED_T3 = 38.0f;

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

    const int PRICE_UPG_WEAPON_T1 = 100;
    const int VAL_UPG_WPN_DMG_T1 = 5;
    const int VAL_UPG_WPN_DUR_T1 = 8;

    const int PRICE_UPG_WEAPON_T2 = 240;
    const int VAL_UPG_WPN_DMG_T2 = 12;
    const int VAL_UPG_WPN_DUR_T2 = 18;

    const int PRICE_UPG_WEAPON_T3 = 520;
    const int VAL_UPG_WPN_DMG_T3 = 24;
    const int VAL_UPG_WPN_DUR_T3 = 35;
    const int MAX_WEAPON_ENHANCEMENT_LEVEL = 6;

    // ==========================================
    // 7. 排行榜综合得分权重
    // ==========================================

    const int SCORE_STAGE_MAX = 3000;
    const int SCORE_TIME_MAX = 2200;
    const int SCORE_FISH_COUNT_MAX = 1800;
    const int SCORE_FISH_VALUE_MAX = 900;
    const int SCORE_KILL_MAX = 1700;
    const int SCORE_SURVIVAL_MAX = 400;

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

    namespace GameConfig {
        const qreal SHIP_BASE_SPEED = 150.0;
        const qreal SHIP_BOOST_SPEED = 250.0;
        const qreal SHIP_BOOST_MULTIPLIER = 1.65;
        const int   MAX_STAMINA = 100;
        const int   BOOST_STAMINA_COST_PER_FRAME = 1;
        const int   TOP_BORDER = 60;
        const int   BOTTOM_BORDER = 700;
        const int   RIGHT_BORDER = 47000;
        const int   STAGE_COUNT = 9;
        const int   STAGE_LENGTH = 2000;
        const int   FINAL_BOSS_TRIGGER_BUFFER = 500;
        const int   BOSS_EDGE_BUFFER = 320;

        const int   PLAYER_COLLIDER_WIDTH = 56;
        const int   PLAYER_COLLIDER_HEIGHT = 32;
        const int   FISH_INTERACTION_RADIUS = 34;
        const int   SHARK_VISUAL_WIDTH = 122;
        const int   SHARK_VISUAL_HEIGHT = 66;
        const int   SHARK_COLLIDER_WIDTH = 92;
        const int   SHARK_COLLIDER_HEIGHT = 34;
        const int   SHARK_BITE_REACH = 34;
        const int   SHARK_BITE_HEIGHT = 50;
        const int   SWORDFISH_VISUAL_WIDTH = 122;
        const int   SWORDFISH_VISUAL_HEIGHT = 72;
        const int   SWORDFISH_COLLIDER_WIDTH = 102;
        const int   SWORDFISH_COLLIDER_HEIGHT = 30;
        const int   OCTOPUS_VISUAL_WIDTH = 78;
        const int   OCTOPUS_VISUAL_HEIGHT = 76;
        const int   OCTOPUS_COLLIDER_WIDTH = 54;
        const int   OCTOPUS_COLLIDER_HEIGHT = 54;
        const int   ELECTRIC_RAY_VISUAL_WIDTH = 124;
        const int   ELECTRIC_RAY_VISUAL_HEIGHT = 96;
        const int   ELECTRIC_RAY_COLLIDER_WIDTH = 94;
        const int   ELECTRIC_RAY_COLLIDER_HEIGHT = 52;
        const int   ELECTRIC_RAY_PULSE_RADIUS = 112;
        const int   JELLYFISH_VISUAL_WIDTH = 118;
        const int   JELLYFISH_VISUAL_HEIGHT = 76;
        const int   JELLYFISH_COLLIDER_WIDTH = 78;
        const int   JELLYFISH_COLLIDER_HEIGHT = 52;
        const int   JELLYFISH_STING_REACH = 112;
        const int   JELLYFISH_STING_HEIGHT = 58;
        const int   BOSS_COLLIDER_WIDTH = 80;
        const int   BOSS_COLLIDER_HEIGHT = 40;
        const int   TALI_CLONE_COLLIDER_WIDTH = 56;
        const int   TALI_CLONE_COLLIDER_HEIGHT = 44;

        const int   SHARK_ATTACK_COOLDOWN_FRAMES = 90;
        const int   SHARK_RETREAT_FRAMES = 36;
        const qreal SHARK_RETREAT_SPEED_MULTIPLIER = 1.8;
        const qreal FISH_LOCKED_SPEED_MULTIPLIER = 0.35;

        const int   WAVE_WARNING_MS = 3000;
        const int   WAVE_DURATION_MS = 8000;
        const qreal WAVE_SPEED_UP_MULTIPLIER = 1.5;
        const qreal WAVE_SPEED_DOWN_MULTIPLIER = 0.6;

        const int   WEATHER_MIN_FRAMES = 1800;
        const int   WEATHER_MAX_FRAMES = 3600;
        const int   WEATHER_TRANSITION_FRAMES = 180;
        const qreal FOG_VISION_REDUCTION = 0.48;
        const qreal STORM_FISH_VALUE_BONUS = 1.5;
        const int   STORM_LIGHTNING_DAMAGE = 15;

        const int   REEF_MIN_SIZE = 20;
        const int   REEF_MAX_SIZE = 40;
        const int   REEF_DAMAGE = 10;
        const int   STUN_DURATION_MS = 500;
        const int   REEF_COLLISION_COOLDOWN_MS = 700;
        const qreal REEF_REBOUND_FACTOR = 0.45;
        const qreal WHIRLPOOL_MAX_SPEED_REDUCTION = 0.7;

        const int   VISION_RANGE = 800;
        const int   WINDOW_HEIGHT = 720;

        struct StageConfig {
            int targetDistance;
            bool hasBoss;

            int initialFish;
            int fishCap;
            int fishSpawnInterval;
            int sardineWeight;
            int tunaWeight;
            int eelWeight;
            int goldenWeight;

            int sharkCap;
            int sharkSpawnInterval;
            int swordfishCap;
            int swordfishSpawnInterval;
            int octopusCap;
            int octopusSpawnInterval;
            int electricRayCap;
            int electricRaySpawnInterval;
            int jellyfishCap;
            int jellyfishSpawnInterval;

            int reefCount;
            int whirlpoolCount;

            int waveChancePerFrame;
            int waveRightWeight;
            int waveLeftWeight;

            int sunnyWeight;
            int fogWeight;
            int stormWeight;
            int weatherMinFrames;
            int weatherMaxFrames;
            int lightningChanceDenominator;
        };

        inline const StageConfig STAGE_CONFIGS[STAGE_COUNT] = {
            // 1-3：逐步教学鱼群、追击敌人和环境危险。
            {4200, false,
             14, 20, 105, 70, 30, 0, 0,
             1, 900, 0, 0, 0, 0, 0, 0, 0, 0,
             5, 0, 1900, 72, 28, 100, 0, 0, 3600, 5400, 0},
            {8900, false,
             16, 22, 100, 55, 40, 5, 0,
             1, 820, 1, 1250, 0, 0, 0, 0, 0, 0,
             6, 0, 1700, 66, 34, 85, 15, 0, 3200, 5000, 0},
            {14000, false,
             17, 23, 96, 40, 45, 12, 3,
             2, 760, 1, 1050, 1, 1500, 1, 1500, 0, 0,
             7, 1, 1450, 58, 42, 70, 25, 5, 2800, 4600, 700},

            // 4：五头鲨关卡；密度暂缓，重点转向 Boss 机制。
            {19400, true,
             18, 24, 92, 30, 40, 22, 8,
             2, 700, 1, 950, 1, 1350, 1, 1300, 0, 0,
             7, 1, 1250, 54, 46, 60, 30, 10, 2500, 4300, 620},

            // 5-8：逐级引入水母、鳐鱼、稀有鱼、逆浪和暴风雨。
            {24600, false,
             19, 25, 88, 25, 38, 27, 10,
             2, 650, 2, 900, 1, 1200, 1, 1100, 1, 1500,
             8, 2, 1100, 50, 50, 50, 35, 15, 2300, 4000, 560},
            {29800, false,
             20, 26, 84, 20, 35, 30, 15,
             2, 600, 2, 820, 1, 1050, 1, 1000, 1, 1300,
             8, 2, 980, 47, 53, 40, 35, 25, 2100, 3800, 500},
            {35100, false,
             20, 27, 80, 16, 30, 34, 20,
             3, 560, 2, 760, 2, 980, 2, 900, 1, 1120,
             9, 3, 860, 43, 57, 35, 35, 30, 1900, 3500, 450},
            {40500, false,
             21, 28, 76, 12, 27, 36, 25,
             3, 520, 2, 700, 2, 900, 2, 820, 2, 1000,
             10, 3, 760, 40, 60, 25, 35, 40, 1750, 3300, 400},

            // 9：塞壬终局。保持可读的敌群上限，把压力留给技能组合。
            {46200, true,
             22, 29, 74, 8, 22, 38, 32,
             3, 480, 3, 650, 2, 820, 2, 760, 2, 900,
             11, 4, 680, 36, 64, 18, 28, 54, 1600, 3100, 350}
        };

        inline const StageConfig& stageConfig(int stage)
        {
            return STAGE_CONFIGS[qBound(0, stage - 1, STAGE_COUNT - 1)];
        }

        struct StageDecorPlacement {
            int stage;
            int imageIndex;
            qreal stageRatio;
            int y;
            qreal scale;
            int colliderWidth;
            int colliderHeight;
            int colliderOffsetY;
            bool mirror;
        };

        inline const StageDecorPlacement STAGE_DECORS[] = {
            {1, 0, 0.18, 118, 0.82, 300, 168, 34, false},
            {1, 0, 0.78, 636, 0.70, 258, 142, -28, true},

            {2, 1, 0.18, 132, 0.90, 332, 176, 30, false},
            {2, 0, 0.64, 628, 0.62, 230, 126, -24, true},

            {3, 2, 0.20, 612, 0.88, 326, 146, -22, false},
            {3, 2, 0.68, 126, 0.70, 260, 118, 20, true},

            {4, 3, 0.20, 620, 0.88, 322, 128, -20, false},
            {4, 2, 0.60, 118, 0.68, 252, 114, 18, true},

            {5, 4, 0.18, 128, 0.86, 316, 148, 22, false},
            {5, 3, 0.58, 628, 0.72, 264, 108, -18, true},
            {5, 4, 0.82, 610, 0.64, 236, 112, -18, true},

            {6, 5, 0.16, 130, 0.78, 286, 112, 18, false},
            {6, 0, 0.42, 618, 0.72, 266, 146, -24, true},
            {6, 2, 0.68, 132, 0.68, 252, 114, 18, false},
            {6, 5, 0.86, 604, 0.70, 258, 102, -18, true},

            {7, 4, 0.14, 126, 0.80, 294, 138, 20, false},
            {7, 5, 0.48, 616, 0.74, 272, 108, -20, true},
            {7, 3, 0.80, 132, 0.68, 250, 102, 18, false},

            {8, 5, 0.12, 612, 0.78, 286, 112, -18, true},
            {8, 4, 0.44, 126, 0.82, 302, 142, 22, false},
            {8, 2, 0.76, 618, 0.70, 258, 116, -20, true},

            {9, 5, 0.10, 128, 0.82, 300, 118, 20, false},
            {9, 4, 0.38, 616, 0.80, 294, 138, -20, true},
            {9, 3, 0.66, 126, 0.74, 272, 112, 20, false},
            {9, 5, 0.88, 604, 0.78, 286, 114, -18, true}
        };

        inline constexpr int STAGE_DECOR_COUNT =
            static_cast<int>(sizeof(STAGE_DECORS) / sizeof(STAGE_DECORS[0]));

        struct TerrainPropPlacement {
            int stage;
            int imageIndex;
            qreal stageRatio;
            int y;
            qreal scale;
            int colliderWidth;
            int colliderHeight;
            bool mirror;
        };

        inline const TerrainPropPlacement TERRAIN_PROPS[] = {
            {1, 1, 0.32, 132, 0.52, 150, 70, false},
            {1, 5, 0.50, 600, 0.38, 42, 42, false},
            {1, 0, 0.64, 142, 0.48, 120, 100, true},
            {1, 9, 0.88, 610, 0.42, 120, 55, false},

            {2, 10, 0.12, 610, 0.48, 100, 95, false},
            {2, 6, 0.34, 136, 0.48, 140, 90, false},
            {2, 3, 0.50, 608, 0.44, 120, 75, true},
            {2, 5, 0.68, 166, 0.34, 38, 38, false},
            {2, 11, 0.88, 612, 0.42, 80, 80, false},

            {3, 0, 0.10, 146, 0.48, 120, 100, false},
            {3, 7, 0.28, 606, 0.44, 100, 100, true},
            {3, 8, 0.44, 142, 0.48, 125, 70, false},
            {3, 4, 0.62, 604, 0.45, 150, 90, false},
            {3, 3, 0.80, 146, 0.44, 120, 75, true},
            {3, 5, 0.91, 584, 0.34, 38, 38, false},

            {4, 4, 0.12, 146, 0.46, 150, 90, true},
            {4, 0, 0.27, 608, 0.50, 125, 105, false},
            {4, 6, 0.42, 146, 0.48, 140, 90, true},
            {4, 7, 0.57, 604, 0.46, 105, 105, false},
            {4, 9, 0.72, 150, 0.44, 125, 58, true},
            {4, 8, 0.87, 606, 0.50, 130, 72, false},

            {5, 11, 0.09, 150, 0.44, 84, 84, false},
            {5, 3, 0.22, 608, 0.48, 128, 80, false},
            {5, 0, 0.36, 146, 0.52, 132, 108, true},
            {5, 4, 0.50, 606, 0.48, 155, 94, false},
            {5, 7, 0.64, 146, 0.48, 110, 108, true},
            {5, 9, 0.77, 608, 0.46, 132, 60, false},
            {5, 10, 0.90, 146, 0.50, 105, 100, false},

            {6, 0, 0.08, 608, 0.54, 138, 112, false},
            {6, 11, 0.18, 146, 0.46, 88, 88, true},
            {6, 3, 0.30, 608, 0.50, 132, 82, false},
            {6, 6, 0.42, 146, 0.50, 146, 94, true},
            {6, 8, 0.54, 608, 0.52, 136, 76, false},
            {6, 4, 0.66, 146, 0.50, 160, 96, true},
            {6, 10, 0.78, 608, 0.52, 110, 104, false},
            {6, 7, 0.88, 146, 0.50, 112, 110, true},
            {6, 5, 0.94, 374, 0.36, 40, 40, false},

            {7, 11, 0.10, 146, 0.48, 90, 90, false},
            {7, 4, 0.24, 606, 0.50, 158, 96, true},
            {7, 6, 0.39, 146, 0.52, 150, 96, false},
            {7, 8, 0.55, 606, 0.52, 138, 78, true},
            {7, 7, 0.71, 146, 0.50, 114, 112, false},
            {7, 10, 0.88, 606, 0.52, 112, 106, true},

            {8, 0, 0.08, 606, 0.55, 140, 114, true},
            {8, 3, 0.20, 146, 0.52, 136, 84, false},
            {8, 7, 0.34, 606, 0.52, 116, 114, true},
            {8, 4, 0.48, 146, 0.54, 166, 100, false},
            {8, 8, 0.63, 606, 0.54, 140, 80, true},
            {8, 6, 0.78, 146, 0.52, 150, 96, false},
            {8, 11, 0.91, 604, 0.48, 92, 92, true},

            {9, 11, 0.07, 146, 0.50, 94, 94, false},
            {9, 0, 0.18, 606, 0.56, 144, 116, true},
            {9, 6, 0.30, 146, 0.54, 154, 98, false},
            {9, 4, 0.42, 606, 0.56, 170, 102, true},
            {9, 7, 0.54, 146, 0.54, 120, 116, false},
            {9, 8, 0.66, 606, 0.56, 144, 82, true},
            {9, 10, 0.78, 146, 0.54, 116, 110, false},
            {9, 3, 0.89, 606, 0.54, 140, 86, true},
            {9, 5, 0.95, 374, 0.38, 42, 42, false}
        };

        inline constexpr int TERRAIN_PROP_COUNT =
            static_cast<int>(sizeof(TERRAIN_PROPS) / sizeof(TERRAIN_PROPS[0]));

        struct StageText {
            const char* name;
            const char* brief;
            const char* clearSummary;
        };

        inline const StageText STAGE_TEXTS[STAGE_COUNT] = {
            {u8"近岸练习海域", u8"熟悉捕鱼、移动与礁石，建立基础航行节奏。", u8"近岸航线恢复安全。"},
            {u8"外海遭遇", u8"鲨鱼和剑鱼开始共同出没，注意保留船体耐久。", u8"外海第一段航线已经打通。"},
            {u8"暗流航段", u8"鳐鱼、章鱼与暗流加入战场，观察预警再行动。", u8"船队穿过了反复无常的暗流。"},
            {u8"五首猎场", u8"守命五头鲨封锁航道，击败它才能继续远航。", u8"五头鲨的围猎被彻底瓦解。"},
            {u8"深海裂谷", u8"水母与稀有鱼出现，收益提高，风险也同步上升。", u8"深海裂谷暂时归于平静。"},
            {u8"迷雾沉沟", u8"雾、雷雨和漩涡交替出现，航线选择更加重要。", u8"沉沟中的安全水道已经标记。"},
            {u8"风暴前沿", u8"高密度敌群与逆浪考验装备、走位和补给规划。", u8"风暴前沿被成功穿越。"},
            {u8"塞壬外环", u8"珍稀鱼群藏在危险海况中，为终战完成最后准备。", u8"塞壬外环的迷雾开始消散。"},
            {u8"终海门", u8"塞壬守在航线尽头，终海的危险全面苏醒。", u8"终海门被打开，渔途航线完整连通。"}
        };

        inline const StageText& stageText(int stage)
        {
            return STAGE_TEXTS[qBound(0, stage - 1, STAGE_COUNT - 1)];
        }

        inline int stageStartDistance(int stage)
        {
            const int safeStage = qBound(1, stage, STAGE_COUNT);
            if (safeStage <= 1) return 0;
            return stageConfig(safeStage - 1).targetDistance;
        }
    }
}
