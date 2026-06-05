#include "Shopdialog.h"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QStringList>
#include <algorithm>

#include "GameUiDialog.h"
#include "weathersystem.h"

namespace {
constexpr QRect kTopStatusRect(156, 14, 968, 62);
constexpr QRect kMainRect(136, 92, 1008, 588);
constexpr QRect kNavRect(166, 170, 190, 390);
constexpr QRect kContentRect(380, 170, 500, 430);
constexpr QRect kRightRect(900, 170, 216, 430);
constexpr QRect kTipRect(0, 0, 0, 0);
constexpr int kCardPageSize = 4;
constexpr int kShipUpgradeRowHeight = 58;
constexpr int kShipUpgradeRowGap = 4;

QString firstAvailableFont(const QStringList& candidates)
{
    const QStringList families = QFontDatabase::families();
    for (const QString& candidate : candidates) {
        for (const QString& family : families) {
            if (family.compare(candidate, Qt::CaseInsensitive) == 0) {
                return family;
            }
        }
    }
    return QStringLiteral("Microsoft YaHei UI");
}

QFont uiFont(int size, int weight = QFont::Normal)
{
    static const QString family = firstAvailableFont({
        QStringLiteral("YouYuan"),
        QStringLiteral("幼圆"),
        QStringLiteral("FZYaoTi"),
        QStringLiteral("方正姚体"),
        QStringLiteral("STKaiti"),
        QStringLiteral("华文楷体"),
        QStringLiteral("STXihei"),
        QStringLiteral("华文细黑"),
        QStringLiteral("Microsoft YaHei UI")
    });
    QFont font(family, size, weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    return font;
}

QFont shopTitleFont(int size, int weight = QFont::Bold)
{
    static const QString family = firstAvailableFont({
        QStringLiteral("STXinwei"),
        QStringLiteral("华文新魏"),
        QStringLiteral("FZShuTi"),
        QStringLiteral("方正舒体"),
        QStringLiteral("STXingkai"),
        QStringLiteral("华文行楷"),
        QStringLiteral("STKaiti"),
        QStringLiteral("华文楷体"),
        QStringLiteral("YouYuan"),
        QStringLiteral("幼圆"),
        QStringLiteral("Microsoft YaHei UI")
    });
    QFont font(family, size, weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    return font;
}

QString weatherName()
{
    switch (WeatherSystem::instance().currentWeather()) {
    case WeatherType::SUNNY: return QStringLiteral("晴朗");
    case WeatherType::FOG: return QStringLiteral("雾天");
    case WeatherType::STORM: return QStringLiteral("风暴");
    }
    return QStringLiteral("晴朗");
}

QString moneyText(int value)
{
    return QString::number(value);
}
}

ShopDialog::ShopDialog(QWidget* parent)
    : QDialog(parent)
{
    InventorySystem::instance().initDefaultWeaponIfNeeded();
    setWindowTitle(QStringLiteral("码头补给站"));
    setFixedSize(1280, 720);
    setMouseTracking(true);
    loadAssets();
    buildCatalogs();
}

void ShopDialog::loadAssets()
{
    m_seaBackground.load(":/FishingVoyage/ui/shop/background.png");
    if (m_seaBackground.isNull()) {
        m_seaBackground.load(":/FishingVoyage/backgrounds/sea.png");
    }
    m_topStatusBar.load(":/FishingVoyage/ui/shop/top_status_bar.png");
    m_windowPanel.load(":/FishingVoyage/ui/shop/window_panel.png");
    m_titlePlaque.load(":/FishingVoyage/ui/shop/title_plaque.png");
    m_contentPanel.load(":/FishingVoyage/ui/shop/content_panel.png");
    m_sidePanel.load(":/FishingVoyage/ui/shop/side_panel.png");
    m_infoParchment.load(":/FishingVoyage/ui/shop/info_parchment.png");
    m_navNormal.load(":/FishingVoyage/ui/shop/nav_normal.png");
    m_navHover.load(":/FishingVoyage/ui/shop/nav_hover.png");
    m_navSelected.load(":/FishingVoyage/ui/shop/nav_selected.png");
    m_navDisabled.load(":/FishingVoyage/ui/shop/nav_disabled.png");
    m_cardNormal.load(":/FishingVoyage/ui/shop/card_normal.png");
    m_cardHover.load(":/FishingVoyage/ui/shop/card_hover.png");
    m_cardSelected.load(":/FishingVoyage/ui/shop/card_selected.png");
    m_cardDisabled.load(":/FishingVoyage/ui/shop/card_disabled.png");
    m_rowNormal.load(":/FishingVoyage/ui/shop/row_normal.png");
    m_rowHover.load(":/FishingVoyage/ui/shop/row_hover.png");
    m_rowSelected.load(":/FishingVoyage/ui/shop/row_selected.png");
    m_rowDisabled.load(":/FishingVoyage/ui/shop/row_disabled.png");
    m_buttonGreen.load(":/FishingVoyage/ui/shop/button_green.png");
    m_buttonBlue.load(":/FishingVoyage/ui/shop/button_blue.png");
    m_buttonRed.load(":/FishingVoyage/ui/shop/button_red.png");
    m_buttonGold.load(":/FishingVoyage/ui/shop/button_gold.png");
    m_buttonGray.load(":/FishingVoyage/ui/shop/button_gray.png");
    m_priceBadge.load(":/FishingVoyage/ui/shop/price_badge.png");
    m_coinIcon.load(":/FishingVoyage/ui/shop/coin_icon.png");
    m_shopkeeper.load(":/FishingVoyage/ui/shop/shopkeeper_portrait.png");
    m_noticeBoard.load(":/FishingVoyage/ui/common/wood_notice_board.png");
    m_noticeButton.load(":/FishingVoyage/ui/common/wood_notice_button.png");
    m_noticeIcon.load(":/FishingVoyage/ui/common/notice_icon_info.png");

    m_iconRod.load(":/FishingVoyage/ui/icons/weapon_rod.png");
    m_iconNet.load(":/FishingVoyage/ui/icons/weapon_net.png");
    m_iconHarpoon.load(":/FishingVoyage/ui/icons/weapon_harpoon.png");
    m_iconPistol.load(":/FishingVoyage/ui/icons/weapon_pistol.png");
    m_iconShotgun.load(":/FishingVoyage/ui/icons/weapon_shotgun.png");
    m_iconFood.load(":/FishingVoyage/ui/icons/item_food.png");
    m_iconRepair1.load(":/FishingVoyage/ui/icons/item_repair_t1.png");
    m_iconRepair2.load(":/FishingVoyage/ui/icons/item_repair_t2.png");
    m_iconRepair3.load(":/FishingVoyage/ui/icons/item_repair_t3.png");
    m_iconEmergencyRepair.load(":/FishingVoyage/ui/icons/item_emergency_repair.png");
    m_navIconEquipment.load(":/FishingVoyage/ui/shop/nav_icon_equipment.png");
    m_navIconSupply.load(":/FishingVoyage/ui/shop/nav_icon_supply.png");
    m_navIconUpgrade.load(":/FishingVoyage/ui/shop/nav_icon_upgrade.png");
    m_navIconShip.load(":/FishingVoyage/ui/shop/nav_icon_ship.png");
    m_navIconBackpack.load(":/FishingVoyage/ui/shop/nav_icon_backpack.png");
    m_navIconExit.load(":/FishingVoyage/ui/shop/nav_icon_exit.png");
}

void ShopDialog::buildCatalogs()
{
    m_weaponOffers = {
        {"Rod", 1, QStringLiteral("铁制鱼竿"), QStringLiteral("稳固耐用的基础钓具"),
         QStringLiteral("用于普通鱼类的 QTE 捕捞。"), QColor("#2f83b6")},
        {"Rod", 2, QStringLiteral("进阶鱼竿"), QStringLiteral("更可靠的中级钓具"),
         QStringLiteral("QTE 容错更高，适合远海鱼类。"), QColor("#2f83b6")},
        {"Rod", 3, QStringLiteral("传世鱼竿"), QStringLiteral("顶级传统钓具"),
         QStringLiteral("高耐久的 QTE 捕鱼工具。"), QColor("#2f83b6")},
        {"Net", 1, QStringLiteral("普通渔网"), QStringLiteral("适合捕捞小型鱼群"),
         QStringLiteral("校准捕鱼，范围更宽。"), QColor("#5e9b35")},
        {"Net", 2, QStringLiteral("加固渔网"), QStringLiteral("稳定的中级渔网"),
         QStringLiteral("校准捕鱼，耐久更充足。"), QColor("#5e9b35")},
        {"Net", 3, QStringLiteral("捕捉大师渔网"), QStringLiteral("高阶捕捞工具"),
         QStringLiteral("校准捕鱼，适合高价值鱼。"), QColor("#5e9b35")},
        {"Harpoon", 1, QStringLiteral("铁制鱼叉"), QStringLiteral("近距捕鱼与攻击兼备"),
         QStringLiteral("能捕鱼，也能应对海中威胁。"), QColor("#7b4aa5")},
        {"Harpoon", 2, QStringLiteral("合金鱼叉"), QStringLiteral("强化双用工具"),
         QStringLiteral("更高伤害，更适合深海航段。"), QColor("#7b4aa5")},
        {"Harpoon", 3, QStringLiteral("海王鱼叉"), QStringLiteral("顶级双用装备"),
         QStringLiteral("校准捕鱼，也可重创敌人。"), QColor("#7b4aa5")},
        {"Pistol", 1, QStringLiteral("手枪"), QStringLiteral("远距离自卫武器"),
         QStringLiteral("无法捕鱼，发射单颗子弹。"), QColor("#b67921")},
        {"Pistol", 2, QStringLiteral("改良手枪"), QStringLiteral("更高稳定性的枪械"),
         QStringLiteral("中远距离伤害装备。"), QColor("#b67921")},
        {"Pistol", 3, QStringLiteral("执法者手枪"), QStringLiteral("高级远程武器"),
         QStringLiteral("高伤害单发弹道。"), QColor("#b67921")},
        {"Shotgun", 1, QStringLiteral("猎枪"), QStringLiteral("高伤害近距武器"),
         QStringLiteral("无法捕鱼，发射扇形弹丸。"), QColor("#a74a24")},
        {"Shotgun", 2, QStringLiteral("双管猎枪"), QStringLiteral("升级近距火力"),
         QStringLiteral("高风险海域的强力选择。"), QColor("#a74a24")},
        {"Shotgun", 3, QStringLiteral("破灭者猎枪"), QStringLiteral("顶级近距爆发"),
         QStringLiteral("多弹丸压制大型威胁。"), QColor("#a74a24")}
    };

    m_supplyOffers = {
        {InventoryItemType::Food, 0, Config::PRICE_FOOD_RATION,
         QStringLiteral("航海干粮"), QStringLiteral("恢复 30 体力"),
         QStringLiteral("压缩干粮与淡水，航行中可随时使用。"), &m_iconFood, QColor("#5e9b35")},
        {InventoryItemType::ShipRepairT1, 1, Config::PRICE_REPAIR_T1,
         QStringLiteral("初级船体修理包"), QStringLiteral("恢复 20 耐久"),
         QStringLiteral("基础船板与补漏材料，适合小损伤。"), &m_iconRepair1, QColor("#2f83b6")},
        {InventoryItemType::ShipRepairT2, 2, Config::PRICE_REPAIR_T2,
         QStringLiteral("中级船体修理包"), QStringLiteral("恢复 40 耐久"),
         QStringLiteral("优质船体材料，能修复较严重损伤。"), &m_iconRepair2, QColor("#7b4aa5")},
        {InventoryItemType::ShipRepairT3, 3, Config::PRICE_REPAIR_T3,
         QStringLiteral("高级船体修理包"), QStringLiteral("恢复 100 耐久"),
         QStringLiteral("专业级修理材料，关键时刻救船。"), &m_iconRepair3, QColor("#b67921")},
        {InventoryItemType::EmergencyWeaponRepair, 0, Config::PRICE_EMERGENCY_WEAPON_REPAIR,
         QStringLiteral("紧急装备修理工具"), QStringLiteral("恢复装备 25% 耐久"),
         QStringLiteral("小型工具组，可修复当前背包装备。"), &m_iconEmergencyRepair, QColor("#a74a24")}
    };

    m_shipUpgrades = {
        {"Speed", 1, Config::PRICE_UPG_SPEED_T1,
         QStringLiteral("船速升级 I"), QStringLiteral("船只航行速度 +12"),
         QStringLiteral("打磨船底并调整帆索，让船更轻快。"), QStringLiteral("速度 +12")},
        {"Speed", 2, Config::PRICE_UPG_SPEED_T2,
         QStringLiteral("船速升级 II"), QStringLiteral("船只航行速度 +22"),
         QStringLiteral("更换螺旋桨与传动结构。"), QStringLiteral("速度 +22")},
        {"Speed", 3, Config::PRICE_UPG_SPEED_T3,
         QStringLiteral("船速升级 III"), QStringLiteral("船只航行速度 +38"),
         QStringLiteral("装配高效引擎，显著提升巡航能力。"), QStringLiteral("速度 +38")},
        {"Durability", 1, Config::PRICE_UPG_DUR_T1,
         QStringLiteral("船体耐久上限 I"), QStringLiteral("最大耐久 +20"),
         QStringLiteral("增加防撞木板，降低意外损伤风险。"), QStringLiteral("耐久上限 +20")},
        {"Durability", 2, Config::PRICE_UPG_DUR_T2,
         QStringLiteral("船体耐久上限 II"), QStringLiteral("最大耐久 +50"),
         QStringLiteral("钢板加固船身，适合远海探索。"), QStringLiteral("耐久上限 +50")},
        {"Durability", 3, Config::PRICE_UPG_DUR_T3,
         QStringLiteral("船体耐久上限 III"), QStringLiteral("最大耐久 +100"),
         QStringLiteral("钛合金装甲，面对深海冲击更从容。"), QStringLiteral("耐久上限 +100")},
        {"Stamina", 1, Config::PRICE_UPG_STAMINA_T1,
         QStringLiteral("体力上限 I"), QStringLiteral("最大体力 +20"),
         QStringLiteral("基础体能训练，提升连续作业能力。"), QStringLiteral("体力上限 +20")},
        {"Stamina", 2, Config::PRICE_UPG_STAMINA_T2,
         QStringLiteral("体力上限 II"), QStringLiteral("最大体力 +50"),
         QStringLiteral("专业船员训练，长航程更稳定。"), QStringLiteral("体力上限 +50")},
        {"Stamina", 3, Config::PRICE_UPG_STAMINA_T3,
         QStringLiteral("体力上限 III"), QStringLiteral("最大体力 +100"),
         QStringLiteral("高强度特训，支撑长时间冲刺作业。"), QStringLiteral("体力上限 +100")}
    };
}

void ShopDialog::paintEvent(QPaintEvent*)
{
    m_zones.clear();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    drawBackground(p);
    drawTopStatus(p);
    drawMainFrame(p);
    drawTitle(p);
    drawNav(p);
    drawCurrentPage(p);
    drawRightPanel(p);
    drawNotice(p);
}

void ShopDialog::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->pos();
    setCursor(zoneAt(m_mousePos) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

void ShopDialog::leaveEvent(QEvent*)
{
    m_mousePos = QPoint(-1, -1);
    setCursor(Qt::ArrowCursor);
    update();
}

void ShopDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    if (hasNotice()) {
        clearNotice();
        return;
    }
    if (const ClickZone* zone = zoneAt(event->pos())) {
        handleZone(*zone);
    }
}

void ShopDialog::wheelEvent(QWheelEvent* event)
{
    if (hasNotice()) {
        QDialog::wheelEvent(event);
        return;
    }

    if (m_category == Category::ShipUpgrade) {
        const int pixelDelta = event->pixelDelta().y();
        const int angleDelta = event->angleDelta().y();
        const int scrollDelta = pixelDelta != 0 ? -pixelDelta : -angleDelta / 2;
        if (scrollDelta != 0 && maxShipUpgradeScroll() > 0) {
            scrollShipUpgradeList(scrollDelta);
            event->accept();
            return;
        }
    }

    QDialog::wheelEvent(event);
}

void ShopDialog::keyPressEvent(QKeyEvent* event)
{
    if (hasNotice()) {
        clearNotice();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (m_category == Category::Backpack) {
            setCategory(Category::Equipment);
        } else {
            accept();
        }
        return;
    }
    if (event->key() == Qt::Key_B || event->key() == Qt::Key_P) {
        setCategory(m_category == Category::Backpack ? Category::Equipment : Category::Backpack);
        return;
    }
    QDialog::keyPressEvent(event);
}

void ShopDialog::drawBackground(QPainter& p)
{
    if (!m_seaBackground.isNull()) {
        p.drawPixmap(rect(), m_seaBackground);
    } else {
        p.fillRect(rect(), QColor(18, 92, 138));
    }
    p.fillRect(rect(), QColor(0, 18, 32, 96));
}

void ShopDialog::drawTopStatus(QPainter& p)
{
    p.save();
    p.setPen(QPen(QColor(176, 112, 32), 3));
    p.setBrush(QColor(35, 22, 14, 232));
    p.drawRoundedRect(kTopStatusRect, 8, 8);
    p.setPen(QPen(QColor(235, 184, 72), 1));
    p.drawRoundedRect(kTopStatusRect.adjusted(6, 6, -6, -6), 5, 5);

    drawTextShadow(p, QRect(kTopStatusRect.left() + 18, kTopStatusRect.top() + 8, 120, 24),
                   QStringLiteral("渔途"), shopTitleFont(20, QFont::Bold), QColor(255, 232, 166),
                   Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(kTopStatusRect.left() + 18, kTopStatusRect.top() + 34, 120, 18),
                   QStringLiteral("码头补给"), uiFont(10, QFont::Bold), QColor(211, 164, 88),
                   Qt::AlignLeft | Qt::AlignVCenter);

    Player& pl = Player::instance();
    struct StatusCell { QString label; QString value; QColor color; int current; int max; };
    QVector<StatusCell> cells = {
        {QStringLiteral("耐久"), QString("%1/%2").arg(pl.durability()).arg(pl.maxDurability), QColor(220, 66, 58), pl.durability(), pl.maxDurability},
        {QStringLiteral("体力"), QString("%1/%2").arg(pl.stamina()).arg(pl.maxStamina), QColor(52, 124, 232), pl.stamina(), pl.maxStamina},
        {QStringLiteral("金币"), QString::number(pl.coins), QColor(235, 173, 42), 1, 1},
        {QStringLiteral("鱼获"), QString::number(pl.fishCaught), QColor(90, 178, 220), pl.fishCaught, qMax(1, pl.fishCaught)},
        {QStringLiteral("天气"), weatherName(), QColor(255, 211, 64), 1, 1}
    };

    int x = kTopStatusRect.left() + 154;
    for (const auto& cell : cells) {
        QRect cellRect(x, kTopStatusRect.top() + 10, 150, 42);
        p.setPen(QPen(QColor(92, 58, 25), 1));
        p.setBrush(QColor(16, 13, 10, 210));
        p.drawRoundedRect(cellRect, 5, 5);
        drawTextShadow(p, QRect(cellRect.left() + 10, cellRect.top() + 4, 52, 18), cell.label,
                       uiFont(10, QFont::Bold), QColor(228, 183, 112), Qt::AlignLeft | Qt::AlignVCenter);
        drawTextShadow(p, QRect(cellRect.left() + 58, cellRect.top() + 4, 82, 18), cell.value,
                       uiFont(10, QFont::Bold), QColor(255, 245, 220), Qt::AlignRight | Qt::AlignVCenter);
        p.setPen(Qt::NoPen);
        QRect bar(cellRect.left() + 10, cellRect.bottom() - 12, cellRect.width() - 20, 7);
        p.setBrush(QColor(32, 24, 18));
        p.drawRoundedRect(bar, 3, 3);
        const int fillWidth = cell.max > 0 ? qBound(0, cell.current * bar.width() / cell.max, bar.width()) : bar.width();
        p.setBrush(cell.color);
        p.drawRoundedRect(QRect(bar.left(), bar.top(), fillWidth, bar.height()), 3, 3);
        x += 158;
    }
    p.restore();
}

void ShopDialog::drawMainFrame(QPainter& p)
{
    if (!m_windowPanel.isNull()) {
        p.drawPixmap(kMainRect, m_windowPanel);
    } else {
        p.fillRect(kMainRect, QColor(70, 38, 16));
    }
}

void ShopDialog::drawTitle(QPainter& p)
{
    QRect plaque(400, 88, 480, 84);
    p.drawPixmap(plaque, m_titlePlaque);
    drawTextShadow(p, QRect(plaque.left() + 28, plaque.top() + 18, plaque.width() - 56, 36),
                   QStringLiteral("码头补给站"), shopTitleFont(27, QFont::Bold),
                   QColor(255, 224, 138));
    drawTextShadow(p, QRect(plaque.left() + 30, plaque.top() + 52, plaque.width() - 60, 22),
                   QStringLiteral("PORT SUPPLY SHOP"), uiFont(11, QFont::Bold),
                   QColor(255, 219, 134));
}

void ShopDialog::drawNav(QPainter& p)
{
    struct NavItem { Category category; QString text; const QPixmap* icon; };
    QVector<NavItem> items = {
        {Category::Equipment, QStringLiteral("装备商店"), &m_navIconEquipment},
        {Category::Supplies, QStringLiteral("道具补给"), &m_navIconSupply},
        {Category::WeaponUpgrade, QStringLiteral("装备强化"), &m_navIconUpgrade},
        {Category::ShipUpgrade, QStringLiteral("船体升级"), &m_navIconShip},
        {Category::Backpack, QStringLiteral("船舱背包"), &m_navIconBackpack},
        {Category::Close, QStringLiteral("离开商店"), &m_navIconExit}
    };

    int y = kNavRect.top();
    for (int i = 0; i < items.size(); ++i) {
        QRect r(kNavRect.left(), y, kNavRect.width(), 60);
        bool selected = (m_category == items[i].category);
        const QPixmap& bg = selected ? m_navSelected : (isHovered(r) ? m_navHover : m_navNormal);
        p.drawPixmap(r, bg);
        if (items[i].icon) {
            drawPixmapFit(p, *items[i].icon, QRect(r.left() + 13, r.top() + 9, 36, 42));
        }
        drawTextShadow(p, QRect(r.left() + 56, r.top() + 8, r.width() - 68, 42), items[i].text,
                       shopTitleFont(15, QFont::Bold), selected ? QColor(255, 244, 185) : QColor(228, 205, 150),
                       Qt::AlignLeft | Qt::AlignVCenter);
        addZone(r, items[i].category == Category::Close ? ZoneAction::Close : ZoneAction::SelectCategory, static_cast<int>(items[i].category));
        y += 68;
    }
}

void ShopDialog::drawCurrentPage(QPainter& p)
{
    p.drawPixmap(kContentRect, m_contentPanel);
    switch (m_category) {
    case Category::Equipment: drawEquipmentPage(p); break;
    case Category::Supplies: drawSuppliesPage(p); break;
    case Category::WeaponUpgrade: drawWeaponUpgradePage(p); break;
    case Category::ShipUpgrade: drawShipUpgradePage(p); break;
    case Category::Backpack: drawBackpackPage(p); break;
    case Category::Close: break;
    }
}

void ShopDialog::drawEquipmentPage(QPainter& p)
{
    drawTextShadow(p, QRect(kContentRect.left() + 24, kContentRect.top() + 72, kContentRect.width() - 48, 28),
                   QStringLiteral("选择合适的装备，购买后会放入装备背包。"), uiFont(13, QFont::Bold),
                   QColor(73, 43, 16));

    const int pageCount = visiblePageCount(m_weaponOffers.size(), kCardPageSize);
    m_page = std::clamp(m_page, 0, pageCount - 1);
    const int start = m_page * kCardPageSize;
    const int cardW = 114;
    const int cardH = 268;
    const int gap = 6;
    const int x0 = kContentRect.left() + 18;
    const int y0 = kContentRect.top() + 108;

    for (int i = 0; i < kCardPageSize; ++i) {
        int idx = start + i;
        if (idx >= m_weaponOffers.size()) break;
        const auto& offer = m_weaponOffers[idx];
        Weapon* sample = ItemFactory::createWeapon(offer.type.toStdString(), offer.tier);
        if (!sample) continue;

        QString subtitle = QStringLiteral("耐久 %1\n范围 %2").arg(sample->getMaxDur()).arg(sample->getRange());
        if (sample->canAttack()) subtitle += QStringLiteral("  伤害 %1").arg(sample->getDamage());
        drawCard(p, QRect(x0 + i * (cardW + gap), y0, cardW, cardH),
                 offer.title, subtitle, offer.description, weaponIcon(offer.type),
                 sample->getValue(), offer.tagColor, Player::instance().coins < sample->getValue(),
                 ZoneAction::BuyWeapon, idx);
        delete sample;
    }

    QRect prev(kContentRect.left() + 142, kContentRect.bottom() - 46, 86, 34);
    QRect next(kContentRect.left() + 274, kContentRect.bottom() - 46, 86, 34);
    drawActionButton(p, prev, QStringLiteral("上一页"), m_buttonGray, ZoneAction::PrevPage, -1, 0, m_page <= 0);
    drawActionButton(p, next, QStringLiteral("下一页"), m_buttonGray, ZoneAction::NextPage, -1, 0, m_page >= pageCount - 1);
    drawTextShadow(p, QRect(kContentRect.left() + 228, kContentRect.bottom() - 42, 46, 26),
                   QString("%1/%2").arg(m_page + 1).arg(pageCount), uiFont(10, QFont::Bold), QColor(91, 54, 18));
}

void ShopDialog::drawSuppliesPage(QPainter& p)
{
    drawTextShadow(p, QRect(kContentRect.left() + 24, kContentRect.top() + 72, kContentRect.width() - 48, 28),
                   QStringLiteral("补给会进入物品背包，可在背包页使用。"), uiFont(13, QFont::Bold),
                   QColor(73, 43, 16));

    const int pageCount = visiblePageCount(m_supplyOffers.size(), kCardPageSize);
    m_page = std::clamp(m_page, 0, pageCount - 1);
    const int start = m_page * kCardPageSize;
    const int cardW = 114;
    const int cardH = 268;
    const int gap = 6;
    const int x0 = kContentRect.left() + 18;
    const int y0 = kContentRect.top() + 108;

    for (int i = 0; i < kCardPageSize; ++i) {
        int idx = start + i;
        if (idx >= m_supplyOffers.size()) break;
        const auto& offer = m_supplyOffers[idx];
        const int owned = InventorySystem::instance().getItemCount(offer.type);
        drawCard(p, QRect(x0 + i * (cardW + gap), y0, cardW, cardH),
                 offer.title, QStringLiteral("当前拥有：%1").arg(owned), offer.description,
                 offer.icon ? *offer.icon : QPixmap(), offer.price, offer.tagColor,
                 Player::instance().coins < offer.price, ZoneAction::BuySupply, idx);
    }

    QRect prev(kContentRect.left() + 142, kContentRect.bottom() - 46, 86, 34);
    QRect next(kContentRect.left() + 274, kContentRect.bottom() - 46, 86, 34);
    drawActionButton(p, prev, QStringLiteral("上一页"), m_buttonGray, ZoneAction::PrevPage, -1, 0, m_page <= 0);
    drawActionButton(p, next, QStringLiteral("下一页"), m_buttonGray, ZoneAction::NextPage, -1, 0, m_page >= pageCount - 1);
    drawTextShadow(p, QRect(kContentRect.left() + 228, kContentRect.bottom() - 42, 46, 26),
                   QString("%1/%2").arg(m_page + 1).arg(pageCount), uiFont(10, QFont::Bold), QColor(91, 54, 18));
}

void ShopDialog::drawWeaponUpgradePage(QPainter& p)
{
    const auto& weapons = InventorySystem::instance().weapons();
    if (m_selectedWeaponIndex >= static_cast<int>(weapons.size())) m_selectedWeaponIndex = 0;

    drawTextShadow(p, QRect(kContentRect.left() + 18, kContentRect.top() + 62, 204, 30),
                   QStringLiteral("我的装备"), shopTitleFont(14, QFont::Bold), QColor(73, 43, 16), Qt::AlignCenter);

    int y = kContentRect.top() + 96;
    for (int i = 0; i < static_cast<int>(weapons.size()); ++i) {
        const Weapon* w = weapons[i];
        if (!w) continue;
        drawRow(p, QRect(kContentRect.left() + 18, y, 204, 58),
                QString::fromStdString(w->getName()),
                QStringLiteral("Lv.%1  +%2  %3/%4")
                    .arg(w->getTier())
                    .arg(w->getEnhancementLevel())
                    .arg(w->getCurrentDur())
                    .arg(w->getMaxDur()),
                i == InventorySystem::instance().currentWeaponIndex() ? QStringLiteral("已装备") : QStringLiteral("背包"),
                weaponIcon(QString::fromStdString(w->getTypeCode())),
                i == m_selectedWeaponIndex, false, ZoneAction::SelectWeapon, i);
        y += 62;
    }

    QRect detail(kContentRect.left() + 228, kContentRect.top() + 96, 242, 176);
    p.setBrush(QColor(104, 62, 24, 55));
    p.setPen(QPen(QColor(143, 91, 28), 2));
    p.drawRoundedRect(detail, 8, 8);

    if (!weapons.empty() && weapons[m_selectedWeaponIndex]) {
        Weapon* w = weapons[m_selectedWeaponIndex];
        drawTextShadow(p, QRect(detail.left() + 18, detail.top() + 14, detail.width() - 36, 30),
                       QString::fromStdString(w->getName()), shopTitleFont(18, QFont::Bold), QColor(54, 29, 10), Qt::AlignCenter);
        drawPixmapFit(p, weaponIcon(QString::fromStdString(w->getTypeCode())), QRect(detail.left() + 20, detail.top() + 50, 96, 112));
        QString stats = QStringLiteral("等级 Lv.%1\n强化 +%2\n伤害 %3\n耐久 %4\n范围 %5\n当前 %6/%7")
            .arg(w->getTier()).arg(w->getEnhancementLevel()).arg(w->getDamage()).arg(w->getMaxDur()).arg(w->getRange()).arg(w->getCurrentDur()).arg(w->getMaxDur());
        drawTextShadow(p, QRect(detail.left() + 124, detail.top() + 52, detail.width() - 140, 108),
                       stats, uiFont(11, QFont::Bold), QColor(54, 31, 12), Qt::AlignLeft | Qt::AlignTop);
    }

    drawActionButton(p, QRect(kContentRect.left() + 238, kContentRect.bottom() - 104, 104, 42),
                     QStringLiteral("强化 I"), m_buttonGreen, ZoneAction::UpgradeWeaponTier, m_selectedWeaponIndex, 1,
                     weapons.empty());
    drawActionButton(p, QRect(kContentRect.left() + 364, kContentRect.bottom() - 104, 104, 42),
                     QStringLiteral("强化 II"), m_buttonGreen, ZoneAction::UpgradeWeaponTier, m_selectedWeaponIndex, 2,
                     weapons.empty());
    drawActionButton(p, QRect(kContentRect.left() + 302, kContentRect.bottom() - 52, 104, 42),
                     QStringLiteral("修复"), m_buttonGold, ZoneAction::RepairWeapon, m_selectedWeaponIndex, 0,
                     weapons.empty());

    drawPrice(p, QRect(kContentRect.left() + 238, kContentRect.bottom() - 150, 96, 34), Config::PRICE_UPG_WEAPON_T1);
    drawPrice(p, QRect(kContentRect.left() + 364, kContentRect.bottom() - 150, 96, 34), Config::PRICE_UPG_WEAPON_T2);
}

void ShopDialog::drawShipUpgradePage(QPainter& p)
{
    if (m_selectedShipUpgradeIndex >= m_shipUpgrades.size()) m_selectedShipUpgradeIndex = 0;
    clampShipUpgradeScroll();
    drawTextShadow(p, QRect(kContentRect.left() + 18, kContentRect.top() + 60, 250, 26),
                   QStringLiteral("升级船只，征服远海"), shopTitleFont(15, QFont::Bold), QColor(73, 43, 16));

    const QRect listViewport = shipUpgradeListViewport();
    int y = listViewport.top() - m_shipUpgradeScroll;
    p.save();
    p.setClipRect(listViewport.adjusted(-2, -2, 2, 2));
    for (int i = 0; i < m_shipUpgrades.size(); ++i) {
        const auto& u = m_shipUpgrades[i];
        const QRect row(listViewport.left(), y, listViewport.width(), kShipUpgradeRowHeight);
        if (row.intersects(listViewport)) {
            drawRow(p, row, u.title, u.subtitle,
                    QStringLiteral("%1 金币").arg(u.price), QPixmap(), i == m_selectedShipUpgradeIndex,
                    false, ZoneAction::SelectWeapon, i, 0, &listViewport);
        }
        y += kShipUpgradeRowHeight + kShipUpgradeRowGap;
    }
    p.restore();

    const int maxScroll = maxShipUpgradeScroll();
    if (maxScroll > 0) {
        const QRect track(listViewport.right() + 6, listViewport.top() + 2, 8, listViewport.height() - 4);
        const int contentHeight = qMax(1, shipUpgradeContentHeight());
        const int thumbHeight = qBound(32, track.height() * listViewport.height() / contentHeight, track.height());
        const int thumbTravel = qMax(1, track.height() - thumbHeight);
        const int thumbY = track.top() + m_shipUpgradeScroll * thumbTravel / maxScroll;
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(QColor(95, 58, 24, 145), 1));
        p.setBrush(QColor(62, 36, 17, 105));
        p.drawRoundedRect(track, 4, 4);
        p.setPen(QPen(QColor(255, 224, 117, 170), 1));
        p.setBrush(QColor(196, 125, 39, 225));
        p.drawRoundedRect(QRect(track.left() + 1, thumbY, track.width() - 2, thumbHeight), 3, 3);
        p.restore();
    }

    const auto& selected = m_shipUpgrades[m_selectedShipUpgradeIndex];
    QRect detail(kContentRect.left() + 282, kContentRect.top() + 72, 188, 210);
    drawPanelText(p, detail, selected.title, selected.description + "\n\n" + selected.statLine);
    drawPrice(p, QRect(detail.left() + 32, detail.bottom() + 20, 124, 36), selected.price);
    drawActionButton(p, QRect(detail.left() + 8, detail.bottom() + 68, 172, 48),
                     QStringLiteral("确认升级"), m_buttonGold, ZoneAction::BuyShipUpgrade,
                     m_selectedShipUpgradeIndex);
}

void ShopDialog::drawBackpackPage(QPainter& p)
{
    InventorySystem& inv = InventorySystem::instance();
    drawTextShadow(p, QRect(kContentRect.left() + 24, kContentRect.top() + 38, kContentRect.width() - 48, 30),
                   QStringLiteral("船舱背包"), shopTitleFont(15, QFont::Bold), QColor(73, 43, 16));

    const auto& weapons = inv.weapons();
    int y = kContentRect.top() + 78;
    for (int i = 0; i < static_cast<int>(weapons.size()); ++i) {
        const Weapon* w = weapons[i];
        if (!w) continue;
        drawRow(p, QRect(kContentRect.left() + 18, y, 220, 58),
                QString::fromStdString(w->getName()),
                QStringLiteral("耐久 %1/%2  +%3  范围%4").arg(w->getCurrentDur()).arg(w->getMaxDur()).arg(w->getEnhancementLevel()).arg(w->getRange()),
                i == inv.currentWeaponIndex() ? QStringLiteral("已装备") : QStringLiteral("可装备"),
                weaponIcon(QString::fromStdString(w->getTypeCode())),
                i == inv.currentWeaponIndex(), w->isBroken(), ZoneAction::SelectBackpackWeapon, i);
        y += 62;
    }

    QRect itemPanel(kContentRect.left() + 258, kContentRect.top() + 78, 212, 188);
    drawPanelText(p, itemPanel, QStringLiteral("道具背包"),
                  QStringLiteral("干粮：%1\n初级修理包：%2\n中级修理包：%3\n高级修理包：%4\n紧急装备修理：%5")
                  .arg(inv.getItemCount(InventoryItemType::Food))
                  .arg(inv.getItemCount(InventoryItemType::ShipRepairT1))
                  .arg(inv.getItemCount(InventoryItemType::ShipRepairT2))
                  .arg(inv.getItemCount(InventoryItemType::ShipRepairT3))
                  .arg(inv.getItemCount(InventoryItemType::EmergencyWeaponRepair)));

    drawActionButton(p, QRect(kContentRect.left() + 272, kContentRect.bottom() - 94, 92, 38),
                     QStringLiteral("用干粮"), m_buttonBlue, ZoneAction::UseItem, 0,
                     static_cast<int>(InventoryItemType::Food), inv.getItemCount(InventoryItemType::Food) <= 0);
    drawActionButton(p, QRect(kContentRect.left() + 376, kContentRect.bottom() - 94, 92, 38),
                     QStringLiteral("初级修理"), m_buttonBlue, ZoneAction::UseItem, 1,
                     static_cast<int>(InventoryItemType::ShipRepairT1), inv.getItemCount(InventoryItemType::ShipRepairT1) <= 0);
    drawActionButton(p, QRect(kContentRect.left() + 272, kContentRect.bottom() - 48, 92, 38),
                     QStringLiteral("中级修理"), m_buttonBlue, ZoneAction::UseItem, 2,
                     static_cast<int>(InventoryItemType::ShipRepairT2), inv.getItemCount(InventoryItemType::ShipRepairT2) <= 0);
    drawActionButton(p, QRect(kContentRect.left() + 376, kContentRect.bottom() - 48, 92, 38),
                     QStringLiteral("高级修理"), m_buttonBlue, ZoneAction::UseItem, 3,
                     static_cast<int>(InventoryItemType::ShipRepairT3), inv.getItemCount(InventoryItemType::ShipRepairT3) <= 0);
    drawActionButton(p, QRect(kContentRect.left() + 272, kContentRect.bottom() - 140, 196, 38),
                     QStringLiteral("紧急装备修理"), m_buttonGold, ZoneAction::EmergencyRepair, m_selectedWeaponIndex,
                     0, inv.getItemCount(InventoryItemType::EmergencyWeaponRepair) <= 0 || weapons.empty());
}

void ShopDialog::drawRightPanel(QPainter& p)
{
    p.drawPixmap(kRightRect, m_sidePanel);
    drawTextShadow(p, QRect(kRightRect.left() + 16, kRightRect.top() + 10, kRightRect.width() - 32, 28),
                   merchantTitle(), shopTitleFont(14, QFont::Bold), QColor(255, 221, 142));
    drawPixmapFit(p, m_shopkeeper, QRect(kRightRect.left() + 16, kRightRect.top() + 44, kRightRect.width() - 32, 118));

    InventorySystem& inv = InventorySystem::instance();
    QString status;
    switch (m_category) {
    case Category::Equipment:
        status = QStringLiteral("装备背包：%1/%2\n购买装备会进入背包；背包满时需要替换旧装备。")
            .arg(inv.weaponCount()).arg(inv.maxWeaponCapacity());
        break;
    case Category::Supplies:
        status = QStringLiteral("物品数量：%1/%2\n干粮：%3\n修理包：%4/%5/%6\n紧急修理：%7")
            .arg(inv.getTotalItemCount()).arg(Config::MAX_ITEM_BACKPACK)
            .arg(inv.getItemCount(InventoryItemType::Food))
            .arg(inv.getItemCount(InventoryItemType::ShipRepairT1))
            .arg(inv.getItemCount(InventoryItemType::ShipRepairT2))
            .arg(inv.getItemCount(InventoryItemType::ShipRepairT3))
            .arg(inv.getItemCount(InventoryItemType::EmergencyWeaponRepair));
        break;
    case Category::WeaponUpgrade: {
        const auto& weapons = inv.weapons();
        if (!weapons.empty() && m_selectedWeaponIndex >= 0 && m_selectedWeaponIndex < static_cast<int>(weapons.size()) && weapons[m_selectedWeaponIndex]) {
            Weapon* w = weapons[m_selectedWeaponIndex];
            status = QStringLiteral("%1\nLv.%2  强化 +%3\n耐久 %4/%5\n伤害 %6  范围 %7\n强化提升伤害和耐久。")
                .arg(QString::fromStdString(w->getName()))
                .arg(w->getTier()).arg(w->getEnhancementLevel())
                .arg(w->getCurrentDur()).arg(w->getMaxDur())
                .arg(w->getDamage()).arg(w->getRange());
        } else {
            status = QStringLiteral("当前没有可强化装备。");
        }
        break;
    }
    case Category::ShipUpgrade: {
        const auto& selected = m_shipUpgrades[qBound(0, m_selectedShipUpgradeIndex, m_shipUpgrades.size() - 1)];
        status = QStringLiteral("%1\n%2\n价格：%3 金币\n升级会立即生效。")
            .arg(selected.title).arg(selected.statLine).arg(selected.price);
        break;
    }
    case Category::Backpack:
        status = QStringLiteral("当前装备：%1\n装备背包：%2/%3\n物品数量：%4/%5\n损坏装备需先修复再装备。")
            .arg(inv.currentWeapon() ? QString::fromStdString(inv.currentWeapon()->getName()) : QStringLiteral("无"))
            .arg(inv.weaponCount()).arg(inv.maxWeaponCapacity())
            .arg(inv.getTotalItemCount()).arg(Config::MAX_ITEM_BACKPACK);
        break;
    case Category::Close:
        break;
    }

    drawPanelText(p, QRect(kRightRect.left() + 15, kRightRect.top() + 176, kRightRect.width() - 30, 118),
                  QStringLiteral("店主建议"), merchantBody());
    drawPanelText(p, QRect(kRightRect.left() + 15, kRightRect.top() + 294, kRightRect.width() - 30, 122),
                  QStringLiteral("当前状态"), status);
}

void ShopDialog::drawFooterHint(QPainter& p)
{
    Q_UNUSED(p);
}

void ShopDialog::drawNotice(QPainter& p)
{
    if (!hasNotice()) return;

    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 10, 18, 125));
    p.drawRect(rect());

    QRect panel(370, 176, 540, 430);
    if (!m_noticeBoard.isNull()) {
        p.drawPixmap(panel, m_noticeBoard, m_noticeBoard.rect());
    } else {
        p.fillRect(panel, QColor(64, 35, 16, 245));
    }

    QRect inner = panel.adjusted(78, 90, -76, -92);
    drawPixmapFit(p, m_noticeIcon.isNull() ? m_coinIcon : m_noticeIcon,
                  QRect(inner.left(), inner.top() + 10, 58, 58));
    drawTextShadow(p, QRect(inner.left() + 74, inner.top() + 4, inner.width() - 86, 38),
                   m_noticeTitle, uiFont(18, QFont::Bold), QColor(96, 48, 12), Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(inner.left() + 74, inner.top() + 56, inner.width() - 86, 108),
                   m_noticeBody, uiFont(11, QFont::Bold), QColor(75, 45, 18),
                   Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap);

    QRect hint(panel.center().x() - 76, panel.bottom() - 102, 152, 44);
    if (!m_noticeButton.isNull()) {
        p.drawPixmap(hint, m_noticeButton, m_noticeButton.rect());
    }
    drawTextShadow(p, hint, QStringLiteral("点击关闭"), uiFont(10, QFont::Bold), QColor(255, 232, 178));
    p.restore();
}

