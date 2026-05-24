#include "Shopdialog.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFrame>
#include <QColor>
#include <QList>
#include <QStringList>
#include <QLayoutItem>

// ============================================================
// 辅助函数：创建商品按钮
// ============================================================

static QPushButton* makeItemButton(const QString& text, int price, QColor color)
{
    auto* btn = new QPushButton(QString("%1\n【%2 金币】").arg(text).arg(price));
    btn->setMinimumSize(190, 64);
    btn->setStyleSheet(QString(
        "QPushButton { background:%1; color:white; border-radius:8px; font-size:13px; padding:4px; }"
        "QPushButton:hover { background:%2; }"
        "QPushButton:disabled { background:#555; color:#888; }"
    ).arg(color.name()).arg(color.lighter(130).name()));

    return btn;
}

static QPushButton* makeSmallButton(const QString& text, QColor color)
{
    auto* btn = new QPushButton(text);
    btn->setMinimumHeight(36);
    btn->setStyleSheet(QString(
        "QPushButton { background:%1; color:white; border-radius:6px; font-size:12px; padding:4px; }"
        "QPushButton:hover { background:%2; }"
    ).arg(color.name()).arg(color.lighter(130).name()));

    return btn;
}

// ============================================================
// 构造 / 析构
// ============================================================

ShopDialog::ShopDialog(QWidget* parent)
    : QDialog(parent)
{
    InventorySystem::instance().initDefaultWeaponIfNeeded();

    setWindowTitle("⚓ 补给商店");
    setFixedSize(1180, 720);
    setStyleSheet("background:#1a2a3a; color:white;");

    setupUI();
}

ShopDialog::~ShopDialog()
{
}

// ============================================================
// 主界面
// ============================================================

void ShopDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto* topBar = new QHBoxLayout;

    auto* title = new QLabel("⚓ 补给商店 — 购买补给、装备、升级与修复服务");
    title->setStyleSheet("font-size:18px; font-weight:bold; color:#FFD700;");

    m_coinsLabel = new QLabel();
    m_coinsLabel->setStyleSheet("font-size:16px; color:#FFD700;");

    topBar->addWidget(title);
    topBar->addStretch();
    topBar->addWidget(m_coinsLabel);

    mainLayout->addLayout(topBar);

    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color:#2E75B6;");
    mainLayout->addWidget(line);

    auto* contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    contentLayout->addWidget(makeConsumableSection());
    contentLayout->addWidget(makeWeaponSection());
    contentLayout->addWidget(makeUpgradeSection());
    contentLayout->addWidget(makeBackpackSection());

    mainLayout->addLayout(contentLayout);

    auto* closeBtn = new QPushButton("✓ 出发！");
    closeBtn->setMinimumHeight(45);
    closeBtn->setStyleSheet(
        "QPushButton { background:#2E75B6; color:white; border-radius:8px; font-size:15px; font-weight:bold; }"
        "QPushButton:hover { background:#3A8FD6; }"
    );

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);

    updateCoinsLabel();
    refreshBackpackUI();
}

// ============================================================
// 消耗品区域
// ============================================================

