#include "ShopDialog.h"
#include <QMessageBox>

// ============================================================
// 辅助函数：创建商品按钮
// ============================================================
static QPushButton* makeItemButton(const QString& text, int price, QColor color)
{
    auto* btn = new QPushButton(QString("%1\n【%2 金币】").arg(text).arg(price));
    btn->setMinimumSize(180, 60);
    btn->setStyleSheet(QString(
        "QPushButton { background:%1; color:white; border-radius:8px; font-size:13px; padding:4px; }"
        "QPushButton:hover { background:%2; }"
        "QPushButton:disabled { background:#555; color:#888; }"
    ).arg(color.name()).arg(color.lighter(130).name()));
    return btn;
}

// ============================================================
// 构造函数
// ============================================================
ShopDialog::ShopDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("⚓ 补给商店");
    setFixedSize(900, 620);
    setStyleSheet("background:#1a2a3a; color:white;");
    setupUI();
}

ShopDialog::~ShopDialog() {}

// ============================================================
// 界面构建
// ============================================================
void ShopDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // 顶部标题和金币显示
    auto* topBar = new QHBoxLayout;
    auto* title = new QLabel("⚓ 补给商店 — 购买装备迎接下一关");
    title->setStyleSheet("font-size:18px; font-weight:bold; color:#FFD700;");
    m_coinsLabel = new QLabel();
    m_coinsLabel->setStyleSheet("font-size:16px; color:#FFD700;");
    updateCoinsLabel();
    topBar->addWidget(title);
    topBar->addStretch();
    topBar->addWidget(m_coinsLabel);
    mainLayout->addLayout(topBar);

    // 分隔线
    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color:#2E75B6;");
    mainLayout->addWidget(line);

    // 三栏内容
    auto* contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);
    contentLayout->addWidget(makeConsumableSection());
    contentLayout->addWidget(makeWeaponSection());
    contentLayout->addWidget(makeUpgradeSection());
    mainLayout->addLayout(contentLayout);

    // 底部关闭按钮
    auto* closeBtn = new QPushButton("✓ 出发！");
    closeBtn->setMinimumHeight(45);
    closeBtn->setStyleSheet(
        "QPushButton { background:#2E75B6; color:white; border-radius:8px; font-size:15px; font-weight:bold; }"
        "QPushButton:hover { background:#3A8FD6; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
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

    // 食物
    auto* foodBtn = makeItemButton("航海干粮\n恢复30体力", Config::PRICE_FOOD_RATION, QColor("#2d6a2d"));
    connect(foodBtn, &QPushButton::clicked, [this, foodBtn]() {
        Item* item = ItemFactory::createFood();
        buyItem(item);
        delete item;
        });
    layout->addWidget(foodBtn);

    // 修理包三级
    auto* repair1Btn = makeItemButton("初级修理包\n恢复20耐久", Config::PRICE_REPAIR_T1, QColor("#5a3a2d"));
    connect(repair1Btn, &QPushButton::clicked, [this]() {
        Item* item = ItemFactory::createRepairKit(1);
        buyItem(item);
        delete item;
        });
    layout->addWidget(repair1Btn);

    auto* repair2Btn = makeItemButton("中级修理包\n恢复40耐久", Config::PRICE_REPAIR_T2, QColor("#6a4a2d"));
    connect(repair2Btn, &QPushButton::clicked, [this]() {
        Item* item = ItemFactory::createRepairKit(2);
        buyItem(item);
        delete item;
        });
    layout->addWidget(repair2Btn);

    auto* repair3Btn = makeItemButton("高级修理包\n恢复100耐久", Config::PRICE_REPAIR_T3, QColor("#8a5a2d"));
    connect(repair3Btn, &QPushButton::clicked, [this]() {
        Item* item = ItemFactory::createRepairKit(3);
        buyItem(item);
        delete item;
        });
    layout->addWidget(repair3Btn);

    layout->addStretch();
    return section;
}