void ShopDialog::drawTextShadow(QPainter& p, const QRect& rect, const QString& text,
                                const QFont& font, const QColor& color, int flags)
{
    if (rect.isEmpty() || text.isEmpty()) return;

    QFont cleanFont = font;
    if (cleanFont.pointSize() <= 0) {
        cleanFont.setPointSize(10);
    }
    cleanFont.setStyleStrategy(QFont::PreferAntialias);
    cleanFont.setHintingPreference(QFont::PreferFullHinting);

    const bool wraps = (flags & Qt::TextWordWrap) || text.contains('\n');
    const int drawFlags = (flags & Qt::TextWordWrap)
        ? (flags | Qt::TextWordWrap | Qt::TextWrapAnywhere)
        : flags;
    const int minPointSize = wraps ? 8 : 9;
    const int hPad = rect.width() <= 64 ? 1 : 3;
    const int vPad = rect.height() <= 22 ? 0 : 1;
    QRect textRect = rect.adjusted(hPad, vPad, -hPad, -vPad);
    if (textRect.width() <= 0 || textRect.height() <= 0) return;

    while (cleanFont.pointSize() > minPointSize) {
        QFontMetrics metrics(cleanFont);
        QRect measured = metrics.boundingRect(textRect, drawFlags, text);
        if (measured.width() <= textRect.width() + 1 && measured.height() <= textRect.height() + 1) {
            break;
        }
        cleanFont.setPointSize(cleanFont.pointSize() - 1);
    }

    QString drawText = text;
    if (!wraps) {
        QFontMetrics metrics(cleanFont);
        drawText = metrics.elidedText(text, Qt::ElideRight, textRect.width());
    }

    p.save();
    p.setClipRect(rect.adjusted(1, 1, -1, -1));
    p.setFont(cleanFont);
    p.setPen(color);
    p.drawText(textRect, drawFlags, drawText);
    p.restore();
}