QWidget* ShopDialog::makeConsumableSection()
{
    auto* section = new QWidget;
    section->setStyleSheet("background:#1e3a5a; border-radius:10px;");

    auto* layout = new QVBoxLayout(section);

    auto* header = new QLabel("🍖 消耗品");
    header->setStyleSheet("font-size:15px; font-weight:bold; color:#90EE90; padding:4px;");
    layout->addWidget(header);

    auto* foodBtn = makeItemButton(
        "航海干粮\n放入背包：恢复30体力",
        Config::PRICE_FOOD_RATION,
        QColor("#2d6a2d")
    );
    connect(foodBtn, &QPushButton::clicked, [this]() {
        buyBackpackItem(
            InventoryItemType::Food,
            Config::PRICE_FOOD_RATION,
            "航海干粮"
        );
    });
    layout->addWidget(foodBtn);

    auto* repair1Btn = makeItemButton(
        "初级船体修理包\n放入背包：恢复20耐久",
        Config::PRICE_REPAIR_T1,
        QColor("#5a3a2d")
    );
    connect(repair1Btn, &QPushButton::clicked, [this]() {
        buyBackpackItem(
            InventoryItemType::ShipRepairT1,
            Config::PRICE_REPAIR_T1,
            "初级船体修理包"
        );
    });
    layout->addWidget(repair1Btn);

    auto* repair2Btn = makeItemButton(
        "中级船体修理包\n放入背包：恢复40耐久",
        Config::PRICE_REPAIR_T2,
        QColor("#6a4a2d")
    );
    connect(repair2Btn, &QPushButton::clicked, [this]() {
        buyBackpackItem(
            InventoryItemType::ShipRepairT2,
            Config::PRICE_REPAIR_T2,
            "中级船体修理包"
        );
    });
    layout->addWidget(repair2Btn);

    auto* repair3Btn = makeItemButton(
        "高级船体修理包\n放入背包：恢复100耐久",
        Config::PRICE_REPAIR_T3,
        QColor("#8a5a2d")
    );
    connect(repair3Btn, &QPushButton::clicked, [this]() {
        buyBackpackItem(
            InventoryItemType::ShipRepairT3,
            Config::PRICE_REPAIR_T3,
            "高级船体修理包"
        );
    });
    layout->addWidget(repair3Btn);

    auto* emergencyRepairBtn = makeItemButton(
        "紧急装备修理工具\n放入背包：修复25%耐久",
        Config::PRICE_EMERGENCY_WEAPON_REPAIR,
        QColor("#6a5a2d")
    );
    connect(emergencyRepairBtn, &QPushButton::clicked, [this]() {
        buyBackpackItem(
            InventoryItemType::EmergencyWeaponRepair,
            Config::PRICE_EMERGENCY_WEAPON_REPAIR,
            "紧急装备修理工具"
        );
    });
    layout->addWidget(emergencyRepairBtn);

    layout->addStretch();
    return section;
}

// ============================================================
// 装备区域
// ============================================================

QWidget* ShopDialog::makeWeaponSection()
{
    auto* section = new QWidget;
    section->setStyleSheet("background:#1e3a5a; border-radius:10px;");

    auto* layout = new QVBoxLayout(section);

    auto* header = new QLabel("⚔ 装备商店");
    header->setStyleSheet("font-size:15px; font-weight:bold; color:#FFB347; padding:4px;");
    layout->addWidget(header);

    struct WeaponInfo {
        QString type;
        int tier;
        QColor color;
    };

    QList<WeaponInfo> weapons = {
        {"Rod",     1, QColor("#2d4a6a")},
        {"Net",     1, QColor("#2d6a6a")},
        {"Harpoon", 1, QColor("#4a2d6a")},
        {"Harpoon", 2, QColor("#5a3a7a")},
        {"Pistol",  1, QColor("#6a2d2d")},
        {"Shotgun", 1, QColor("#7a3a2d")},
        {"Shotgun", 2, QColor("#8a4a2d")},
    };

    for (const auto& info : weapons) {
        Weapon* sample = ItemFactory::createWeapon(info.type.toStdString(), info.tier);
        if (!sample) {
            continue;
        }

        QString desc = QString("%1\n%2｜%3｜耐久%4")
            .arg(QString::fromStdString(sample->getName()))
            .arg(QString::fromStdString(sample->getRoleName()))
            .arg(QString::fromStdString(sample->getFishingModeName()))
            .arg(sample->getMaxDur());

        if (sample->canAttack()) {
            desc += QString("｜伤害%1").arg(sample->getDamage());
        }

        auto* btn = makeItemButton(desc, sample->getValue(), info.color);

        connect(btn, &QPushButton::clicked, [this, info]() {
            Weapon* weapon = ItemFactory::createWeapon(info.type.toStdString(), info.tier);
            buyWeapon(weapon);
        });

        layout->addWidget(btn);
        delete sample;
    }

    layout->addStretch();
    return section;
}

