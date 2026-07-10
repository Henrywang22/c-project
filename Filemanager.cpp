#define _CRT_SECURE_NO_WARNINGS

#include "FileManager.h"
#include "InventorySystem.h"
#include "GameConfig.h"

#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

// ============================================================
// 存档文件格式
// ============================================================

namespace {

    bool gSavePresenceKnown = false;
    bool gHasSave = false;

    QString dataFilePath(const char* fileName)
    {
        QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (root.isEmpty()) root = QDir::homePath() + QStringLiteral("/.fishing-voyage");
        QDir().mkpath(root);

        const QString name = QString::fromLatin1(fileName);
        const QString target = QDir(root).filePath(name);
        const QString legacyCandidates[] = {
            QDir::current().filePath(name),
            QDir(QCoreApplication::applicationDirPath()).filePath(name)
        };
        if (!QFileInfo::exists(target)) {
            for (const QString& legacy : legacyCandidates) {
                if (QFileInfo::exists(legacy) && QFile::copy(legacy, target)) break;
            }
        }
        return target;
    }

    std::string dataFilePathStd(const char* fileName)
    {
        return QDir::toNativeSeparators(dataFilePath(fileName)).toStdString();
    }

    bool isSaveDataSane(const SaveData& data)
    {
        return data.stage >= 1 && data.stage <= Config::GameConfig::STAGE_COUNT &&
               data.distance >= 0 && data.distance <= Config::GameConfig::RIGHT_BORDER &&
               data.coins >= 0 && data.coins <= 1000000000 &&
               data.maxDurability >= 1 && data.maxDurability <= 1000000 &&
               data.maxStamina >= 1 && data.maxStamina <= 1000000 &&
               data.durability >= 0 && data.durability <= data.maxDurability &&
               data.stamina >= 0 && data.stamina <= data.maxStamina &&
               data.fishCaught >= 0 && data.fishCaught <= 100000000 &&
               data.fishTotalValue >= 0 && data.fishTotalValue <= 1000000000 &&
               data.gameSeconds >= 0 && data.gameSeconds <= 1000000000 &&
               data.killCount >= 0 && data.killCount <= 100000000 &&
               std::isfinite(data.baseSpeed) && data.baseSpeed > 0.0f && data.baseSpeed <= 5000.0f;
    }

    const char SAVE_MAGIC[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '7', '\0' };
    const char SAVE_MAGIC_V6[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '6', '\0' };
    const char SAVE_MAGIC_V5[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '5', '\0' };
    const char SAVE_MAGIC_V4[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '4', '\0' };
    const char SAVE_MAGIC_V3[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '3', '\0' };
    const char SAVE_MAGIC_V2[8] = { 'Y', 'U', 'T', 'U', 'S', 'V', '2', '\0' };
    const int SAVE_VERSION = 7;
    const int SAVE_VERSION_V6 = 6;
    const int SAVE_VERSION_V5 = 5;
    const int SAVE_VERSION_V4 = 4;
    const int SAVE_VERSION_V3 = 3;
    const int SAVE_VERSION_V2 = 2;

    const int MAX_SAVE_WEAPONS = 32;
    const int LEGACY_SAVE_WEAPONS = 3;
    const int FISH_DISCOVERY_COUNT = 12;
    const int ENEMY_DISCOVERY_COUNT = 10;
    const int BOSS_DISCOVERY_COUNT = 10;
    const int EQUIPMENT_DISCOVERY_COUNT = 5;
    const int ITEM_DISCOVERY_COUNT = 5;
    const int ENEMY_DISCOVERY_OFFSET = FISH_DISCOVERY_COUNT;
    const int BOSS_DISCOVERY_OFFSET = FISH_DISCOVERY_COUNT + ENEMY_DISCOVERY_COUNT;
    const int EQUIPMENT_DISCOVERY_OFFSET =
        FISH_DISCOVERY_COUNT + ENEMY_DISCOVERY_COUNT + BOSS_DISCOVERY_COUNT;
    const int ITEM_DISCOVERY_OFFSET =
        EQUIPMENT_DISCOVERY_OFFSET + EQUIPMENT_DISCOVERY_COUNT;
    const int TOTAL_DISCOVERY_COUNT =
        FISH_DISCOVERY_COUNT + ENEMY_DISCOVERY_COUNT + BOSS_DISCOVERY_COUNT +
        EQUIPMENT_DISCOVERY_COUNT + ITEM_DISCOVERY_COUNT;