void ShopDialog::drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& rect)
{
    if (pixmap.isNull()) return;
    QSize size = pixmap.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    QRect target(rect.left() + (rect.width() - size.width()) / 2,
                 rect.top() + (rect.height() - size.height()) / 2,
                 size.width(), size.height());
    p.drawPixmap(target, pixmap);
}

void ShopDialog::drawPanelText(QPainter& p, const QRect& rect, const QString& title, const QString& body)
{
    if (!m_infoParchment.isNull()) {
        p.drawPixmap(rect, m_infoParchment);
    } else {
        p.setBrush(QColor(225, 195, 135, 230));
        p.setPen(QPen(QColor(112, 70, 26), 2));
        p.drawRoundedRect(rect, 6, 6);
    }
    const bool roomy = rect.height() >= 170;
    const int titleTop = rect.top() + (roomy ? 22 : 12);
    const int titleHeight = roomy ? 34 : 26;
    const int bodyTop = rect.top() + (roomy ? 64 : 42);
    const int bodyBottomPad = roomy ? 18 : 12;
    drawTextShadow(p, QRect(rect.left() + 18, titleTop, rect.width() - 36, titleHeight),
                   title, shopTitleFont(roomy ? 15 : 12, QFont::Bold), QColor(68, 36, 12), Qt::AlignCenter);
    drawTextShadow(p, QRect(rect.left() + 22, bodyTop, rect.width() - 44, rect.bottom() - bodyTop - bodyBottomPad + 1),
                   body, uiFont(roomy ? 11 : 9, QFont::Bold), QColor(58, 34, 14), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap);
}