// ============================================================
// 升级 / 修复区域
// ============================================================

QWidget* ShopDialog::makeUpgradeSection()
{
    auto* section = new QWidget;
    section->setStyleSheet("background:#1e3a5a; border-radius:10px;");

    auto* layout = new QVBoxLayout(section);

    auto* header = new QLabel("⬆ 升级与修复");
    header->setStyleSheet("font-size:15px; font-weight:bold; color:#87CEEB; padding:4px;");
    layout->addWidget(header);

    struct UpgradeInfo {
        QString attr;
        int tier;
        QString desc;
        QColor color;
    };

    QList<UpgradeInfo> upgrades = {
        {"Speed",      1, "初级润滑油\n船速+1.0",       QColor("#2d5a4a")},
        {"Speed",      2, "中级螺旋桨\n船速+2.0",       QColor("#2d6a5a")},
        {"Durability", 1, "木板补强\n船体耐久上限+20", QColor("#4a3a2d")},
        {"Durability", 2, "钢板加固\n船体耐久上限+50", QColor("#5a4a2d")},
        {"Stamina",    1, "初级耐力训练\n体力上限+20", QColor("#2d3a5a")},
        {"Stamina",    2, "中级体能训练\n体力上限+50", QColor("#2d4a6a")},
    };

    for (const auto& info : upgrades) {
        Item* sample = ItemFactory::createAttributeUpgrade(info.attr.toStdString(), info.tier);
        if (!sample) {
            continue;
        }

        auto* btn = makeItemButton(info.desc, sample->getValue(), info.color);
        delete sample;

        connect(btn, &QPushButton::clicked, [this, info]() {
            Item* item = ItemFactory::createAttributeUpgrade(info.attr.toStdString(), info.tier);
            buyAndUseAttributeUpgrade(item);
            delete item;
        });

        layout->addWidget(btn);
    }

    auto* weaponUpgrade1 = makeItemButton(
        "初级装备强化\n选择一把装备强化",
        Config::PRICE_UPG_WEAPON_T1,
        QColor("#5a2d5a")
    );
    connect(weaponUpgrade1, &QPushButton::clicked, [this]() {
        buyWeaponUpgrade(1);
    });
    layout->addWidget(weaponUpgrade1);

    auto* weaponUpgrade2 = makeItemButton(
        "中级装备强化\n选择一把装备强化+",
        Config::PRICE_UPG_WEAPON_T2,
        QColor("#6a3a6a")
    );
    connect(weaponUpgrade2, &QPushButton::clicked, [this]() {
        buyWeaponUpgrade(2);
    });
    layout->addWidget(weaponUpgrade2);

    auto* shopRepair = makeItemButton(
        "商店装备修复服务\n选择装备恢复80%耐久",
        Config::PRICE_SHOP_WEAPON_REPAIR,
        QColor("#6a5a3a")
    );
    connect(shopRepair, &QPushButton::clicked, [this]() {
        buyShopWeaponRepair();
    });
    layout->addWidget(shopRepair);

    layout->addStretch();
    return section;
}

// ============================================================
// 背包区域
// ============================================================