    struct SaveFileHeader {
        char magic[8];
        int version;
    };

    struct LegacyHighScoreEntry {
        char name[20];
        int score;
        int distance;
        int kills;
        int fishCaught;
        int fishTotalValue;
        int gameSeconds;
        int stagesCleared;
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

    struct InventorySaveBlockV6 {
        int foodCount;
        int shipRepairT1Count;
        int shipRepairT2Count;
        int shipRepairT3Count;
        int emergencyWeaponRepairCount;

        int weaponCount;
        int currentWeaponIndex;

        WeaponSaveBlock weapons[MAX_SAVE_WEAPONS];
        int quickWeaponSlots[6];
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
        int quickWeaponSlots[6];
        int itemPurchaseCounts[ITEM_DISCOVERY_COUNT];
    };

    struct InventorySaveBlockV5 {
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

    struct FullSaveDataV6 {
        SaveData core;
        InventorySaveBlockV6 inventory;
    };

    struct FullSaveDataV4 {
        SaveData core;
        InventorySaveBlockV4 inventory;
    };

    struct FullSaveDataV5 {
        SaveData core;
        InventorySaveBlockV5 inventory;
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
        const auto& quickSlots = inv.quickWeaponSlots();
        for (int slot = 0; slot < 6; ++slot) {
            block.quickWeaponSlots[slot] = quickSlots[slot];
        }
        for (int item = 0; item < ITEM_DISCOVERY_COUNT; ++item) {
            block.itemPurchaseCounts[item] = inv.getItemPurchaseCount(
                static_cast<InventoryItemType>(item));
        }

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
        for (int slot = 0; slot < 6; ++slot) {
            data.quickWeaponSlots[slot] = block.quickWeaponSlots[slot];
        }
        for (int item = 0; item < ITEM_DISCOVERY_COUNT; ++item) {
            data.itemPurchaseCounts[item] = block.itemPurchaseCounts[item];
        }

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

