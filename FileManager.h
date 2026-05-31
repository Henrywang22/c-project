#pragma once

#include <QString>
#include <vector>

// ============================================================
// SaveData
// 游戏基础存档数据
//
// 说明：
// 这里保留原来的字段顺序，避免其他成员已经写好的 saveGame({...})
// 初始化代码大面积报错。
// 新增字段只放在后面。
// ============================================================

struct SaveData {
    int stage;
    int distance;
    int coins;
    int durability;
    int stamina;
    int fishCaught;
    int fishTotalValue;
    int gameSeconds;
    bool isDead;

    // 预留：后期联调 Player 时可以恢复这些数值
    int maxDurability;
    int maxStamina;

    // 船速升级后的基础速度
    float baseSpeed;
};

// ============================================================
// HighScoreEntry
// 排行榜记录
// ============================================================

struct HighScoreEntry {
    char name[20];

    int score;
    int distance;
    int kills;
    int fishCaught;
    int fishTotalValue;
    int gameSeconds;
    int stagesCleared;

    // 结算时可展示更多数据
    int coins;
    int durability;
    int stamina;
};

// ============================================================
// FishEntry
// 图鉴记录
// ============================================================

struct FishEntry {
    int fishID;
    bool discovered;
    char name[30];
};

// ============================================================
// FileManager
//
// 负责：
// 1. save.dat 存档
// 2. highscore.dat 排行榜
// 3. Log.dat 图鉴
//
// 这一版 saveGame / loadGame 会同时处理 InventorySystem。
// ============================================================

class FileManager {
public:
    FileManager();

    // -------------------------
    // 游戏存档
    // -------------------------
    void saveGame(const SaveData& data);
    bool loadGame(SaveData& data);
    bool hasSave();
    void deleteSave();

    // -------------------------
    // 图鉴
    // -------------------------
    void markFishDiscovered(int fishID, const char* fishName);
    bool isFishDiscovered(int fishID);

    // -------------------------
    // 排行榜
    // -------------------------
    int calculateScore(
        int stagesCleared,
        int fishTotalValue,
        int fishCaught,
        int kills,
        int coins,
        int durability,
        int stamina,
        int gameSeconds
    ) const;

    void saveHighScore(
        const char* name,
        int score,
        int distance,
        int kills,
        int fishCaught,
        int fishTotalValue,
        int gameSeconds,
        int stagesCleared
    );

    void saveHighScoreByStats(
        const char* name,
        int distance,
        int kills,
        int fishCaught,
        int fishTotalValue,
        int gameSeconds,
        int stagesCleared,
        int coins,
        int durability,
        int stamina
    );

    std::vector<HighScoreEntry> loadHighScores();
};