QWidget* ShopDialog::makeBackpackSection()
{
    auto* section = new QWidget;
    section->setStyleSheet("background:#1e3a5a; border-radius:10px;");

    auto* layout = new QVBoxLayout(section);

    m_itemBagLabel = new QLabel();
    m_itemBagLabel->setStyleSheet("font-size:15px; font-weight:bold; color:#90EE90; padding:4px;");
    layout->addWidget(m_itemBagLabel);

    auto* itemBagWidget = new QWidget;
    m_itemBagLayout = new QVBoxLayout(itemBagWidget);
    m_itemBagLayout->setContentsMargins(0, 0, 0, 0);
    m_itemBagLayout->setSpacing(5);
    layout->addWidget(itemBagWidget);

    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color:#5a7a9a;");
    layout->addWidget(line);

    m_weaponBagLabel = new QLabel();
    m_weaponBagLabel->setStyleSheet("font-size:15px; font-weight:bold; color:#FFD700; padding:4px;");
    layout->addWidget(m_weaponBagLabel);

    auto* weaponBagWidget = new QWidget;
    m_weaponBagLayout = new QVBoxLayout(weaponBagWidget);
    m_weaponBagLayout->setContentsMargins(0, 0, 0, 0);
    m_weaponBagLayout->setSpacing(5);
    layout->addWidget(weaponBagWidget);

    layout->addStretch();
    return section;
}

// ============================================================
// 购买：背包物品
// ============================================================

void ShopDialog::buyBackpackItem(InventoryItemType type, int price, const QString& displayName)
{
    Player& player = Player::instance();

    if (player.coins < price) {
        QMessageBox::warning(
            this,
            "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(player.coins)
        );
        return;
    }

    if (!InventorySystem::instance().canAddItem()) {
        QMessageBox::warning(
            this,
            "物品背包已满",
            QString("物品背包容量已满，当前最大容量为 %1。").arg(Config::MAX_ITEM_BACKPACK)
        );
        return;
    }

    player.coins -= price;
    InventorySystem::instance().addItem(type, 1);

    updateCoinsLabel();
    refreshBackpackUI();

    QMessageBox::information(
        this,
        "购买成功",
        QString("已购买：%1\n物品已放入背包。").arg(displayName)
    );
}

// ============================================================
// 购买：装备
// ============================================================

void ShopDialog::buyWeapon(Weapon* weapon)
{
    if (!weapon) {
        return;
    }

    Player& player = Player::instance();
    int price = weapon->getValue();

    if (player.coins < price) {
        QMessageBox::warning(
            this,
            "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(player.coins)
        );
        delete weapon;
        return;
    }

    InventorySystem& inv = InventorySystem::instance();

    if (inv.canAddWeapon()) {
        player.coins -= price;
        inv.addWeapon(weapon);

        updateCoinsLabel();
        refreshBackpackUI();

        QMessageBox::information(
            this,
            "购买成功",
            QString("已购买：%1\n装备已放入背包。").arg(QString::fromStdString(weapon->getName()))
        );
        return;
    }

    int replaceIndex = askReplaceWeaponIndex();
    if (replaceIndex < 0) {
        delete weapon;
        return;
    }

    QString oldName = QString::fromStdString(inv.weapons()[replaceIndex]->getName());

    player.coins -= price;
    inv.replaceWeapon(replaceIndex, weapon);

    updateCoinsLabel();
    refreshBackpackUI();

    QMessageBox::information(
        this,
        "替换成功",
        QString("已用 %1 替换 %2。")
        .arg(QString::fromStdString(weapon->getName()))
        .arg(oldName)
    );
}

// ============================================================
// 购买并立即使用：基础属性升级
// ============================================================

void ShopDialog::buyAndUseAttributeUpgrade(Item* item)
{
    if (!item) {
        return;
    }

    Player& player = Player::instance();
    int price = item->getValue();

    if (player.coins < price) {
        QMessageBox::warning(
            this,
            "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(player.coins)
        );
        return;
    }

    player.coins -= price;
    item->use(player);

    updateCoinsLabel();
    refreshBackpackUI();

    QMessageBox::information(
        this,
        "升级成功",
        QString("已完成升级：%1").arg(QString::fromStdString(item->getName()))
    );
}

// ============================================================
// 购买：装备强化
// ============================================================