    void loadInventoryFromSaveBlockV6(const InventorySaveBlockV6& block)
    {
        InventorySystem::InventoryLoadData data;

        data.foodCount = block.foodCount;
        data.shipRepairT1Count = block.shipRepairT1Count;
        data.shipRepairT2Count = block.shipRepairT2Count;
        data.shipRepairT3Count = block.shipRepairT3Count;
        data.emergencyWeaponRepairCount = block.emergencyWeaponRepairCount;
        data.currentWeaponIndex = block.currentWeaponIndex;
        for (int slot = 0; slot < 6; ++slot) {
            data.quickWeaponSlots[slot] = block.quickWeaponSlots[slot];
        }

        const int weaponCount = qBound(0, block.weaponCount, MAX_SAVE_WEAPONS);
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

    void loadInventoryFromSaveBlockV5(const InventorySaveBlockV5& block)
    {
        InventorySystem::InventoryLoadData data;

        data.foodCount = block.foodCount;
        data.shipRepairT1Count = block.shipRepairT1Count;
        data.shipRepairT2Count = block.shipRepairT2Count;
        data.shipRepairT3Count = block.shipRepairT3Count;
        data.emergencyWeaponRepairCount = block.emergencyWeaponRepairCount;
        data.currentWeaponIndex = block.currentWeaponIndex;

        int weaponCount = qBound(0, block.weaponCount, MAX_SAVE_WEAPONS);
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
        const std::string logPath = dataFilePathStd("Log.dat");
        const std::streamoff expectedBytes =
            static_cast<std::streamoff>(TOTAL_DISCOVERY_COUNT * sizeof(FishEntry));

        std::ifstream existing(logPath, std::ios::binary | std::ios::ate);
        if (existing.is_open()) {
            std::streamoff currentBytes = existing.tellg();
            existing.close();

            if (currentBytes >= expectedBytes) {
                return;
            }

            const std::streamoff alignedBytes =
                (currentBytes / static_cast<std::streamoff>(sizeof(FishEntry))) *
                static_cast<std::streamoff>(sizeof(FishEntry));
            if (alignedBytes != currentBytes) {
                QFile truncated(dataFilePath("Log.dat"));
                if (!truncated.open(QIODevice::ReadWrite) || !truncated.resize(alignedBytes)) return;
                truncated.close();
                currentBytes = alignedBytes;
            }

            int startIndex = static_cast<int>(currentBytes / sizeof(FishEntry));
            std::ofstream append(logPath, std::ios::binary | std::ios::app);
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

        std::ofstream init(logPath, std::ios::binary);
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

        std::fstream f(dataFilePathStd("Log.dat"), std::ios::binary | std::ios::in | std::ios::out);
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

        std::ifstream f(dataFilePathStd("Log.dat"), std::ios::binary);
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

bool FileManager::saveGame(const SaveData& data)
{
    SaveFileHeader header;
    std::memset(&header, 0, sizeof(header));
    std::memcpy(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC));
    header.version = SAVE_VERSION;

    FullSaveData fullSave;
    std::memset(&fullSave, 0, sizeof(fullSave));

    fullSave.core = data;
    fullSave.inventory = makeInventorySaveBlock();

    QSaveFile file(dataFilePath("save.dat"));
    if (!file.open(QIODevice::WriteOnly)) return false;

    const qint64 headerBytes = file.write(
        reinterpret_cast<const char*>(&header), sizeof(header));
    const qint64 saveBytes = file.write(
        reinterpret_cast<const char*>(&fullSave), sizeof(fullSave));
    if (headerBytes != sizeof(header) || saveBytes != sizeof(fullSave) || !file.commit()) {
        file.cancelWriting();
        return false;
    }

    gSavePresenceKnown = true;
    gHasSave = true;
    return true;
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
    const QString savePath = dataFilePath("save.dat");
    std::ifstream f(dataFilePathStd("save.dat"), std::ios::binary);

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

    bool isV6Save =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC_V6, sizeof(SAVE_MAGIC_V6)) == 0
        && header.version == SAVE_VERSION_V6;

    bool isV3Save =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC_V3, sizeof(SAVE_MAGIC_V3)) == 0
        && header.version == SAVE_VERSION_V3;

    bool isV5Save =
        f.good()
        && std::memcmp(header.magic, SAVE_MAGIC_V5, sizeof(SAVE_MAGIC_V5)) == 0
        && header.version == SAVE_VERSION_V5;

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
        if (!isSaveDataSane(data)) return false;
        loadInventoryFromSaveBlock(fullSave.inventory);

        return true;
    }

    if (isV6Save) {
        FullSaveDataV6 oldSave;
        std::memset(&oldSave, 0, sizeof(oldSave));

        f.read(reinterpret_cast<char*>(&oldSave), sizeof(oldSave));
        if (!f.good()) return false;

        data = oldSave.core;
        if (!isSaveDataSane(data)) return false;
        loadInventoryFromSaveBlockV6(oldSave.inventory);
        return true;
    }

    if (isV5Save) {
        FullSaveDataV5 oldSave;
        std::memset(&oldSave, 0, sizeof(oldSave));

        f.read(reinterpret_cast<char*>(&oldSave), sizeof(oldSave));

        if (!f.good()) {
            return false;
        }

        data = oldSave.core;
        if (!isSaveDataSane(data)) return false;
        loadInventoryFromSaveBlockV5(oldSave.inventory);
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
        if (!isSaveDataSane(data)) return false;
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
        if (!isSaveDataSane(data)) return false;
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
        if (!isSaveDataSane(data)) return false;
        loadInventoryFromSaveBlockV3(oldSave.inventory);

        return true;
    }