void ShopDialog::drawPrice(QPainter& p, const QRect& rect, int price, bool purchasable)
{
    if (purchasable && !m_buttonGold.isNull()) {
        p.drawPixmap(rect, m_buttonGold);
        drawPixmapFit(p, m_coinIcon, QRect(rect.left() + 8, rect.top() + 5, 24, rect.height() - 10));
        drawTextShadow(p, QRect(rect.left() + 34, rect.top() + 3, rect.width() - 42, rect.height() - 6),
                       QStringLiteral("购买 %1").arg(price), uiFont(12, QFont::Bold),
                       QColor(255, 247, 205), Qt::AlignCenter);
        return;
    }

    p.drawPixmap(rect, m_priceBadge);
    const int textOffset = rect.width() < 100 ? 32 : 38;
    const int fontSize = rect.width() < 100 ? 13 : 14;
    drawTextShadow(p, QRect(rect.left() + textOffset, rect.top() + 3,
                            rect.width() - textOffset - 9, rect.height() - 6),
                   QString::number(price), uiFont(fontSize, QFont::Bold),
                   QColor(255, 239, 202), Qt::AlignCenter);
}

void ShopDialog::drawCard(QPainter& p, const QRect& rect, const QString& title,
                          const QString& subtitle, const QString& description,
                          const QPixmap& icon, int price, const QColor& tagColor,
                          bool disabled, ZoneAction action, int index, int value)
{
    const bool purchasable = !disabled;
    const bool hover = isHovered(rect);
    const QPixmap& bg = (purchasable && hover) ? m_cardHover : m_cardNormal;
    p.drawPixmap(rect, bg);

    if (purchasable) {
        p.save();
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(255, 211, 72), 3));
        p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 6, 6);
        QRect ribbon(rect.left() + 16, rect.top() + 48, rect.width() - 32, 22);
        p.setBrush(QColor(92, 143, 43, 220));
        p.setPen(QPen(QColor(255, 231, 128), 1));
        p.drawRoundedRect(ribbon, 4, 4);
        drawTextShadow(p, ribbon, QStringLiteral("可购买"), uiFont(10, QFont::Bold),
                       QColor(255, 247, 211), Qt::AlignCenter);
        p.restore();
    }

    QRect tag(rect.left() + 8, rect.top() + 12, rect.width() - 16, 36);
    p.setBrush(tagColor);
    p.setPen(QPen(QColor(91, 50, 12), 2));
    p.drawRoundedRect(tag, 4, 4);
    drawTextShadow(p, tag, title, shopTitleFont(12, QFont::Bold), QColor(255, 242, 210),
                   Qt::AlignCenter | Qt::TextWordWrap);
    drawPixmapFit(p, icon, QRect(rect.left() + 18, rect.top() + (purchasable ? 74 : 56), rect.width() - 36, purchasable ? 54 : 72));
    drawTextShadow(p, QRect(rect.left() + 13, rect.top() + 136, rect.width() - 26, 48),
                   description, uiFont(9, QFont::Bold), QColor(58, 34, 12), Qt::AlignCenter | Qt::TextWordWrap);
    drawTextShadow(p, QRect(rect.left() + 13, rect.top() + 188, rect.width() - 26, 34),
                   subtitle, uiFont(9, QFont::Bold), QColor(70, 40, 14), Qt::AlignCenter | Qt::TextWordWrap);
    drawPrice(p, QRect(rect.left() + 8, rect.bottom() - 38, rect.width() - 16, 36), price, purchasable);

    const bool canReportPurchaseProblem = action == ZoneAction::BuyWeapon || action == ZoneAction::BuySupply;
    if (!disabled || canReportPurchaseProblem) addZone(rect, action, index, value);
}

