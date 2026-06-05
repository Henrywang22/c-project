#pragma once

#include <QDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QVector>
#include <QWheelEvent>

#include "GameConfig.h"
#include "InventorySystem.h"
#include "Item.h"
#include "ItemFactory.h"
#include "Player.h"
#include "Weapon.h"

class ShopDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShopDialog(QWidget* parent = nullptr);
    ~ShopDialog() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class Category {
        Equipment,
        Supplies,
        WeaponUpgrade,
        ShipUpgrade,
        Backpack,
        Close
    };

    enum class ZoneAction {
        SelectCategory,
        BuyWeapon,
        BuySupply,
        BuyShipUpgrade,
        SelectWeapon,
        SelectBackpackWeapon,
        UseItem,
        UpgradeWeaponTier,
        RepairWeapon,
        EmergencyRepair,
        PrevPage,
        NextPage,
        Close
    };

    struct ClickZone {
        QRect rect;
        ZoneAction action;
        int index = -1;
        int value = 0;
    };

    struct WeaponOffer {
        QString type;
        int tier = 1;
        QString title;
        QString subtitle;
        QString description;
        QColor tagColor;
    };

    struct SupplyOffer {
        InventoryItemType type;
        int tier = 0;
        int price = 0;
        QString title;
        QString subtitle;
        QString description;
        QPixmap* icon = nullptr;
        QColor tagColor;
    };

    struct ShipUpgradeOffer {
        QString attr;
        int tier = 1;
        int price = 0;
        QString title;
        QString subtitle;
        QString description;
        QString statLine;
    };

    void loadAssets();
    void buildCatalogs();

    void drawBackground(QPainter& p);
    void drawTopStatus(QPainter& p);
    void drawMainFrame(QPainter& p);
    void drawNav(QPainter& p);
    void drawTitle(QPainter& p);
    void drawRightPanel(QPainter& p);
    void drawFooterHint(QPainter& p);
    void drawCurrentPage(QPainter& p);
    void drawEquipmentPage(QPainter& p);
    void drawSuppliesPage(QPainter& p);
    void drawWeaponUpgradePage(QPainter& p);
    void drawShipUpgradePage(QPainter& p);
    void drawBackpackPage(QPainter& p);
    void drawNotice(QPainter& p);

    void drawTextShadow(QPainter& p, const QRect& rect, const QString& text,
                        const QFont& font, const QColor& color,
                        int flags = Qt::AlignCenter);
    void drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& rect);
    void drawPanelText(QPainter& p, const QRect& rect, const QString& title,
                       const QString& body);
    void drawPrice(QPainter& p, const QRect& rect, int price, bool purchasable = false);
    void drawCard(QPainter& p, const QRect& rect, const QString& title,
                  const QString& subtitle, const QString& description,
                  const QPixmap& icon, int price, const QColor& tagColor,
                  bool disabled, ZoneAction action, int index, int value = 0);
    void drawRow(QPainter& p, const QRect& rect, const QString& title,
                 const QString& subtitle, const QString& rightText,
                 const QPixmap& icon, bool selected, bool disabled,
                 ZoneAction action, int index, int value = 0,
                 const QRect* hitClip = nullptr);
    void drawActionButton(QPainter& p, const QRect& rect, const QString& text,
                          const QPixmap& pixmap, ZoneAction action,
                          int index = -1, int value = 0, bool disabled = false);

    void addZone(const QRect& rect, ZoneAction action, int index = -1, int value = 0);
    const ClickZone* zoneAt(const QPoint& pos) const;
    bool isHovered(const QRect& rect) const;
    bool hasNotice() const;
    void showShopNotice(const QString& title, const QString& body);
    void showInsufficientCoinsNotice(int requiredCoins);
    void clearNotice();

    void handleZone(const ClickZone& zone);
    void setCategory(Category category);
    void resetPage();

    void buyBackpackItem(InventoryItemType type, int price, const QString& displayName);
    void buyWeapon(Weapon* weapon);
    void buyAndUseAttributeUpgrade(Item* item);
    void buyWeaponUpgrade(int tier);
    void buyShopWeaponRepair();
    void useFoodFromBackpack();
    void useShipRepairFromBackpack(int tier);
    void useEmergencyWeaponRepairFromBackpack();
    int askWeaponIndex(const QString& title, const QString& label, bool allowBroken = true);
    int askReplaceWeaponIndex();
    void selectWeaponFromBackpack(int index);
    void updateCoinsLabel();
    void refreshBackpackUI();
    QString weaponDisplayText(const Weapon* weapon, int index) const;

    QPixmap weaponIcon(const QString& type) const;
    QString categoryTitle() const;
    QString merchantTitle() const;
    QString merchantBody() const;
    QString tipText() const;
    int visiblePageCount(int itemCount, int pageSize) const;
    QRect shipUpgradeListViewport() const;
    int shipUpgradeContentHeight() const;
    int maxShipUpgradeScroll() const;
    void clampShipUpgradeScroll();
    void scrollShipUpgradeList(int deltaPixels);

    Category m_category = Category::Equipment;
    QPoint m_mousePos;
    QVector<ClickZone> m_zones;

    QVector<WeaponOffer> m_weaponOffers;
    QVector<SupplyOffer> m_supplyOffers;
    QVector<ShipUpgradeOffer> m_shipUpgrades;

    int m_page = 0;
    int m_selectedWeaponIndex = 0;
    int m_selectedShipUpgradeIndex = 0;
    int m_shipUpgradeScroll = 0;
    QString m_noticeTitle;
    QString m_noticeBody;

    QPixmap m_seaBackground;
    QPixmap m_topStatusBar;
    QPixmap m_windowPanel;
    QPixmap m_titlePlaque;
    QPixmap m_contentPanel;
    QPixmap m_sidePanel;
    QPixmap m_infoParchment;
    QPixmap m_navNormal;
    QPixmap m_navHover;
    QPixmap m_navSelected;
    QPixmap m_navDisabled;
    QPixmap m_cardNormal;
    QPixmap m_cardHover;
    QPixmap m_cardSelected;
    QPixmap m_cardDisabled;
    QPixmap m_rowNormal;
    QPixmap m_rowHover;
    QPixmap m_rowSelected;
    QPixmap m_rowDisabled;
    QPixmap m_buttonGreen;
    QPixmap m_buttonBlue;
    QPixmap m_buttonRed;
    QPixmap m_buttonGold;
    QPixmap m_buttonGray;
    QPixmap m_priceBadge;
    QPixmap m_coinIcon;
    QPixmap m_shopkeeper;
    QPixmap m_noticeBoard;
    QPixmap m_noticeButton;
    QPixmap m_noticeIcon;

    QPixmap m_iconRod;
    QPixmap m_iconNet;
    QPixmap m_iconHarpoon;
    QPixmap m_iconPistol;
    QPixmap m_iconShotgun;
    QPixmap m_iconFood;
    QPixmap m_iconRepair1;
    QPixmap m_iconRepair2;
    QPixmap m_iconRepair3;
    QPixmap m_iconEmergencyRepair;
    QPixmap m_navIconEquipment;
    QPixmap m_navIconSupply;
    QPixmap m_navIconUpgrade;
    QPixmap m_navIconShip;
    QPixmap m_navIconBackpack;
    QPixmap m_navIconExit;
};