void ShopDialog::buyWeaponUpgrade(int tier)
{
    Player& player = Player::instance();

    int price = Config::PRICE_UPG_WEAPON_T1;
    int damageBoost = Config::VAL_UPG_WPN_DMG_T1;
    int durabilityBoost = Config::VAL_UPG_WPN_DUR_T1;

    if (tier == 2) {
        price = Config::PRICE_UPG_WEAPON_T2;
        damageBoost = Config::VAL_UPG_WPN_DMG_T2;
        durabilityBoost = Config::VAL_UPG_WPN_DUR_T2;
    }
    else if (tier == 3) {
        price = Config::PRICE_UPG_WEAPON_T3;
        damageBoost = Config::VAL_UPG_WPN_DMG_T3;
        durabilityBoost = Config::VAL_UPG_WPN_DUR_T3;
    }

    if (player.coins < price) {
        QMessageBox::warning(
            this,
            "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(player.coins)
        );
        return;
    }

    int index = askWeaponIndex("选择强化装备", "请选择要强化的装备：", true);
    if (index < 0) {
        return;
    }

    player.coins -= price;
    InventorySystem::instance().upgradeWeapon(index, damageBoost, durabilityBoost);

    updateCoinsLabel();
    refreshBackpackUI();

    QMessageBox::information(
        this,
        "强化成功",
        QString("装备强化完成：伤害 +%1，耐久上限 +%2。")
        .arg(damageBoost)
        .arg(durabilityBoost)
    );
}

// ============================================================
// 购买：商店装备修复服务
// ============================================================

void ShopDialog::buyShopWeaponRepair()
{
    Player& player = Player::instance();

    int price = Config::PRICE_SHOP_WEAPON_REPAIR;

    if (player.coins < price) {
        QMessageBox::warning(
            this,
            "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(player.coins)
        );
        return;
    }

    int index = askWeaponIndex("选择修复装备", "请选择要在商店修复的装备：", true);
    if (index < 0) {
        return;
    }

    Weapon* weapon = InventorySystem::instance().weapons()[index];
    if (!weapon || weapon->getCurrentDur() >= weapon->getMaxDur()) {
        QMessageBox::information(this, "无需修复", "该装备耐久已满，无需修复。");
        return;
    }

    player.coins -= price;
    InventorySystem::instance().repairWeaponByPercent(index, Config::SHOP_WEAPON_REPAIR_PERCENT);

    updateCoinsLabel();
    refreshBackpackUI();

    QMessageBox::information(
        this,
        "修复成功",
        QString("装备已恢复 %1% 最大耐久。").arg(Config::SHOP_WEAPON_REPAIR_PERCENT)
    );
}

// ============================================================
// 背包使用
// ============================================================

void ShopDialog::useFoodFromBackpack()
{
    if (!InventorySystem::instance().useFood(Player::instance())) {
        QMessageBox::information(this, "使用失败", "没有航海干粮。");
        return;
    }

    refreshBackpackUI();
}

void ShopDialog::useShipRepairFromBackpack(int tier)
{
    if (!InventorySystem::instance().useShipRepairKit(Player::instance(), tier)) {
        QMessageBox::information(this, "使用失败", "没有对应等级的船体修理包。");
        return;
    }

    refreshBackpackUI();
}

void ShopDialog::useEmergencyWeaponRepairFromBackpack()
{
    if (InventorySystem::instance().getItemCount(InventoryItemType::EmergencyWeaponRepair) <= 0) {
        QMessageBox::information(this, "使用失败", "没有紧急装备修理工具。");
        return;
    }

    int index = askWeaponIndex("紧急修理", "请选择要紧急修理的装备：", true);
    if (index < 0) {
        return;
    }

    if (!InventorySystem::instance().useEmergencyWeaponRepair(index)) {
        QMessageBox::information(this, "使用失败", "该装备可能已经满耐久，或无法修复。");
        return;
    }

    refreshBackpackUI();

    QMessageBox::information(
        this,
        "紧急修理完成",
        QString("已恢复所选装备 %1% 最大耐久。").arg(Config::EMERGENCY_WEAPON_REPAIR_PERCENT)
    );
}

