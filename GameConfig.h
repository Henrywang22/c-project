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

    const int MAX_WEAPON_BACKPACK = 99;
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

    namespace GameConfig {
        const qreal SHIP_BASE_SPEED = 150.0;
        const qreal SHIP_BOOST_SPEED = 250.0;
        const qreal SHIP_BOOST_MULTIPLIER = 1.65;
        const int   MAX_STAMINA = 100;
        const int   BOOST_STAMINA_COST_PER_FRAME = 1;
        const int   TOP_BORDER = 60;
        const int   BOTTOM_BORDER = 700;
        const int   RIGHT_BORDER = 33000;
        const int   STAGE_COUNT = 6;
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
            // 1. 近海试航：捕鱼、移动、少量威胁，无 Boss。
            {4200, false,
             16, 22, 95, 60, 40, 0, 0,
             1, 780, 0, 0, 0, 0,
             6, 0,
             1900, 70, 30,
             100, 0, 0, 3600, 5400, 0},

            // 2. 外海遭遇：第一次 Boss 考试，压力仍然温和。
            {9000, false,
             18, 24, 90, 35, 50, 15, 0,
             2, 650, 1, 1100, 0, 0,
             7, 1,
             1500, 60, 40,
             80, 20, 0, 2700, 4500, 0},

            // 3. 暗流航段：环境和机动考验，无正式 Boss。
            {14200, true,
             18, 25, 85, 20, 45, 25, 10,
             2, 560, 1, 850, 1, 1300,
             8, 2,
             1200, 50, 50,
             55, 35, 10, 2200, 3900, 520},

            // 4. 深海裂谷：敌人、障碍、塔里海怪组合考验。
            {19800, false,
             20, 27, 80, 15, 35, 35, 15,
             2, 500, 2, 760, 1, 980,
             9, 2,
             1000, 45, 55,
             40, 35, 25, 1900, 3500, 460},

            // 5. 塞壬海域：最终压迫，暴风雨和逆浪权重更高。
            {25800, false,
             22, 29, 75, 10, 25, 40, 25,
             3, 440, 2, 650, 2, 860,
             10, 3,
             850, 40, 60,
             25, 30, 45, 1700, 3200, 390},

            {32200, true,
             24, 32, 70, 5, 20, 40, 35,
             3, 390, 3, 560, 2, 700,
             12, 4,
             700, 35, 65,
             15, 25, 60, 1500, 3000, 330}
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
            {6, 5, 0.86, 604, 0.70, 258, 102, -18, true}
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
            {6, 5, 0.94, 374, 0.36, 40, 40, false}
        };

        inline constexpr int TERRAIN_PROP_COUNT =
            static_cast<int>(sizeof(TERRAIN_PROPS) / sizeof(TERRAIN_PROPS[0]));

        struct StageText {
            const char* name;
            const char* brief;
            const char* clearSummary;
        };

        inline const StageText STAGE_TEXTS[STAGE_COUNT] = {
            {u8"\u8fd1\u5cb8\u7ec3\u4e60\u6d77\u57df",
             u8"\u4ece\u6e2f\u53e3\u5916\u4fa7\u51fa\u822a\uff0c\u719f\u6089\u6355\u9c7c\u4e0e\u907f\u969c\u8282\u594f\u3002",
             u8"\u6e2f\u53e3\u5916\u4fa7\u822a\u7ebf\u6062\u590d\u5b89\u5168\u3002"},
            {u8"\u5916\u6d77\u906d\u9047",
             u8"\u79bb\u5f00\u6d45\u6d77\u4fdd\u62a4\uff0c\u5f00\u59cb\u5e94\u5bf9\u6b63\u9762\u5a01\u80c1\u3002",
             u8"\u5916\u6d77\u822a\u7ebf\u7684\u7b2c\u4e00\u6b21\u5371\u673a\u5df2\u89e3\u9664\u3002"},
            {u8"\u6697\u6d41\u822a\u6bb5",
             u8"\u6d77\u6d41\u5f00\u59cb\u53d8\u5f97\u53cd\u590d\uff0c\u9c7c\u7fa4\u548c\u654c\u4eba\u66f4\u96be\u9884\u5224\u3002",
             u8"\u6697\u6d41\u533a\u5df2\u7ecf\u88ab\u7a7f\u8d8a\uff0c\u8239\u961f\u638c\u63e1\u4e86\u65b0\u7684\u822a\u7ebf\u3002"},
            {u8"\u6df1\u6d77\u88c2\u8c37",
             u8"\u6697\u7901\u4e0e\u6f29\u6da1\u5bc6\u96c6\uff0c\u9700\u8981\u540c\u65f6\u5904\u7406\u822a\u884c\u548c\u6218\u6597\u538b\u529b\u3002",
             u8"\u6df1\u6d77\u88c2\u8c37\u6682\u65f6\u5f52\u4e8e\u5e73\u9759\u3002"},
            {u8"\u585e\u58ec\u6d77\u57df",
             u8"\u66b4\u98ce\u96e8\u4e0e\u9006\u6d6a\u4ea4\u9519\uff0c\u524d\u65b9\u662f\u7ec8\u6d77\u95e8\u7684\u5916\u5708\u9632\u7ebf\u3002",
             u8"\u585e\u58ec\u7684\u8ff7\u96fe\u6563\u53bb\uff0c\u7ec8\u6d77\u95e8\u5df2\u7ecf\u5728\u524d\u65b9\u663e\u5f62\u3002"},
            {u8"\u7ec8\u6d77\u95e8",
             u8"\u6700\u540e\u7684\u957f\u7ebf\u822a\u6bb5\uff0c\u654c\u7fa4\u3001\u9006\u6d6a\u4e0e Boss \u4e00\u8d77\u538b\u4e0a\u6765\u3002",
             u8"\u7ec8\u6d77\u95e8\u88ab\u6253\u5f00\uff0c\u6e14\u9014\u822a\u7ebf\u5b8c\u6574\u8fde\u901a\u3002"}
        };

        inline const StageText& stageText(int stage)
        {
            return STAGE_TEXTS[qBound(0, stage - 1, STAGE_COUNT - 1)];
        }

        inline int stageStartDistance(int stage)
        {
            if (stage <= 1) return 0;
            return stageConfig(stage - 1).targetDistance;
        }
    }
}
