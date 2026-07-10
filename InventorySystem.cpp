#include "InventorySystem.h"
#include "Player.h"
#include "ItemFactory.h"
#include "FileManager.h"
#include <algorithm>

namespace {

void markEquipmentDiscoveryForWeapon(const Weapon* weapon)
{
    if (!weapon) return;

    const std::string type = weapon->getTypeCode();
    FileManager fileManager;
    if (type == "Rod") {
        fileManager.markEquipmentDiscovered(0, "Rod");
    }
    else if (type == "Net") {
        fileManager.markEquipmentDiscovered(1, "Net");
    }
    else if (type == "Harpoon") {
        fileManager.markEquipmentDiscovered(2, "Harpoon");
    }
    else if (type == "Pistol") {
        fileManager.markEquipmentDiscovered(3, "Pistol");
    }
    else if (type == "Shotgun") {
        fileManager.markEquipmentDiscovered(4, "Shotgun");
    }
}

void markItemDiscovery(InventoryItemType type)
{
    FileManager fileManager;
    switch (type) {
    case InventoryItemType::Food:
        fileManager.markItemDiscovered(0, "Food");
        break;
    case InventoryItemType::ShipRepairT1:
        fileManager.markItemDiscovered(1, "Ship Repair T1");
        break;
    case InventoryItemType::ShipRepairT2:
        fileManager.markItemDiscovered(2, "Ship Repair T2");
        break;
    case InventoryItemType::ShipRepairT3:
        fileManager.markItemDiscovered(3, "Ship Repair T3");
        break;
    case InventoryItemType::EmergencyWeaponRepair:
        fileManager.markItemDiscovered(4, "Emergency Weapon Repair");
        break;
    }
}

}

InventorySystem& InventorySystem::instance()
{
    static InventorySystem inv;
    return inv;
}

InventorySystem::InventorySystem()
{
    initDefaultWeaponIfNeeded();
}

InventorySystem::~InventorySystem()
{
    clearAll();
}

void InventorySystem::initDefaultWeaponIfNeeded()
{
    if (!m_weapons.empty()) {
        return;
    }

    Weapon* defaultRod = ItemFactory::createWeapon("Rod", 1);
    if (defaultRod) {
        defaultRod->makeDurabilityInfinite();
        m_weapons.push_back(defaultRod);
        markEquipmentDiscoveryForWeapon(defaultRod);
    }

    Weapon* starterHarpoon = ItemFactory::createWeapon("Harpoon", 1);
    if (starterHarpoon) {
        starterHarpoon->makeDurabilityInfinite();
        m_weapons.push_back(starterHarpoon);
        markEquipmentDiscoveryForWeapon(starterHarpoon);
    }

    if (!m_weapons.empty()) {
        m_quickWeaponSlots.fill(-1);
        for (int i = 0; i < static_cast<int>(m_weapons.size()) && i < 6; ++i) {
            m_quickWeaponSlots[i] = i;
        }
        m_currentWeaponIndex = static_cast<int>(m_weapons.size()) > 1 ? 1 : 0;
        Player::instance().equipWeapon(m_weapons[m_currentWeaponIndex]);
    }
}

bool InventorySystem::canAddItem(int count) const
{
    if (count <= 0) {
        return false;
    }

    const int remaining = qMax(0, Config::MAX_ITEM_BACKPACK - getTotalItemCount());
    return count <= remaining;
}

bool InventorySystem::addItem(InventoryItemType type, int count)
{
    if (count <= 0) {
        return false;
    }

    if (!canAddItem(count)) {
        return false;
    }

    switch (type) {
    case InventoryItemType::Food:
        m_foodCount += count;
        markItemDiscovery(type);
        return true;

    case InventoryItemType::ShipRepairT1:
        m_shipRepairT1Count += count;
        markItemDiscovery(type);
        return true;

    case InventoryItemType::ShipRepairT2:
        m_shipRepairT2Count += count;
        markItemDiscovery(type);
        return true;

    case InventoryItemType::ShipRepairT3:
        m_shipRepairT3Count += count;
        markItemDiscovery(type);
        return true;

    case InventoryItemType::EmergencyWeaponRepair:
        m_emergencyWeaponRepairCount += count;
        markItemDiscovery(type);
        return true;
    }

    return false;
}