    // 旧版无文件头存档只在长度精确匹配时迁移。未知或损坏的
    // 新版文件绝不能按旧结构解释，否则 magic 会被当作游戏数据。
    if (QFileInfo(savePath).size() != static_cast<qint64>(sizeof(SaveDataV2Core))) {
        return false;
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
    if (!isSaveDataSane(data)) return false;

    // 旧存档没有背包信息，给默认鱼竿
    InventorySystem::instance().clearAll();
    InventorySystem::instance().initDefaultWeaponIfNeeded();

    return true;
}

bool FileManager::hasSave()
{
    if (gSavePresenceKnown) return gHasSave;

    const QString savePath = dataFilePath("save.dat");
    QFile file(savePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gSavePresenceKnown = true;
        gHasSave = false;
        return false;
    }

    SaveFileHeader header{};
    const qint64 bytes = file.read(reinterpret_cast<char*>(&header), sizeof(header));
    const bool knownHeader = bytes == sizeof(header) && (
        (std::memcmp(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC)) == 0 && header.version == SAVE_VERSION) ||
        (std::memcmp(header.magic, SAVE_MAGIC_V6, sizeof(SAVE_MAGIC_V6)) == 0 && header.version == SAVE_VERSION_V6) ||
        (std::memcmp(header.magic, SAVE_MAGIC_V5, sizeof(SAVE_MAGIC_V5)) == 0 && header.version == SAVE_VERSION_V5) ||
        (std::memcmp(header.magic, SAVE_MAGIC_V4, sizeof(SAVE_MAGIC_V4)) == 0 && header.version == SAVE_VERSION_V4) ||
        (std::memcmp(header.magic, SAVE_MAGIC_V3, sizeof(SAVE_MAGIC_V3)) == 0 && header.version == SAVE_VERSION_V3) ||
        (std::memcmp(header.magic, SAVE_MAGIC_V2, sizeof(SAVE_MAGIC_V2)) == 0 && header.version == SAVE_VERSION_V2));
    const bool legacyExact = file.size() == static_cast<qint64>(sizeof(SaveDataV2Core));