void ShopDialog::drawRow(QPainter& p, const QRect& rect, const QString& title,
                         const QString& subtitle, const QString& rightText,
                         const QPixmap& icon, bool selected, bool disabled,
                         ZoneAction action, int index, int value, const QRect* hitClip)
{
    const QRect hitRect = hitClip ? rect.intersected(*hitClip) : rect;
    const QPixmap& bg = disabled ? m_rowDisabled : (selected ? m_rowSelected : (isHovered(hitRect) ? m_rowHover : m_rowNormal));
    p.drawPixmap(rect, bg);
    const int iconWidth = icon.isNull() ? 0 : qMin(48, rect.height() - 12);
    const QFont rightFont = uiFont(10, QFont::Bold);
    const int measuredRightWidth = rightText.isEmpty() ? 0 : QFontMetrics(rightFont).horizontalAdvance(rightText) + 12;
    const int rightWidth = rightText.isEmpty() ? 0 : qBound(58, measuredRightWidth, qMax(84, rect.width() / 3));
    const int textLeft = rect.left() + (icon.isNull() ? 28 : 64);
    const int textRight = rect.right() - rightWidth - (rightWidth > 0 ? 18 : 16);
    const int textWidth = qMax(54, textRight - textLeft + 1);
    drawPixmapFit(p, icon, QRect(rect.left() + 10, rect.top() + 7, iconWidth, rect.height() - 14));
    drawTextShadow(p, QRect(textLeft, rect.top() + 8, textWidth, 25),
                   title, shopTitleFont(13, QFont::Bold), QColor(54, 29, 10), Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(textLeft, rect.top() + 34, textWidth, 22),
                   subtitle, uiFont(10, QFont::Bold), QColor(73, 44, 17), Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(rect.right() - rightWidth - 16, rect.top() + 13, rightWidth, rect.height() - 26),
                   rightText, rightFont, selected ? QColor(31, 110, 29) : QColor(94, 55, 16),
                   Qt::AlignRight | Qt::AlignVCenter);
    if (!disabled && !hitRect.isEmpty()) addZone(hitRect, action, index, value);
}