bool InventorySystem::useFood(Player& player)
{
    if (m_foodCount <= 0) {
        return false;
    }

    const int staminaBefore = player.stamina();
    player.restoreStamina(Config::HEAL_FOOD_RATION);
    if (player.stamina() <= staminaBefore) {
        return false;
    }
    m_foodCount--;
    return true;
}

bool InventorySystem::useShipRepairKit(Player& player, int tier)
{
    if (player.durability() >= player.maxDurability) {
        return false;
    }

    if (tier == 1) {
        if (m_shipRepairT1Count <= 0) return false;
        player.restoreDurability(Config::HEAL_REPAIR_T1);
        m_shipRepairT1Count--;
        return true;
    }

    if (tier == 2) {
        if (m_shipRepairT2Count <= 0) return false;
        player.restoreDurability(Config::HEAL_REPAIR_T2);
        m_shipRepairT2Count--;
        return true;
    }

    if (tier == 3) {
        if (m_shipRepairT3Count <= 0) return false;
        player.restoreDurability(Config::HEAL_REPAIR_T3);
        m_shipRepairT3Count--;
        return true;
    }

    return false;
}

bool InventorySystem::useEmergencyWeaponRepair(int weaponIndex)
{
    if (m_emergencyWeaponRepairCount <= 0) {
        return false;
    }

    if (weaponIndex < 0 || weaponIndex >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    Weapon* weapon = m_weapons[weaponIndex];
    if (!weapon) {
        return false;
    }

    if (weapon->getCurrentDur() >= weapon->getMaxDur()) {
        return false;
    }

    weapon->repairByPercent(Config::EMERGENCY_WEAPON_REPAIR_PERCENT);
    m_emergencyWeaponRepairCount--;
    return true;
}

int InventorySystem::getItemCount(InventoryItemType type) const
{
    switch (type) {
    case InventoryItemType::Food:
        return m_foodCount;

    case InventoryItemType::ShipRepairT1:
        return m_shipRepairT1Count;

    case InventoryItemType::ShipRepairT2:
        return m_shipRepairT2Count;

    case InventoryItemType::ShipRepairT3:
        return m_shipRepairT3Count;

    case InventoryItemType::EmergencyWeaponRepair:
        return m_emergencyWeaponRepairCount;
    }

    return 0;
}

int InventorySystem::getTotalItemCount() const
{
    return m_foodCount
        + m_shipRepairT1Count
        + m_shipRepairT2Count
        + m_shipRepairT3Count
        + m_emergencyWeaponRepairCount;
}

int InventorySystem::getItemPurchaseCount(InventoryItemType type) const
{
    const int index = static_cast<int>(type);
    if (index < 0 || index >= static_cast<int>(m_itemPurchaseCounts.size())) {
        return 0;
    }
    return m_itemPurchaseCounts[index];
}

void InventorySystem::recordItemPurchase(InventoryItemType type)
{
    const int index = static_cast<int>(type);
    if (index < 0 || index >= static_cast<int>(m_itemPurchaseCounts.size())) {
        return;
    }
    m_itemPurchaseCounts[index] = qMin(1000000, m_itemPurchaseCounts[index] + 1);
}

bool InventorySystem::canAddWeapon() const
{
    return static_cast<int>(m_weapons.size()) < Config::MAX_WEAPON_BACKPACK;
}

bool InventorySystem::addWeapon(Weapon* weapon)
{
    if (!weapon) {
        return false;
    }

    if (!canAddWeapon()) {
        return false;
    }

    m_weapons.push_back(weapon);
    markEquipmentDiscoveryForWeapon(weapon);
    const int newIndex = static_cast<int>(m_weapons.size()) - 1;
    for (int slot = 0; slot < 6; ++slot) {
        if (m_quickWeaponSlots[slot] < 0) {
            m_quickWeaponSlots[slot] = newIndex;
            break;
        }
    }

    if (m_currentWeaponIndex < 0) {
        m_currentWeaponIndex = 0;
        Player::instance().equipWeapon(m_weapons[0]);
    }

    return true;
}

bool InventorySystem::replaceWeapon(int index, Weapon* weapon)
{
    if (!weapon) {
        return false;
    }

    if (index < 0 || index >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    delete m_weapons[index];
    m_weapons[index] = weapon;
    markEquipmentDiscoveryForWeapon(weapon);

    if (m_currentWeaponIndex == index) {
        Player::instance().equipWeapon(weapon);
    }

    return true;
}

bool InventorySystem::removeWeapon(int index)
{
    if (index < 0 || index >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    Weapon* weapon = m_weapons[index];
    if (!weapon || !weapon->isBroken()) {
        return false;
    }

    const bool removedCurrent = m_currentWeaponIndex == index;
    delete weapon;
    m_weapons.erase(m_weapons.begin() + index);

    for (int& slotWeaponIndex : m_quickWeaponSlots) {
        if (slotWeaponIndex == index) {
            slotWeaponIndex = -1;
        }
        else if (slotWeaponIndex > index) {
            --slotWeaponIndex;
        }
    }

    if (m_currentWeaponIndex > index) {
        --m_currentWeaponIndex;
    }
    else if (removedCurrent) {
        m_currentWeaponIndex = -1;
        for (int i = 0; i < static_cast<int>(m_weapons.size()); ++i) {
            if (m_weapons[i] && !m_weapons[i]->isBroken()) {
                m_currentWeaponIndex = i;
                break;
            }
        }
    }

    Player::instance().equipWeapon(
        m_currentWeaponIndex >= 0 ? m_weapons[m_currentWeaponIndex] : nullptr
    );
    return true;
}

bool InventorySystem::selectWeapon(int index)
{
    if (index < 0 || index >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    Weapon* weapon = m_weapons[index];
    if (!weapon || weapon->isBroken()) {
        return false;
    }

    m_currentWeaponIndex = index;

    // 兼容旧接口
    Player::instance().equipWeapon(weapon);

    return true;
}

bool InventorySystem::selectQuickWeaponSlot(int slotIndex)
{
    return selectWeapon(weaponIndexForQuickSlot(slotIndex));
}

bool InventorySystem::assignWeaponToQuickSlot(int weaponIndex, int slotIndex)
{
    if (weaponIndex < 0 || weaponIndex >= static_cast<int>(m_weapons.size()) ||
        slotIndex < 0 || slotIndex >= 6 || !m_weapons[weaponIndex]) {
        return false;
    }

    const int previousSlot = quickSlotForWeapon(weaponIndex);
    const int displacedWeapon = m_quickWeaponSlots[slotIndex];
    m_quickWeaponSlots[slotIndex] = weaponIndex;

    if (previousSlot >= 0 && previousSlot != slotIndex) {
        m_quickWeaponSlots[previousSlot] = displacedWeapon;
    }
    // A weapon without a previous slot replaces the target slot directly.
    // The displaced weapon remains safely stored in the backpack, unassigned.
    return true;
}

Weapon* InventorySystem::currentWeapon()
{
    if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= static_cast<int>(m_weapons.size())) {
        return nullptr;
    }

    return m_weapons[m_currentWeaponIndex];
}

const Weapon* InventorySystem::currentWeapon() const
{
    if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= static_cast<int>(m_weapons.size())) {
        return nullptr;
    }

    return m_weapons[m_currentWeaponIndex];
}

int InventorySystem::currentWeaponIndex() const
{
    return m_currentWeaponIndex;
}

int InventorySystem::weaponCount() const
{
    return static_cast<int>(m_weapons.size());
}

int InventorySystem::maxWeaponCapacity() const
{
    return Config::MAX_WEAPON_BACKPACK;
}

int InventorySystem::weaponIndexForQuickSlot(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= 6) {
        return -1;
    }
    const int weaponIndex = m_quickWeaponSlots[slotIndex];
    return weaponIndex >= 0 && weaponIndex < static_cast<int>(m_weapons.size())
        ? weaponIndex
        : -1;
}

int InventorySystem::quickSlotForWeapon(int weaponIndex) const
{
    for (int slot = 0; slot < 6; ++slot) {
        if (m_quickWeaponSlots[slot] == weaponIndex) {
            return slot;
        }
    }
    return -1;
}

const std::array<int, 6>& InventorySystem::quickWeaponSlots() const
{
    return m_quickWeaponSlots;
}

const std::vector<Weapon*>& InventorySystem::weapons() const
{
    return m_weapons;
}

bool InventorySystem::repairWeaponByPercent(int index, int percent)
{
    if (index < 0 || index >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    Weapon* weapon = m_weapons[index];
    if (!weapon) {
        return false;
    }

    if (weapon->getCurrentDur() >= weapon->getMaxDur()) {
        return false;
    }

    weapon->repairByPercent(percent);
    return true;
}

bool InventorySystem::repairWeaponToFull(int index)
{
    if (index < 0 || index >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    Weapon* weapon = m_weapons[index];
    if (!weapon) {
        return false;
    }

    if (weapon->getCurrentDur() >= weapon->getMaxDur()) {
        return false;
    }

    weapon->repairToFull();
    return true;
}

bool InventorySystem::upgradeWeapon(int index, int damageBoost, int durabilityBoost)
{
    if (index < 0 || index >= static_cast<int>(m_weapons.size())) {
        return false;
    }

    Weapon* weapon = m_weapons[index];
    if (!weapon) {
        return false;
    }

    weapon->upgradeStats(damageBoost, durabilityBoost);
    return true;
}

void InventorySystem::clearAll()
{
    Player::instance().equipWeapon(nullptr);
    for (Weapon* weapon : m_weapons) {
        delete weapon;
    }

    m_weapons.clear();
    m_currentWeaponIndex = -1;
    m_quickWeaponSlots.fill(-1);

    m_foodCount = 0;
    m_shipRepairT1Count = 0;
    m_shipRepairT2Count = 0;
    m_shipRepairT3Count = 0;
    m_emergencyWeaponRepairCount = 0;
    m_itemPurchaseCounts.fill(0);
}

InventorySystem::InventoryLoadData InventorySystem::exportData() const
{
    InventoryLoadData data;

    data.foodCount = m_foodCount;
    data.shipRepairT1Count = m_shipRepairT1Count;
    data.shipRepairT2Count = m_shipRepairT2Count;
    data.shipRepairT3Count = m_shipRepairT3Count;
    data.emergencyWeaponRepairCount = m_emergencyWeaponRepairCount;
    data.itemPurchaseCounts = m_itemPurchaseCounts;

    data.currentWeaponIndex = m_currentWeaponIndex;
    data.quickWeaponSlots = m_quickWeaponSlots;

    for (const Weapon* weapon : m_weapons) {
        if (!weapon) {
            continue;
        }

        WeaponLoadData w;

        w.typeCode = weapon->getTypeCode();
        w.tier = weapon->getTier();
        w.damage = weapon->getDamage();
        w.maxDurability = weapon->getMaxDur();
        w.currentDurability = weapon->getCurrentDur();
        w.range = weapon->getRange();
        w.durabilityConsumption = weapon->getDurabilityConsumption();
        w.enhancementLevel = weapon->getEnhancementLevel();

        data.weapons.push_back(w);
    }

    return data;
}

void InventorySystem::loadFromData(const InventoryLoadData& data)
{
    clearAll();

    int remainingItems = Config::MAX_ITEM_BACKPACK;
    auto takeValidatedCount = [&](int savedCount) {
        const int count = qBound(0, savedCount, remainingItems);
        remainingItems -= count;
        return count;
    };
    m_foodCount = takeValidatedCount(data.foodCount);
    m_shipRepairT1Count = takeValidatedCount(data.shipRepairT1Count);
    m_shipRepairT2Count = takeValidatedCount(data.shipRepairT2Count);
    m_shipRepairT3Count = takeValidatedCount(data.shipRepairT3Count);
    m_emergencyWeaponRepairCount = takeValidatedCount(data.emergencyWeaponRepairCount);
    for (int i = 0; i < static_cast<int>(m_itemPurchaseCounts.size()); ++i) {
        m_itemPurchaseCounts[i] = qBound(0, data.itemPurchaseCounts[i], 1000000);
    }
    if (m_foodCount > 0) markItemDiscovery(InventoryItemType::Food);
    if (m_shipRepairT1Count > 0) markItemDiscovery(InventoryItemType::ShipRepairT1);
    if (m_shipRepairT2Count > 0) markItemDiscovery(InventoryItemType::ShipRepairT2);
    if (m_shipRepairT3Count > 0) markItemDiscovery(InventoryItemType::ShipRepairT3);
    if (m_emergencyWeaponRepairCount > 0) {
        markItemDiscovery(InventoryItemType::EmergencyWeaponRepair);
    }

    int maxCount = Config::MAX_WEAPON_BACKPACK;
    std::vector<int> savedToRuntimeIndex(data.weapons.size(), -1);

    for (int i = 0; i < static_cast<int>(data.weapons.size()) && i < maxCount; ++i) {
        const WeaponLoadData& savedWeapon = data.weapons[i];

        const bool fieldsValid =
            savedWeapon.tier >= 1 && savedWeapon.tier <= 3 &&
            savedWeapon.damage >= 0 && savedWeapon.damage <= 100000 &&
            savedWeapon.maxDurability >= 1 && savedWeapon.maxDurability <= Config::INFINITE_WEAPON_DURABILITY &&
            savedWeapon.currentDurability >= 0 && savedWeapon.currentDurability <= savedWeapon.maxDurability &&
            savedWeapon.range >= 1 && savedWeapon.range <= 10000 &&
            savedWeapon.durabilityConsumption >= 0 && savedWeapon.durabilityConsumption <= 100000 &&
            savedWeapon.enhancementLevel >= 0 && savedWeapon.enhancementLevel <= 10000;
        if (!fieldsValid) continue;

        Weapon* weapon = ItemFactory::createWeapon(savedWeapon.typeCode, savedWeapon.tier);

        if (!weapon) {
            continue;
        }

        weapon->loadRuntimeState(
            savedWeapon.damage,
            savedWeapon.maxDurability,
            savedWeapon.currentDurability,
            savedWeapon.range,
            savedWeapon.durabilityConsumption,
            savedWeapon.enhancementLevel
        );

        savedToRuntimeIndex[i] = static_cast<int>(m_weapons.size());
        m_weapons.push_back(weapon);
        markEquipmentDiscoveryForWeapon(weapon);
    }

    if (m_weapons.empty()) {
        initDefaultWeaponIfNeeded();
        return;
    }

    m_quickWeaponSlots.fill(-1);
    std::vector<bool> usedWeaponIndices(m_weapons.size(), false);
    for (int slot = 0; slot < 6; ++slot) {
        const int savedIndex = data.quickWeaponSlots[slot];
        if (savedIndex < 0 || savedIndex >= static_cast<int>(savedToRuntimeIndex.size())) {
            continue;
        }
        const int weaponIndex = savedToRuntimeIndex[savedIndex];
        if (weaponIndex < 0) continue;
        if (usedWeaponIndices[weaponIndex]) {
            continue;
        }
        m_quickWeaponSlots[slot] = weaponIndex;
        usedWeaponIndices[weaponIndex] = true;
    }
    if (data.currentWeaponIndex >= 0 &&
        data.currentWeaponIndex < static_cast<int>(savedToRuntimeIndex.size()) &&
        savedToRuntimeIndex[data.currentWeaponIndex] >= 0) {
        m_currentWeaponIndex = savedToRuntimeIndex[data.currentWeaponIndex];
    }
    else {
        m_currentWeaponIndex = 0;
    }

    Player::instance().equipWeapon(m_weapons[m_currentWeaponIndex]);
}
