#define _CRT_SECURE_NO_WARNINGS

#include "FileManager.h"
#include "InventorySystem.h"
#include "GameConfig.h"

#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdio>

// ============================================================
// 存档文件格式
// ============================================================

namespace {

    const char SAVE_MAGIC[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '5', '\0' };
    const char SAVE_MAGIC_V4[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '4', '\0' };
    const char SAVE_MAGIC_V3[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '3', '\0' };
    const char SAVE_MAGIC_V2[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '2', '\0' };
    const int SAVE_VERSION = 5;
    const int SAVE_VERSION_V4 = 4;
    const int SAVE_VERSION_V3 = 3;
    const int SAVE_VERSION_V2 = 2;

    const int MAX_SAVE_WEAPONS = 32;
    const int LEGACY_SAVE_WEAPONS = 3;
    const int FISH_DISCOVERY_COUNT = 12;
    const int ENEMY_DISCOVERY_COUNT = 10;
    const int BOSS_DISCOVERY_COUNT = 10;
    const int ENEMY_DISCOVERY_OFFSET = FISH_DISCOVERY_COUNT;
    const int BOSS_DISCOVERY_OFFSET = FISH_DISCOVERY_COUNT + ENEMY_DISCOVERY_COUNT;
    const int TOTAL_DISCOVERY_COUNT =
        FISH_DISCOVERY_COUNT + ENEMY_DISCOVERY_COUNT + BOSS_DISCOVERY_COUNT;

    struct SaveFileHeader {
        char magic[8];
        int version;
    };

    struct WeaponSaveBlockV3 {
        char typeCode[20];

        int tier;
        int damage;
        int maxDurability;
        int currentDurability;
        int range;
        int durabilityConsumption;
    };

    struct WeaponSaveBlock {
        char typeCode[20];

        int tier;
        int damage;
        int maxDurability;
        int currentDurability;
        int range;
        int durabilityConsumption;
        int enhancementLevel;
    };

    struct InventorySaveBlockV3 {
        int foodCount;
        int shipRepairT1Count;
        int shipRepairT2Count;
        int shipRepairT3Count;
        int emergencyWeaponRepairCount;

        int weaponCount;
        int currentWeaponIndex;

        WeaponSaveBlockV3 weapons[LEGACY_SAVE_WEAPONS];
    };

    struct InventorySaveBlockV4 {
        int foodCount;
        int shipRepairT1Count;
        int shipRepairT2Count;
        int shipRepairT3Count;
        int emergencyWeaponRepairCount;

        int weaponCount;
        int currentWeaponIndex;

        WeaponSaveBlock weapons[LEGACY_SAVE_WEAPONS];
    };

    struct InventorySaveBlock {
        int foodCount;
        int shipRepairT1Count;
        int shipRepairT2Count;
        int shipRepairT3Count;
        int emergencyWeaponRepairCount;

        int weaponCount;
        int currentWeaponIndex;

        WeaponSaveBlock weapons[MAX_SAVE_WEAPONS];
    };

    struct SaveDataV2Core {
        int stage;
        int distance;
        int coins;
        int durability;
        int stamina;
        int fishCaught;
        int fishTotalValue;
        int gameSeconds;
        bool isDead;
        int maxDurability;
        int maxStamina;
    };

    struct SaveDataV3Core {
        int stage;
        int distance;
        int coins;
        int durability;
        int stamina;
        int fishCaught;
        int fishTotalValue;
        int gameSeconds;
        bool isDead;
        int maxDurability;
        int maxStamina;
        float baseSpeed;
    };

    struct FullSaveData {
        SaveData core;
        InventorySaveBlock inventory;
    };

    struct FullSaveDataV4 {
        SaveData core;
        InventorySaveBlockV4 inventory;
    };

    struct FullSaveDataV3 {
        SaveDataV3Core core;
        InventorySaveBlockV3 inventory;
    };

    struct FullSaveDataV2 {
        SaveDataV2Core core;
        InventorySaveBlockV3 inventory;
    };