void ShopDialog::drawActionButton(QPainter& p, const QRect& rect, const QString& text,
                                  const QPixmap& pixmap, ZoneAction action, int index, int value, bool disabled)
{
    const QPixmap& bg = disabled ? m_buttonGray : pixmap;
    p.drawPixmap(rect, bg);
    const int fontSize = rect.width() < 108 ? 11 : 13;
    drawTextShadow(p, rect.adjusted(4, 2, -4, -2), text, shopTitleFont(fontSize, QFont::Bold),
                   disabled ? QColor(178, 168, 148) : QColor(255, 238, 182));
    if (!disabled) addZone(rect, action, index, value);
}

void ShopDialog::addZone(const QRect& rect, ZoneAction action, int index, int value)
{
    m_zones.push_back({rect, action, index, value});
}

const ShopDialog::ClickZone* ShopDialog::zoneAt(const QPoint& pos) const
{
    for (int i = m_zones.size() - 1; i >= 0; --i) {
        if (m_zones[i].rect.contains(pos)) return &m_zones[i];
    }
    return nullptr;
}

bool ShopDialog::isHovered(const QRect& rect) const
{
    return rect.contains(m_mousePos);
}

bool ShopDialog::hasNotice() const
{
    return !m_noticeTitle.isEmpty() || !m_noticeBody.isEmpty();
}