    gSavePresenceKnown = true;
    gHasSave = knownHeader || legacyExact;
    return gHasSave;
}

void FileManager::deleteSave()
{
    QFile::remove(dataFilePath("save.dat"));
    QFile::remove(dataFilePath("save.tmp"));
    gSavePresenceKnown = true;
    gHasSave = false;
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

void FileManager::markEquipmentDiscovered(int equipmentID, const char* equipmentName)
{
    if (equipmentID < 0 || equipmentID >= EQUIPMENT_DISCOVERY_COUNT) {
        return;
    }

    markDiscoveryAt(EQUIPMENT_DISCOVERY_OFFSET + equipmentID, equipmentID, equipmentName);
}

bool FileManager::isEquipmentDiscovered(int equipmentID)
{
    if (equipmentID < 0 || equipmentID >= EQUIPMENT_DISCOVERY_COUNT) {
        return false;
    }

    return isDiscoverySet(EQUIPMENT_DISCOVERY_OFFSET + equipmentID);
}

void FileManager::markItemDiscovered(int itemID, const char* itemName)
{
    if (itemID < 0 || itemID >= ITEM_DISCOVERY_COUNT) {
        return;
    }

    markDiscoveryAt(ITEM_DISCOVERY_OFFSET + itemID, itemID, itemName);
}

bool FileManager::isItemDiscovered(int itemID)
{
    if (itemID < 0 || itemID >= ITEM_DISCOVERY_COUNT) {
        return false;
    }

    return isDiscoverySet(ITEM_DISCOVERY_OFFSET + itemID);
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
    const int cleared = qBound(0, stagesCleared, Config::GameConfig::STAGE_COUNT);
    const int stageScore = Config::SCORE_STAGE_MAX * cleared /
        qMax(1, Config::GameConfig::STAGE_COUNT);

    const int fastTargetSeconds = qMax(180, cleared * 85);
    const int slowLimitSeconds = qMax(fastTargetSeconds + 1, cleared * 210);
    const qreal timeRatio = gameSeconds <= fastTargetSeconds
        ? 1.0
        : qBound<qreal>(0.0,
            1.0 - static_cast<qreal>(gameSeconds - fastTargetSeconds) /
                (slowLimitSeconds - fastTargetSeconds),
            1.0);
    const int timeScore = qRound(Config::SCORE_TIME_MAX * timeRatio);

    const int fishCountTarget = qMax(1, cleared * 5);
    const int fishValueTarget = qMax(1, cleared * 380);
    const int killTarget = qMax(1, cleared * 4);
    const int fishCountScore = qRound(Config::SCORE_FISH_COUNT_MAX *
        qBound<qreal>(0.0, static_cast<qreal>(fishCaught) / fishCountTarget, 1.0));
    const int fishValueScore = qRound(Config::SCORE_FISH_VALUE_MAX *
        qBound<qreal>(0.0, static_cast<qreal>(fishTotalValue) / fishValueTarget, 1.0));
    const int killScore = qRound(Config::SCORE_KILL_MAX *
        qBound<qreal>(0.0, static_cast<qreal>(kills) / killTarget, 1.0));

    const qreal hullRatio = qBound<qreal>(0.0, durability / 100.0, 1.0);
    const qreal staminaRatio = qBound<qreal>(0.0, stamina / 100.0, 1.0);
    const int survivalScore = qRound(Config::SCORE_SURVIVAL_MAX *
                                     (hullRatio * 0.7 + staminaRatio * 0.3));

    // Coins are deliberately not scored: fish value and kills already reward
    // earning them, while spending on upgrades should never lower a grade.
    (void)coins;
    return qBound(0, stageScore + timeScore + fishCountScore + fishValueScore +
                       killScore + survivalScore, 10000);
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

    QSaveFile file(dataFilePath("highscore.dat"));
    if (!file.open(QIODevice::WriteOnly)) return;
    for (const auto& s : scores) {
        if (file.write(reinterpret_cast<const char*>(&s), sizeof(HighScoreEntry)) != sizeof(HighScoreEntry)) {
            file.cancelWriting();
            return;
        }
    }
    file.commit();
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

    QSaveFile file(dataFilePath("highscore.dat"));
    if (!file.open(QIODevice::WriteOnly)) return;
    for (const auto& s : scores) {
        if (file.write(reinterpret_cast<const char*>(&s), sizeof(HighScoreEntry)) != sizeof(HighScoreEntry)) {
            file.cancelWriting();
            return;
        }
    }
    file.commit();
}

std::vector<HighScoreEntry> FileManager::loadHighScores()
{
    std::vector<HighScoreEntry> scores;

    std::ifstream f(dataFilePathStd("highscore.dat"), std::ios::binary | std::ios::ate);

    if (!f.is_open()) {
        return scores;
    }

    const std::streamoff fileSize = f.tellg();
    f.seekg(0, std::ios::beg);

    if (fileSize > 0 && fileSize % static_cast<std::streamoff>(sizeof(HighScoreEntry)) == 0) {
        HighScoreEntry e;
        while (f.read(reinterpret_cast<char*>(&e), sizeof(HighScoreEntry))) {
            e.name[19] = '\0';
            scores.push_back(e);
        }
    }
    else if (fileSize > 0 && fileSize % static_cast<std::streamoff>(sizeof(LegacyHighScoreEntry)) == 0) {
        LegacyHighScoreEntry legacy;
        while (f.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) {
            HighScoreEntry e{};
            std::memcpy(e.name, legacy.name, sizeof(e.name));
            e.name[19] = '\0';
            e.score = legacy.score;
            e.distance = legacy.distance;
            e.kills = legacy.kills;
            e.fishCaught = legacy.fishCaught;
            e.fishTotalValue = legacy.fishTotalValue;
            e.gameSeconds = legacy.gameSeconds;
            e.stagesCleared = legacy.stagesCleared;
            scores.push_back(e);
        }
    }
    else {
        return scores;
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