    SaveData makeSaveDataFromV2(const SaveDataV2Core& oldCore)
    {
        SaveData data;
        std::memset(&data, 0, sizeof(SaveData));

        data.stage = oldCore.stage;
        data.distance = oldCore.distance;
        data.coins = oldCore.coins;
        data.durability = oldCore.durability;
        data.stamina = oldCore.stamina;
        data.fishCaught = oldCore.fishCaught;
        data.fishTotalValue = oldCore.fishTotalValue;
        data.gameSeconds = oldCore.gameSeconds;
        data.isDead = oldCore.isDead;
        data.maxDurability = oldCore.maxDurability;
        data.maxStamina = oldCore.maxStamina;
        data.baseSpeed = static_cast<float>(Config::GameConfig::SHIP_BASE_SPEED);
        data.killCount = 0;

        return data;
    }

    SaveData makeSaveDataFromV3(const SaveDataV3Core& oldCore)
    {
        SaveData data;
        std::memset(&data, 0, sizeof(SaveData));

        data.stage = oldCore.stage;
        data.distance = oldCore.distance;
        data.coins = oldCore.coins;
        data.durability = oldCore.durability;
        data.stamina = oldCore.stamina;
        data.fishCaught = oldCore.fishCaught;
        data.fishTotalValue = oldCore.fishTotalValue;
        data.gameSeconds = oldCore.gameSeconds;
        data.isDead = oldCore.isDead;
        data.maxDurability = oldCore.maxDurability;
        data.maxStamina = oldCore.maxStamina;
        data.baseSpeed = oldCore.baseSpeed;
        data.killCount = 0;

        return data;
    }

    void copyStringToCharArray(char* dest, int destSize, const char* src)
    {
        if (!dest || destSize <= 0) {
            return;
        }

        std::memset(dest, 0, destSize);

        if (!src) {
            return;
        }

        std::strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }

    InventorySaveBlock makeInventorySaveBlock()
    {
        InventorySaveBlock block;
        std::memset(&block, 0, sizeof(InventorySaveBlock));

        InventorySystem& inv = InventorySystem::instance();

        block.foodCount = inv.getItemCount(InventoryItemType::Food);
        block.shipRepairT1Count = inv.getItemCount(InventoryItemType::ShipRepairT1);
        block.shipRepairT2Count = inv.getItemCount(InventoryItemType::ShipRepairT2);
        block.shipRepairT3Count = inv.getItemCount(InventoryItemType::ShipRepairT3);
        block.emergencyWeaponRepairCount = inv.getItemCount(InventoryItemType::EmergencyWeaponRepair);

        const auto& weapons = inv.weapons();

        block.weaponCount = static_cast<int>(weapons.size());
        if (block.weaponCount > MAX_SAVE_WEAPONS) {
            block.weaponCount = MAX_SAVE_WEAPONS;
        }

        block.currentWeaponIndex = inv.currentWeaponIndex();

        for (int i = 0; i < block.weaponCount; ++i) {
            const Weapon* w = weapons[i];
            if (!w) {
                continue;
            }

            copyStringToCharArray(
                block.weapons[i].typeCode,
                20,
                w->getTypeCode().c_str()
            );

            block.weapons[i].tier = w->getTier();
            block.weapons[i].damage = w->getDamage();
            block.weapons[i].maxDurability = w->getMaxDur();
            block.weapons[i].currentDurability = w->getCurrentDur();
            block.weapons[i].range = w->getRange();
            block.weapons[i].durabilityConsumption = w->getDurabilityConsumption();
            block.weapons[i].enhancementLevel = w->getEnhancementLevel();
        }

        return block;
    }

    void loadInventoryFromSaveBlock(const InventorySaveBlock& block)
    {
        InventorySystem::InventoryLoadData data;

        data.foodCount = block.foodCount;
        data.shipRepairT1Count = block.shipRepairT1Count;
        data.shipRepairT2Count = block.shipRepairT2Count;
        data.shipRepairT3Count = block.shipRepairT3Count;
        data.emergencyWeaponRepairCount = block.emergencyWeaponRepairCount;

        data.currentWeaponIndex = block.currentWeaponIndex;

        int weaponCount = block.weaponCount;
        if (weaponCount < 0) {
            weaponCount = 0;
        }
        if (weaponCount > MAX_SAVE_WEAPONS) {
            weaponCount = MAX_SAVE_WEAPONS;
        }

        for (int i = 0; i < weaponCount; ++i) {
            InventorySystem::WeaponLoadData w;

            w.typeCode = block.weapons[i].typeCode;
            w.tier = block.weapons[i].tier;
            w.damage = block.weapons[i].damage;
            w.maxDurability = block.weapons[i].maxDurability;
            w.currentDurability = block.weapons[i].currentDurability;
            w.range = block.weapons[i].range;
            w.durabilityConsumption = block.weapons[i].durabilityConsumption;
            w.enhancementLevel = block.weapons[i].enhancementLevel;

            data.weapons.push_back(w);
        }

        InventorySystem::instance().loadFromData(data);
    }