void ShopDialog::showShopNotice(const QString& title, const QString& body)
{
    m_noticeTitle = title;
    m_noticeBody = body;
    update();
}

void ShopDialog::showInsufficientCoinsNotice(int requiredCoins)
{
    const int currentCoins = Player::instance().coins;
    const int missingCoins = qMax(0, requiredCoins - currentCoins);
    showShopNotice(QStringLiteral("金币不足"),
                   QStringLiteral("这笔交易需要 %1 金币。\n当前金币：%2，还差 %3。")
                   .arg(requiredCoins)
                   .arg(currentCoins)
                   .arg(missingCoins));
}

void ShopDialog::clearNotice()
{
    m_noticeTitle.clear();
    m_noticeBody.clear();
    update();
}

void ShopDialog::handleZone(const ClickZone& zone)
{
    switch (zone.action) {
    case ZoneAction::SelectCategory:
        setCategory(static_cast<Category>(zone.index));
        break;
    case ZoneAction::BuyWeapon: {
        const auto& offer = m_weaponOffers[zone.index];
        buyWeapon(ItemFactory::createWeapon(offer.type.toStdString(), offer.tier));
        break;
    }
    case ZoneAction::BuySupply: {
        const auto& offer = m_supplyOffers[zone.index];
        buyBackpackItem(offer.type, offer.price, offer.title);
        break;
    }
    case ZoneAction::BuyShipUpgrade: {
        const auto& offer = m_shipUpgrades[zone.index];
        Item* item = ItemFactory::createAttributeUpgrade(offer.attr.toStdString(), offer.tier);
        buyAndUseAttributeUpgrade(item);
        delete item;
        break;
    }
    case ZoneAction::SelectWeapon:
        if (m_category == Category::ShipUpgrade) m_selectedShipUpgradeIndex = zone.index;
        else m_selectedWeaponIndex = zone.index;
        update();
        break;
    case ZoneAction::SelectBackpackWeapon:
        selectWeaponFromBackpack(zone.index);
        break;
    case ZoneAction::UseItem:
        if (zone.value == static_cast<int>(InventoryItemType::Food)) useFoodFromBackpack();
        else if (zone.value == static_cast<int>(InventoryItemType::ShipRepairT1)) useShipRepairFromBackpack(1);
        else if (zone.value == static_cast<int>(InventoryItemType::ShipRepairT2)) useShipRepairFromBackpack(2);
        else if (zone.value == static_cast<int>(InventoryItemType::ShipRepairT3)) useShipRepairFromBackpack(3);
        break;
    case ZoneAction::UpgradeWeaponTier:
        m_selectedWeaponIndex = zone.index;
        if (zone.value > 0) {
            buyWeaponUpgrade(zone.value);
        } else {
            int inferredTier = 1;
            const auto& weapons = InventorySystem::instance().weapons();
            if (m_selectedWeaponIndex >= 0 && m_selectedWeaponIndex < static_cast<int>(weapons.size()) && weapons[m_selectedWeaponIndex]) {
                inferredTier = std::clamp(weapons[m_selectedWeaponIndex]->getTier(), 1, 2);
            }
            buyWeaponUpgrade(inferredTier);
        }
        break;
    case ZoneAction::RepairWeapon:
        m_selectedWeaponIndex = zone.index;
        buyShopWeaponRepair();
        break;
    case ZoneAction::EmergencyRepair:
        useEmergencyWeaponRepairFromBackpack();
        break;
    case ZoneAction::PrevPage:
        if (m_page > 0) --m_page;
        update();
        break;
    case ZoneAction::NextPage:
        ++m_page;
        update();
        break;
    case ZoneAction::Close:
        accept();
        break;
    }
}

void ShopDialog::setCategory(Category category)
{
    if (category == Category::Close) {
        accept();
        return;
    }
    if (m_category != category) {
        m_category = category;
        resetPage();
        update();
    }
}

void ShopDialog::resetPage()
{
    m_page = 0;
    m_shipUpgradeScroll = 0;
}

void ShopDialog::buyBackpackItem(InventoryItemType type, int price, const QString& displayName)
{
    Player& player = Player::instance();
    if (player.coins < price) {
        showInsufficientCoinsNotice(price);
        return;
    }
    if (!InventorySystem::instance().canAddItem()) {
        GameUi::showWoodMessage(this, QStringLiteral("物品背包已满"),
                                QStringLiteral("物品背包容量已满。"));
        return;
    }

    player.coins -= price;
    InventorySystem::instance().addItem(type, 1);
    updateCoinsLabel();
    refreshBackpackUI();
    showShopNotice(QStringLiteral("购买成功"),
                   QStringLiteral("已购买：%1\n花费 %2 金币，物品已放入背包。")
                   .arg(displayName)
                   .arg(price));
}

void ShopDialog::buyWeapon(Weapon* weapon)
{
    if (!weapon) return;
    Player& player = Player::instance();
    int price = weapon->getValue();
    const QString weaponName = QString::fromStdString(weapon->getName());
    if (player.coins < price) {
        showInsufficientCoinsNotice(price);
        delete weapon;
        return;
    }

    InventorySystem& inv = InventorySystem::instance();
    if (inv.canAddWeapon()) {
        player.coins -= price;
        inv.addWeapon(weapon);
        m_selectedWeaponIndex = inv.weaponCount() - 1;
        updateCoinsLabel();
        refreshBackpackUI();
        showShopNotice(QStringLiteral("购买成功"),
                       QStringLiteral("已购买：%1\n花费 %2 金币，装备已放入背包。")
                       .arg(weaponName)
                       .arg(price));
        return;
    }

    int replaceIndex = askReplaceWeaponIndex();
    if (replaceIndex < 0) {
        delete weapon;
        return;
    }

    player.coins -= price;
    inv.replaceWeapon(replaceIndex, weapon);
    m_selectedWeaponIndex = replaceIndex;
    updateCoinsLabel();
    refreshBackpackUI();
    showShopNotice(QStringLiteral("替换成功"),
                   QStringLiteral("已购买：%1\n花费 %2 金币，并替换所选装备。")
                   .arg(weaponName)
                   .arg(price));
}

void ShopDialog::buyAndUseAttributeUpgrade(Item* item)
{
    if (!item) return;
    Player& player = Player::instance();
    int price = item->getValue();
    const QString itemName = QString::fromStdString(item->getName());
    if (player.coins < price) {
        showInsufficientCoinsNotice(price);
        return;
    }

    player.coins -= price;
    item->use(player);
    updateCoinsLabel();
    refreshBackpackUI();
    showShopNotice(QStringLiteral("升级成功"),
                   QStringLiteral("%1 已生效。\n花费 %2 金币。")
                   .arg(itemName)
                   .arg(price));
}

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
    } else if (tier == 3) {
        price = Config::PRICE_UPG_WEAPON_T3;
        damageBoost = Config::VAL_UPG_WPN_DMG_T3;
        durabilityBoost = Config::VAL_UPG_WPN_DUR_T3;
    }

    if (player.coins < price) {
        showInsufficientCoinsNotice(price);
        return;
    }

    int index = m_selectedWeaponIndex;
    const auto& weapons = InventorySystem::instance().weapons();
    if (index < 0 || index >= static_cast<int>(weapons.size())) {
        index = askWeaponIndex(QStringLiteral("选择强化装备"), QStringLiteral("请选择要强化的装备："), true);
    }
    if (index < 0) return;

    QString weaponName = QStringLiteral("装备");
    if (index >= 0 && index < static_cast<int>(weapons.size()) && weapons[index]) {
        weaponName = QString::fromStdString(weapons[index]->getName());
    }
    player.coins -= price;
    InventorySystem::instance().upgradeWeapon(index, damageBoost, durabilityBoost);
    updateCoinsLabel();
    refreshBackpackUI();
    showShopNotice(QStringLiteral("强化成功"),
                   QStringLiteral("%1 强化完成。\n花费 %2 金币。")
                   .arg(weaponName)
                   .arg(price));
}

void ShopDialog::buyShopWeaponRepair()
{
    Player& player = Player::instance();
    int price = Config::PRICE_SHOP_WEAPON_REPAIR;
    if (player.coins < price) {
        showInsufficientCoinsNotice(price);
        return;
    }

    int index = m_selectedWeaponIndex;
    const auto& weapons = InventorySystem::instance().weapons();
    if (index < 0 || index >= static_cast<int>(weapons.size())) {
        index = askWeaponIndex(QStringLiteral("选择修复装备"), QStringLiteral("请选择要修复的装备："), true);
    }
    if (index < 0) return;

    Weapon* weapon = InventorySystem::instance().weapons()[index];
    if (!weapon || weapon->getCurrentDur() >= weapon->getMaxDur()) {
        GameUi::showWoodMessage(this, QStringLiteral("无需修复"), QStringLiteral("该装备耐久已满。"));
        return;
    }

    player.coins -= price;
    InventorySystem::instance().repairWeaponByPercent(index, Config::SHOP_WEAPON_REPAIR_PERCENT);
    updateCoinsLabel();
    refreshBackpackUI();
    showShopNotice(QStringLiteral("修复成功"),
                   QStringLiteral("%1 已完成修复。\n花费 %2 金币。")
                   .arg(QString::fromStdString(weapon->getName()))
                   .arg(price));
}