// ============================================================
// 装备选择
// ============================================================

int ShopDialog::askWeaponIndex(const QString& title, const QString& label, bool allowBroken)
{
    const auto& weapons = InventorySystem::instance().weapons();

    if (weapons.empty()) {
        QMessageBox::information(this, title, "当前没有任何装备。");
        return -1;
    }

    QStringList options;
    std::vector<int> indexMap;

    for (int i = 0; i < static_cast<int>(weapons.size()); ++i) {
        const Weapon* weapon = weapons[i];
        if (!weapon) {
            continue;
        }

        if (!allowBroken && weapon->isBroken()) {
            continue;
        }

        options << weaponDisplayText(weapon, i);
        indexMap.push_back(i);
    }

    if (options.isEmpty()) {
        QMessageBox::information(this, title, "没有可选择的装备。");
        return -1;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(
        this,
        title,
        label,
        options,
        0,
        false,
        &ok
    );

    if (!ok || selected.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < options.size(); ++i) {
        if (options[i] == selected) {
            return indexMap[i];
        }
    }

    return -1;
}

int ShopDialog::askReplaceWeaponIndex()
{
    QMessageBox::information(
        this,
        "装备背包已满",
        QString("装备背包最多携带 %1 件装备。\n请选择一件旧装备进行替换。")
        .arg(Config::MAX_WEAPON_BACKPACK)
    );

    return askWeaponIndex("替换装备", "请选择要被替换的旧装备：", true);
}

void ShopDialog::selectWeaponFromBackpack(int index)
{
    if (!InventorySystem::instance().selectWeapon(index)) {
        QMessageBox::warning(this, "切换失败", "该装备已损坏或无法装备。");
        return;
    }

    refreshBackpackUI();

    Weapon* weapon = InventorySystem::instance().currentWeapon();

    if (weapon) {
        QMessageBox::information(
            this,
            "切换成功",
            QString("当前装备：%1").arg(QString::fromStdString(weapon->getName()))
        );
    }
}

// ============================================================
// UI 刷新
// ============================================================

void ShopDialog::updateCoinsLabel()
{
    if (!m_coinsLabel) {
        return;
    }

    m_coinsLabel->setText(
        QString("💰 金币: %1").arg(Player::instance().coins)
    );
}

void ShopDialog::refreshBackpackUI()
{
    InventorySystem& inv = InventorySystem::instance();

    if (m_itemBagLabel) {
        m_itemBagLabel->setText(
            QString("🎒 物品背包 %1/%2")
            .arg(inv.getTotalItemCount())
            .arg(Config::MAX_ITEM_BACKPACK)
        );
    }

    if (m_itemBagLayout) {
        while (QLayoutItem* item = m_itemBagLayout->takeAt(0)) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }

        QLabel* itemInfo = new QLabel(
            QString("航海干粮：%1\n初级修理包：%2\n中级修理包：%3\n高级修理包：%4\n紧急装备修理工具：%5")
            .arg(inv.getItemCount(InventoryItemType::Food))
            .arg(inv.getItemCount(InventoryItemType::ShipRepairT1))
            .arg(inv.getItemCount(InventoryItemType::ShipRepairT2))
            .arg(inv.getItemCount(InventoryItemType::ShipRepairT3))
            .arg(inv.getItemCount(InventoryItemType::EmergencyWeaponRepair))
        );
        itemInfo->setStyleSheet("color:white; font-size:12px; padding:4px;");
        m_itemBagLayout->addWidget(itemInfo);

        auto* useFoodBtn = makeSmallButton("使用干粮", QColor("#2d6a2d"));
        connect(useFoodBtn, &QPushButton::clicked, [this]() {
            useFoodFromBackpack();
        });
        m_itemBagLayout->addWidget(useFoodBtn);

        auto* useRepair1Btn = makeSmallButton("使用初级修理包", QColor("#5a3a2d"));
        connect(useRepair1Btn, &QPushButton::clicked, [this]() {
            useShipRepairFromBackpack(1);
        });
        m_itemBagLayout->addWidget(useRepair1Btn);

        auto* useRepair2Btn = makeSmallButton("使用中级修理包", QColor("#6a4a2d"));
        connect(useRepair2Btn, &QPushButton::clicked, [this]() {
            useShipRepairFromBackpack(2);
        });
        m_itemBagLayout->addWidget(useRepair2Btn);

        auto* useRepair3Btn = makeSmallButton("使用高级修理包", QColor("#8a5a2d"));
        connect(useRepair3Btn, &QPushButton::clicked, [this]() {
            useShipRepairFromBackpack(3);
        });
        m_itemBagLayout->addWidget(useRepair3Btn);

        auto* useEmergencyBtn = makeSmallButton("使用紧急装备修理工具", QColor("#6a5a2d"));
        connect(useEmergencyBtn, &QPushButton::clicked, [this]() {
            useEmergencyWeaponRepairFromBackpack();
        });
        m_itemBagLayout->addWidget(useEmergencyBtn);
    }

    if (m_weaponBagLabel) {
        m_weaponBagLabel->setText(
            QString("⚔ 装备背包 %1/%2")
            .arg(inv.weaponCount())
            .arg(inv.maxWeaponCapacity())
        );
    }

    if (m_weaponBagLayout) {
        while (QLayoutItem* item = m_weaponBagLayout->takeAt(0)) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }

        const auto& weapons = inv.weapons();

        if (weapons.empty()) {
            QLabel* emptyLabel = new QLabel("暂无装备");
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet("color:#BBBBBB; font-size:12px; padding:8px;");
            m_weaponBagLayout->addWidget(emptyLabel);
        }
        else {
            for (int i = 0; i < static_cast<int>(weapons.size()); ++i) {
                Weapon* weapon = weapons[i];
                if (!weapon) {
                    continue;
                }

                QPushButton* btn = new QPushButton(weaponDisplayText(weapon, i));
                btn->setMinimumHeight(70);

                if (i == inv.currentWeaponIndex()) {
                    btn->setStyleSheet(
                        "QPushButton { background:#D6A11E; color:white; border-radius:8px; font-size:12px; padding:4px; }"
                        "QPushButton:hover { background:#E6B13E; }"
                    );
                }
                else if (weapon->isBroken()) {
                    btn->setStyleSheet(
                        "QPushButton { background:#555555; color:#DDDDDD; border-radius:8px; font-size:12px; padding:4px; }"
                        "QPushButton:hover { background:#666666; }"
                    );
                }
                else {
                    btn->setStyleSheet(
                        "QPushButton { background:#34506a; color:white; border-radius:8px; font-size:12px; padding:4px; }"
                        "QPushButton:hover { background:#456a8a; }"
                    );
                }

                connect(btn, &QPushButton::clicked, [this, i]() {
                    selectWeaponFromBackpack(i);
                });

                m_weaponBagLayout->addWidget(btn);
            }
        }
    }
}

QString ShopDialog::weaponDisplayText(const Weapon* weapon, int index) const
{
    if (!weapon) {
        return "空装备";
    }

    QString status;

    if (weapon->isBroken()) {
        status = "【已损坏】";
    }
    else if (index == InventorySystem::instance().currentWeaponIndex()) {
        status = "【当前】";
    }
    else {
        status = "【背包】";
    }

    QString text = QString("%1%2\n类型:%3｜捕鱼:%4\n伤害:%5｜范围:%6｜耐久:%7/%8")
        .arg(status)
        .arg(QString::fromStdString(weapon->getName()))
        .arg(QString::fromStdString(weapon->getRoleName()))
        .arg(QString::fromStdString(weapon->getFishingModeName()))
        .arg(weapon->getDamage())
        .arg(weapon->getRange())
        .arg(weapon->getCurrentDur())
        .arg(weapon->getMaxDur());

    return text;
}