    void loadInventoryFromSaveBlockV4(const InventorySaveBlockV4& block)
    {
        InventorySystem::InventoryLoadData data;

        data.foodCount = block.foodCount;
        data.shipRepairT1Count = block.shipRepairT1Count;
        data.shipRepairT2Count = block.shipRepairT2Count;
        data.shipRepairT3Count = block.shipRepairT3Count;
        data.emergencyWeaponRepairCount = block.emergencyWeaponRepairCount;
        data.currentWeaponIndex = block.currentWeaponIndex;

        int weaponCount = block.weaponCount;
        if (weaponCount < 0) {
            weaponCount = 0;
        }
        if (weaponCount > LEGACY_SAVE_WEAPONS) {
            weaponCount = LEGACY_SAVE_WEAPONS;
        }

        for (int i = 0; i < weaponCount; ++i) {
            InventorySystem::WeaponLoadData w;
            w.typeCode = block.weapons[i].typeCode;
            w.tier = block.weapons[i].tier;
            w.damage = block.weapons[i].damage;
            w.maxDurability = block.weapons[i].maxDurability;
            w.currentDurability = block.weapons[i].currentDurability;
            w.range = block.weapons[i].range;
            w.durabilityConsumption = block.weapons[i].durabilityConsumption;
            w.enhancementLevel = block.weapons[i].enhancementLevel;
            data.weapons.push_back(w);
        }

        InventorySystem::instance().loadFromData(data);
    }

    void loadInventoryFromSaveBlockV3(const InventorySaveBlockV3& block)
    {
        InventorySystem::InventoryLoadData data;

        data.foodCount = block.foodCount;
        data.shipRepairT1Count = block.shipRepairT1Count;
        data.shipRepairT2Count = block.shipRepairT2Count;
        data.shipRepairT3Count = block.shipRepairT3Count;
        data.emergencyWeaponRepairCount = block.emergencyWeaponRepairCount;

        data.currentWeaponIndex = block.currentWeaponIndex;

        int weaponCount = block.weaponCount;
        if (weaponCount < 0) {
            weaponCount = 0;
        }
        if (weaponCount > LEGACY_SAVE_WEAPONS) {
            weaponCount = LEGACY_SAVE_WEAPONS;
        }

        for (int i = 0; i < weaponCount; ++i) {
            InventorySystem::WeaponLoadData w;

            w.typeCode = block.weapons[i].typeCode;
            w.tier = block.weapons[i].tier;
            w.damage = block.weapons[i].damage;
            w.maxDurability = block.weapons[i].maxDurability;
            w.currentDurability = block.weapons[i].currentDurability;
            w.range = block.weapons[i].range;
            w.durabilityConsumption = block.weapons[i].durabilityConsumption;
            w.enhancementLevel = 0;

            data.weapons.push_back(w);
        }

        InventorySystem::instance().loadFromData(data);
    }

    void initEmptyFishLogIfNeeded()
    {
        const std::streamoff expectedBytes =
            static_cast<std::streamoff>(TOTAL_DISCOVERY_COUNT * sizeof(FishEntry));

        std::ifstream existing("Log.dat", std::ios::binary | std::ios::ate);
        if (existing.is_open()) {
            std::streamoff currentBytes = existing.tellg();
            existing.close();

            if (currentBytes >= expectedBytes) {
                return;
            }

            int startIndex = static_cast<int>(currentBytes / sizeof(FishEntry));
            std::ofstream append("Log.dat", std::ios::binary | std::ios::app);
            if (!append.is_open()) {
                return;
            }

            FishEntry empty = { 0, false, "" };
            for (int i = startIndex; i < TOTAL_DISCOVERY_COUNT; ++i) {
                empty.fishID = i;
                empty.discovered = false;
                std::memset(empty.name, 0, sizeof(empty.name));
                append.write(reinterpret_cast<char*>(&empty), sizeof(FishEntry));
            }
            return;
        }

        std::ofstream init("Log.dat", std::ios::binary);
        FishEntry empty = { 0, false, "" };

        for (int i = 0; i < TOTAL_DISCOVERY_COUNT; i++) {
            empty.fishID = i;
            empty.discovered = false;
            std::memset(empty.name, 0, sizeof(empty.name));
            init.write(reinterpret_cast<char*>(&empty), sizeof(FishEntry));
        }
    }