void ShopDialog::useFoodFromBackpack()
{
    if (!InventorySystem::instance().useFood(Player::instance())) {
        GameUi::showWoodMessage(this, QStringLiteral("使用失败"), QStringLiteral("没有航海干粮。"));
    }
    refreshBackpackUI();
}

void ShopDialog::useShipRepairFromBackpack(int tier)
{
    if (!InventorySystem::instance().useShipRepairKit(Player::instance(), tier)) {
        GameUi::showWoodMessage(this, QStringLiteral("使用失败"), QStringLiteral("没有对应等级的船体修理包。"));
    }
    refreshBackpackUI();
}

void ShopDialog::useEmergencyWeaponRepairFromBackpack()
{
    if (InventorySystem::instance().getItemCount(InventoryItemType::EmergencyWeaponRepair) <= 0) {
        GameUi::showWoodMessage(this, QStringLiteral("使用失败"), QStringLiteral("没有紧急装备修理工具。"));
        return;
    }
    int index = m_selectedWeaponIndex;
    const auto& weapons = InventorySystem::instance().weapons();
    if (index < 0 || index >= static_cast<int>(weapons.size())) {
        index = askWeaponIndex(QStringLiteral("紧急修理"), QStringLiteral("请选择要修理的装备："), true);
    }
    if (index < 0) return;

    if (!InventorySystem::instance().useEmergencyWeaponRepair(index)) {
        GameUi::showWoodMessage(this, QStringLiteral("使用失败"), QStringLiteral("该装备可能已满耐久，或无法修复。"));
    }
    refreshBackpackUI();
}

int ShopDialog::askWeaponIndex(const QString& title, const QString& label, bool allowBroken)
{
    const auto& weapons = InventorySystem::instance().weapons();
    if (weapons.empty()) {
        GameUi::showWoodMessage(this, title, QStringLiteral("当前没有任何装备。"));
        return -1;
    }

    QStringList options;
    std::vector<int> indexMap;
    for (int i = 0; i < static_cast<int>(weapons.size()); ++i) {
        const Weapon* weapon = weapons[i];
        if (!weapon) continue;
        if (!allowBroken && weapon->isBroken()) continue;
        options << weaponDisplayText(weapon, i);
        indexMap.push_back(i);
    }

    if (options.isEmpty()) {
        GameUi::showWoodMessage(this, title, QStringLiteral("没有可选择的装备。"));
        return -1;
    }

    const int selected = GameUi::selectWoodOption(this, title, label, options);
    if (selected < 0 || selected >= static_cast<int>(indexMap.size())) return -1;
    return indexMap[selected];
}

int ShopDialog::askReplaceWeaponIndex()
{
    GameUi::showWoodMessage(this, QStringLiteral("装备背包已满"),
                            QStringLiteral("装备背包最多携带 %1 件装备，请选择旧装备替换。").arg(Config::MAX_WEAPON_BACKPACK));
    return askWeaponIndex(QStringLiteral("替换装备"), QStringLiteral("请选择要被替换的旧装备："), true);
}

void ShopDialog::selectWeaponFromBackpack(int index)
{
    if (!InventorySystem::instance().selectWeapon(index)) {
        GameUi::showWoodMessage(this, QStringLiteral("切换失败"), QStringLiteral("该装备已损坏或无法装备。"));
        return;
    }
    m_selectedWeaponIndex = index;
    refreshBackpackUI();
}

void ShopDialog::updateCoinsLabel()
{
    update();
}

void ShopDialog::refreshBackpackUI()
{
    update();
}

QString ShopDialog::weaponDisplayText(const Weapon* weapon, int index) const
{
    if (!weapon) return QStringLiteral("空装备");
    QString status;
    if (weapon->isBroken()) status = QStringLiteral("【已损坏】");
    else if (index == InventorySystem::instance().currentWeaponIndex()) status = QStringLiteral("【当前】");
    else status = QStringLiteral("【背包】");

    const QString durabilityText = weapon->isInfiniteDurability()
        ? QStringLiteral("\u221e")
        : QStringLiteral("%1/%2").arg(weapon->getCurrentDur()).arg(weapon->getMaxDur());

    return QStringLiteral("%1%2  Lv.%3 +%4  耐久:%5  范围:%6  伤害:%7")
        .arg(status)
        .arg(QString::fromStdString(weapon->getName()))
        .arg(weapon->getTier())
        .arg(weapon->getEnhancementLevel())
        .arg(durabilityText)
        .arg(weapon->getRange())
        .arg(weapon->getDamage());
}

QPixmap ShopDialog::weaponIcon(const QString& type) const
{
    if (type == "Net") return m_iconNet;
    if (type == "Harpoon") return m_iconHarpoon;
    if (type == "Pistol") return m_iconPistol;
    if (type == "Shotgun") return m_iconShotgun;
    return m_iconRod;
}

QString ShopDialog::categoryTitle() const
{
    switch (m_category) {
    case Category::Equipment: return QStringLiteral("装备商店");
    case Category::Supplies: return QStringLiteral("道具补给");
    case Category::WeaponUpgrade: return QStringLiteral("装备强化");
    case Category::ShipUpgrade: return QStringLiteral("船体升级");
    case Category::Backpack: return QStringLiteral("船舱背包");
    case Category::Close: return QStringLiteral("离开商店");
    }
    return {};
}

QString ShopDialog::merchantTitle() const
{
    switch (m_category) {
    case Category::ShipUpgrade: return QStringLiteral("港口船长");
    case Category::WeaponUpgrade: return QStringLiteral("船长的话");
    default: return QStringLiteral("店主的话");
    }
}

QString ShopDialog::merchantBody() const
{
    switch (m_category) {
    case Category::Equipment:
        return QStringLiteral("好渔具能让你事半功倍，挑选合适的装备，出海才更有底气！");
    case Category::Supplies:
        return QStringLiteral("这里出售各类船体修理包和紧急装备修理工具，买好后会放入背包。");
    case Category::WeaponUpgrade:
        return QStringLiteral("强化装备能提升战斗力，让你在深海中更有底气。");
    case Category::ShipUpgrade:
        return QStringLiteral("好的船只需要不断强化，才能在风浪中屹立不倒！");
    case Category::Backpack:
        return QStringLiteral("在这里整理装备和道具。损坏装备需要先修理后才能装备。");
    case Category::Close:
        return {};
    }
    return {};
}

QString ShopDialog::tipText() const
{
    switch (m_category) {
    case Category::Equipment:
        return QStringLiteral("高品质装备可提升捕获率和生存能力，挑战更远的海域吧！");
    case Category::Supplies:
        return QStringLiteral("合理储备修理包和干粮，关键时刻能避免船只和装备受损导致的损失。");
    case Category::WeaponUpgrade:
        return QStringLiteral("优先提升常用装备的攻击伤害与耐久，能更高效应对强敌。");
    case Category::ShipUpgrade:
        return QStringLiteral("永久升级立即生效，合理分配资源，让航行更加顺利。");
    case Category::Backpack:
        return QStringLiteral("出航前检查装备耐久和补给数量，别把麻烦留给海上。");
    case Category::Close:
        return {};
    }
    return {};
}

int ShopDialog::visiblePageCount(int itemCount, int pageSize) const
{
    return std::max(1, (itemCount + pageSize - 1) / pageSize);
}

QRect ShopDialog::shipUpgradeListViewport() const
{
    return QRect(kContentRect.left() + 18, kContentRect.top() + 90, 250, 286);
}

int ShopDialog::shipUpgradeContentHeight() const
{
    if (m_shipUpgrades.isEmpty()) return 0;
    return m_shipUpgrades.size() * (kShipUpgradeRowHeight + kShipUpgradeRowGap) - kShipUpgradeRowGap;
}

int ShopDialog::maxShipUpgradeScroll() const
{
    return qMax(0, shipUpgradeContentHeight() - shipUpgradeListViewport().height());
}

void ShopDialog::clampShipUpgradeScroll()
{
    m_shipUpgradeScroll = qBound(0, m_shipUpgradeScroll, maxShipUpgradeScroll());
}

void ShopDialog::scrollShipUpgradeList(int deltaPixels)
{
    const int oldScroll = m_shipUpgradeScroll;
    m_shipUpgradeScroll = qBound(0, m_shipUpgradeScroll + deltaPixels, maxShipUpgradeScroll());
    if (m_shipUpgradeScroll != oldScroll) {
        update();
    }
}
