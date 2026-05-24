#include "InventorySystem.h"
#include "Player.h"
#include "ItemFactory.h"
#include <algorithm>

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
        m_weapons.push_back(defaultRod);
        m_currentWeaponIndex = 0;

        // 兼容现有 Player 接口
        Player::instance().equipWeapon(defaultRod);
    }
}

bool InventorySystem::canAddItem(int count) const
{
    return getTotalItemCount() + count <= Config::MAX_ITEM_BACKPACK;
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
        return true;

    case InventoryItemType::ShipRepairT1:
        m_shipRepairT1Count += count;
        return true;

    case InventoryItemType::ShipRepairT2:
        m_shipRepairT2Count += count;
        return true;

    case InventoryItemType::ShipRepairT3:
        m_shipRepairT3Count += count;
        return true;

    case InventoryItemType::EmergencyWeaponRepair:
        m_emergencyWeaponRepairCount += count;
        return true;
    }

    return false;
}

bool InventorySystem::useFood(Player& player)
{
    if (m_foodCount <= 0) {
        return false;
    }

    player.restoreStamina(Config::HEAL_FOOD_RATION);
    m_foodCount--;
    return true;
}

bool InventorySystem::useShipRepairKit(Player& player, int tier)
{
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

    if (m_currentWeaponIndex == index) {
        Player::instance().equipWeapon(weapon);
    }

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
    for (Weapon* weapon : m_weapons) {
        delete weapon;
    }

    m_weapons.clear();
    m_currentWeaponIndex = -1;

    m_foodCount = 0;
    m_shipRepairT1Count = 0;
    m_shipRepairT2Count = 0;
    m_shipRepairT3Count = 0;
    m_emergencyWeaponRepairCount = 0;
}