    void markDiscoveryAt(int logIndex, int entryID, const char* entryName)
    {
        if (logIndex < 0 || logIndex >= TOTAL_DISCOVERY_COUNT) {
            return;
        }

        initEmptyFishLogIfNeeded();

        std::fstream f("Log.dat", std::ios::binary | std::ios::in | std::ios::out);
        if (!f.is_open()) {
            return;
        }

        FishEntry entry;
        std::memset(&entry, 0, sizeof(entry));

        f.seekg(logIndex * sizeof(FishEntry), std::ios::beg);
        f.read(reinterpret_cast<char*>(&entry), sizeof(FishEntry));

        entry.fishID = entryID;
        entry.discovered = true;
        copyStringToCharArray(entry.name, 30, entryName);

        f.clear();
        f.seekp(logIndex * sizeof(FishEntry), std::ios::beg);
        f.write(reinterpret_cast<char*>(&entry), sizeof(FishEntry));
    }

    bool isDiscoverySet(int logIndex)
    {
        if (logIndex < 0 || logIndex >= TOTAL_DISCOVERY_COUNT) {
            return false;
        }

        initEmptyFishLogIfNeeded();

        std::ifstream f("Log.dat", std::ios::binary);
        if (!f.is_open()) {
            return false;
        }

        FishEntry entry;
        std::memset(&entry, 0, sizeof(entry));

        f.seekg(logIndex * sizeof(FishEntry), std::ios::beg);
        f.read(reinterpret_cast<char*>(&entry), sizeof(FishEntry));

        if (!f.good()) {
            return false;
        }

        return entry.discovered;
    }
}

// ============================================================
// 构造函数
// ============================================================

FileManager::FileManager()
{
    initEmptyFishLogIfNeeded();
}

// ============================================================
// 保存游戏
// saveGame 会自动保存 InventorySystem 当前背包。
// ============================================================

void FileManager::saveGame(const SaveData& data)
{
    SaveFileHeader header;
    std::memset(&header, 0, sizeof(header));
    std::memcpy(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC));
    header.version = SAVE_VERSION;

    FullSaveData fullSave;
    std::memset(&fullSave, 0, sizeof(fullSave));

    fullSave.core = data;
    fullSave.inventory = makeInventorySaveBlock();

    std::ofstream f("save.dat", std::ios::binary);

    if (!f.is_open()) {
        return;
    }

    f.write(reinterpret_cast<const char*>(&header), sizeof(header));
    f.write(reinterpret_cast<const char*>(&fullSave), sizeof(fullSave));
}

// ============================================================
// 读取游戏
// 兼容旧版 save.dat：
// 1. 如果识别到 YUTUSV4，就读取完整背包、船速、击杀数和强化次数。
// 2. 如果识别到 YUTUSV3，就读取完整背包和船速，并补齐新字段。
// 3. 如果识别到 YUTUSV2，就读取完整背包，并用默认船速补齐。
// 4. 如果不是新格式，就按旧 SaveData 读取，并初始化默认装备。
// ============================================================