// ============================================================
// 武器区域
// ============================================================
QWidget* ShopDialog::makeWeaponSection()
{
    auto* section = new QWidget;
    section->setStyleSheet("background:#1e3a5a; border-radius:10px;");
    auto* layout = new QVBoxLayout(section);

    auto* header = new QLabel("⚔ 武器");
    header->setStyleSheet("font-size:15px; font-weight:bold; color:#FFB347; padding:4px;");
    layout->addWidget(header);

    // 武器列表：类型、等级、描述
    struct WeaponInfo { QString type; int tier; QString desc; QColor color; };
    QList<WeaponInfo> weapons = {
        {"Rod",     1, "基础鱼竿\n伤害5 耐久50",    QColor("#2d4a6a")},
        {"Harpoon", 1, "铁制鱼叉\n伤害30 耐久25",   QColor("#4a2d6a")},
        {"Harpoon", 2, "合金鱼叉\n伤害55 耐久30",   QColor("#5a3a7a")},
        {"Pistol",  1, "旧式手枪\n伤害50 耐久15",   QColor("#6a2d2d")},
        {"Shotgun", 1, "锈蚀猎枪\n伤害80 耐久10",   QColor("#7a3a2d")},
        {"Shotgun", 2, "双管猎枪\n伤害140 耐久12",  QColor("#8a4a2d")},
    };

    for (auto& info : weapons) {
        Weapon* w = ItemFactory::createWeapon(info.type.toStdString(), info.tier);
        if (!w) continue;
        int price = w->getValue();
        auto* btn = makeItemButton(info.desc, price, info.color);
        connect(btn, &QPushButton::clicked, [this, info]() {
            Weapon* weapon = ItemFactory::createWeapon(info.type.toStdString(), info.tier);
            buyWeapon(weapon);
            });
        layout->addWidget(btn);
        delete w;
    }

    layout->addStretch();
    return section;
}

// ============================================================
// 属性升级区域
// ============================================================
QWidget* ShopDialog::makeUpgradeSection()
{
    auto* section = new QWidget;
    section->setStyleSheet("background:#1e3a5a; border-radius:10px;");
    auto* layout = new QVBoxLayout(section);

    auto* header = new QLabel("⬆ 属性升级");
    header->setStyleSheet("font-size:15px; font-weight:bold; color:#87CEEB; padding:4px;");
    layout->addWidget(header);

    // 速度升级
    struct UpgradeInfo { QString attr; int tier; QString desc; QColor color; };
    QList<UpgradeInfo> upgrades = {
        {"Speed",      1, "初级润滑油\n船速+1.0",    QColor("#2d5a4a")},
        {"Speed",      2, "中级螺旋桨\n船速+2.0",    QColor("#2d6a5a")},
        {"Durability", 1, "木板补强\n耐久上限+20",   QColor("#4a3a2d")},
        {"Durability", 2, "钢板加固\n耐久上限+50",   QColor("#5a4a2d")},
        {"Stamina",    1, "初级耐力跑步\n体力上限+20", QColor("#2d3a5a")},
        {"Stamina",    2, "中级体能训练\n体力上限+50", QColor("#2d4a6a")},
        {"Weapon",     1, "初级磨刀石\n武器强化",     QColor("#5a2d5a")},
        {"Weapon",     2, "中级武器打磨\n武器强化+",  QColor("#6a3a6a")},
    };

    for (auto& info : upgrades) {
        Item* sample = (info.attr == "Weapon")
            ? ItemFactory::createWeaponUpgrade(info.tier)
            : ItemFactory::createAttributeUpgrade(info.attr.toStdString(), info.tier);
        if (!sample) continue;
        int price = sample->getValue();
        delete sample;

        auto* btn = makeItemButton(info.desc, price, info.color);
        connect(btn, &QPushButton::clicked, [this, info]() {
            Item* item = (info.attr == "Weapon")
                ? ItemFactory::createWeaponUpgrade(info.tier)
                : ItemFactory::createAttributeUpgrade(info.attr.toStdString(), info.tier);
            buyItem(item);
            delete item;
            });
        layout->addWidget(btn);
    }

    layout->addStretch();
    return section;
}

// ============================================================
// 购买逻辑
// ============================================================
void ShopDialog::buyItem(Item* item)
{
    if (!item) return;
    Player& p = Player::instance();
    int price = item->getValue();

    if (p.coins < price) {
        QMessageBox::warning(this, "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(p.coins));
        return;
    }

    p.coins -= price;
    item->use(p);
    updateCoinsLabel();
}

void ShopDialog::buyWeapon(Weapon* weapon)
{
    if (!weapon) return;
    Player& p = Player::instance();
    int price = weapon->getValue();

    if (p.coins < price) {
        QMessageBox::warning(this, "金币不足",
            QString("需要 %1 金币，当前只有 %2 金币。").arg(price).arg(p.coins));
        delete weapon;
        return;
    }

    p.coins -= price;
    p.equipWeapon(weapon); // 装备新武器
    updateCoinsLabel();
}

void ShopDialog::updateCoinsLabel()
{
    m_coinsLabel->setText(QString("💰 金币: %1").arg(Player::instance().coins));
}