bool FileManager::loadGame(SaveData& data)
{
    std::ifstream f("save.dat", std::ios::binary);

    if (!f.is_open()) {
        return false;
    }

    SaveFileHeader header;
    std::memset(&header, 0, sizeof(header));

    f.read(reinterpret_cast<char*>(&header), sizeof(header));

    bool isNewSave =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC)) == 0
        && header.version == SAVE_VERSION;

    bool isV3Save =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC_V3, sizeof(SAVE_MAGIC_V3)) == 0
        && header.version == SAVE_VERSION_V3;

    bool isV4Save =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC_V4, sizeof(SAVE_MAGIC_V4)) == 0
        && header.version == SAVE_VERSION_V4;

    bool isV2Save =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC_V2, sizeof(SAVE_MAGIC_V2)) == 0
        && header.version == SAVE_VERSION_V2;

    if (isNewSave) {
        FullSaveData fullSave;
        std::memset(&fullSave, 0, sizeof(fullSave));

        f.read(reinterpret_cast<char*>(&fullSave), sizeof(fullSave));

        if (!f.good()) {
            return false;
        }

        data = fullSave.core;
        loadInventoryFromSaveBlock(fullSave.inventory);

        return true;
    }

    if (isV4Save) {
        FullSaveDataV4 oldSave;
        std::memset(&oldSave, 0, sizeof(oldSave));

        f.read(reinterpret_cast<char*>(&oldSave), sizeof(oldSave));

        if (!f.good()) {
            return false;
        }

        data = oldSave.core;
        loadInventoryFromSaveBlockV4(oldSave.inventory);

        return true;
    }

    if (isV3Save) {
        FullSaveDataV3 oldSave;
        std::memset(&oldSave, 0, sizeof(oldSave));

        f.read(reinterpret_cast<char*>(&oldSave), sizeof(oldSave));

        if (!f.good()) {
            return false;
        }

        data = makeSaveDataFromV3(oldSave.core);
        loadInventoryFromSaveBlockV3(oldSave.inventory);

        return true;
    }

    if (isV2Save) {
        FullSaveDataV2 oldSave;
        std::memset(&oldSave, 0, sizeof(oldSave));

        f.read(reinterpret_cast<char*>(&oldSave), sizeof(oldSave));

        if (!f.good()) {
            return false;
        }

        data = makeSaveDataFromV2(oldSave.core);
        loadInventoryFromSaveBlockV3(oldSave.inventory);

        return true;
    }

    // 旧版存档兼容
    f.clear();
    f.seekg(0, std::ios::beg);

    SaveDataV2Core oldData;
    std::memset(&oldData, 0, sizeof(oldData));

    f.read(reinterpret_cast<char*>(&oldData), sizeof(SaveDataV2Core));

    if (!f.good()) {
        return false;
    }

    data = makeSaveDataFromV2(oldData);

    // 旧存档没有背包信息，给默认鱼竿
    InventorySystem::instance().clearAll();
    InventorySystem::instance().initDefaultWeaponIfNeeded();

    return true;
}

bool FileManager::hasSave()
{
    std::ifstream f("save.dat", std::ios::binary);
    return f.is_open();
}

void FileManager::deleteSave()
{
    std::remove("save.dat");
}

// ============================================================
// 图鉴系统
// ============================================================

void FileManager::markFishDiscovered(int fishID, const char* fishName)
{
    if (fishID < 0 || fishID >= FISH_DISCOVERY_COUNT) {
        return;
    }

    markDiscoveryAt(fishID, fishID, fishName);
}

bool FileManager::isFishDiscovered(int fishID)
{
    if (fishID < 0 || fishID >= FISH_DISCOVERY_COUNT) {
        return false;
    }

    return isDiscoverySet(fishID);
}

void FileManager::markEnemyDiscovered(int enemyID, const char* enemyName)
{
    if (enemyID < 0 || enemyID >= ENEMY_DISCOVERY_COUNT) {
        return;
    }

    markDiscoveryAt(ENEMY_DISCOVERY_OFFSET + enemyID, enemyID, enemyName);
}

bool FileManager::isEnemyDiscovered(int enemyID)
{
    if (enemyID < 0 || enemyID >= ENEMY_DISCOVERY_COUNT) {
        return false;
    }

    return isDiscoverySet(ENEMY_DISCOVERY_OFFSET + enemyID);
}

void FileManager::markBossDiscovered(int bossID, const char* bossName)
{
    if (bossID < 0 || bossID >= BOSS_DISCOVERY_COUNT) {
        return;
    }

    markDiscoveryAt(BOSS_DISCOVERY_OFFSET + bossID, bossID, bossName);
}

bool FileManager::isBossDiscovered(int bossID)
{
    if (bossID < 0 || bossID >= BOSS_DISCOVERY_COUNT) {
        return false;
    }

    return isDiscoverySet(BOSS_DISCOVERY_OFFSET + bossID);
}

// ============================================================
// 排行榜综合得分
// ============================================================

int FileManager::calculateScore(
    int stagesCleared,
    int fishTotalValue,
    int fishCaught,
    int kills,
    int coins,
    int durability,
    int stamina,
    int gameSeconds
) const
{
    int score =
        stagesCleared * Config::SCORE_STAGE_WEIGHT
        + fishTotalValue * Config::SCORE_FISH_VALUE_WEIGHT
        + fishCaught * Config::SCORE_FISH_COUNT_WEIGHT
        + kills * Config::SCORE_KILL_WEIGHT
        + coins * Config::SCORE_COIN_WEIGHT
        + durability * Config::SCORE_DURABILITY_WEIGHT
        + stamina * Config::SCORE_STAMINA_WEIGHT
        - gameSeconds * Config::SCORE_TIME_PENALTY;

    if (score < 0) {
        score = 0;
    }

    return score;
}

// ============================================================
// 保存排行榜
// 保留旧接口：外部如果已经算好 score，可以继续用。
// ============================================================

void FileManager::saveHighScore(
    const char* name,
    int score,
    int distance,
    int kills,
    int fishCaught,
    int fishTotalValue,
    int gameSeconds,
    int stagesCleared
)
{
    std::vector<HighScoreEntry> scores = loadHighScores();

    HighScoreEntry e;
    std::memset(&e, 0, sizeof(e));

    copyStringToCharArray(e.name, 20, name);

    e.score = score;
    e.distance = distance;
    e.kills = kills;
    e.fishCaught = fishCaught;
    e.fishTotalValue = fishTotalValue;
    e.gameSeconds = gameSeconds;
    e.stagesCleared = stagesCleared;

    e.coins = 0;
    e.durability = 0;
    e.stamina = 0;

    scores.push_back(e);

    std::sort(
        scores.begin(),
        scores.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        }
    );

    if (scores.size() > 10) {
        scores.resize(10);
    }

    std::ofstream f("highscore.dat", std::ios::binary);

    if (!f.is_open()) {
        return;
    }

    for (const auto& s : scores) {
        f.write(reinterpret_cast<const char*>(&s), sizeof(HighScoreEntry));
    }
}

// ============================================================
// 保存排行榜：按数据自动计算综合得分
// 这个是后面最终结算推荐使用的接口。
// ============================================================

void FileManager::saveHighScoreByStats(
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
)
{
    int score = calculateScore(
        stagesCleared,
        fishTotalValue,
        fishCaught,
        kills,
        coins,
        durability,
        stamina,
        gameSeconds
    );

    std::vector<HighScoreEntry> scores = loadHighScores();

    HighScoreEntry e;
    std::memset(&e, 0, sizeof(e));

    copyStringToCharArray(e.name, 20, name);

    e.score = score;
    e.distance = distance;
    e.kills = kills;
    e.fishCaught = fishCaught;
    e.fishTotalValue = fishTotalValue;
    e.gameSeconds = gameSeconds;
    e.stagesCleared = stagesCleared;
    e.coins = coins;
    e.durability = durability;
    e.stamina = stamina;

    scores.push_back(e);

    std::sort(
        scores.begin(),
        scores.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        }
    );

    if (scores.size() > 10) {
        scores.resize(10);
    }

    std::ofstream f("highscore.dat", std::ios::binary);

    if (!f.is_open()) {
        return;
    }

    for (const auto& s : scores) {
        f.write(reinterpret_cast<const char*>(&s), sizeof(HighScoreEntry));
    }
}

std::vector<HighScoreEntry> FileManager::loadHighScores()
{
    std::vector<HighScoreEntry> scores;

    std::ifstream f("highscore.dat", std::ios::binary);

    if (!f.is_open()) {
        return scores;
    }

    HighScoreEntry e;

    while (f.read(reinterpret_cast<char*>(&e), sizeof(HighScoreEntry))) {
        e.name[19] = '\0';
        scores.push_back(e);
    }

    std::sort(
        scores.begin(),
        scores.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        }
    );

    if (scores.size() > 10) {
        scores.resize(10);
    }

    return scores;
}
