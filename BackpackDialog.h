#pragma once

#include <QDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QVector>

#include "InventorySystem.h"

class Weapon;

class BackpackDialog : public QDialog
{
public:
    explicit BackpackDialog(int stage = 1, QWidget* parent = nullptr);
    ~BackpackDialog() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class Page {
        Equipment,
        Items
    };

    enum class Action {
        SwitchEquipment,
        SwitchItems,
        SelectEquipment,
        SelectItem,
        Equip,
        Use,
        Repair,
        Close
    };

    struct ClickZone {
        QRect rect;
        Action action;
        int index = -1;
    };

    struct ItemDef {
        InventoryItemType type;
        QString name;
        QString subtitle;
        QString description;
        const QPixmap* icon = nullptr;
    };

    void loadAssets();
    void drawBackground(QPainter& p);
    void drawTopStatus(QPainter& p);
    void drawMainFrame(QPainter& p);
    void drawTitle(QPainter& p);
    void drawInfoStrip(QPainter& p);
    void drawTabs(QPainter& p);
    void drawEquipmentList(QPainter& p);
    void drawItemList(QPainter& p);
    void drawEquipmentDetail(QPainter& p);
    void drawItemDetail(QPainter& p);
    void drawActions(QPainter& p);
    void drawFooterHint(QPainter& p);
    void drawRowText(QPainter& p, const QRect& row, const QPixmap& icon,
                     const QString& title, const QString& subtitle,
                     const QString& tag, const QColor& tagColor,
                     bool selected, bool disabled);
    void drawButton(QPainter& p, const QRect& rect, const QPixmap& bg,
                    const QString& text, Action action, int index,
                    bool disabled = false);
    void drawTextShadow(QPainter& p, const QRect& rect, const QString& text,
                        const QFont& font, const QColor& color,
                        int flags = Qt::AlignCenter);
    void drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& rect);
    void drawStatLine(QPainter& p, int& y, const QString& label, const QString& value);

    QVector<ItemDef> itemDefs() const;
    const ItemDef* selectedItemDef() const;
    const Weapon* selectedWeapon() const;
    Weapon* selectedWeapon();
    QPixmap weaponIcon(const QString& typeCode) const;
    int selectedItemCount() const;
    QString weaponRoleText(const Weapon* weapon) const;
    QString fishingModeText(const Weapon* weapon) const;
    QString weaponStatusText(int index, const Weapon* weapon) const;
    QColor weaponStatusColor(int index, const Weapon* weapon) const;
    bool canUseSelectedItem() const;
    bool canRepairSelectedWeapon() const;
    void switchPage(Page page);
    void selectNext(int delta);
    void handleAction(Action action, int index);
    void clampSelections();
    void addZone(const QRect& rect, Action action, int index = -1);
    const ClickZone* zoneAt(const QPoint& pos) const;
    bool isHovered(const QRect& rect) const;
    void setStatusMessage(const QString& text);

    Page m_page = Page::Equipment;
    int m_stage = 1;
    int m_selectedEquipmentIndex = 0;
    int m_selectedItemIndex = 0;
    QPoint m_mousePos = QPoint(-1, -1);
    QVector<ClickZone> m_zones;
    QString m_statusMessage;

    QPixmap m_seaBackground;
    QPixmap m_topStatusBar;
    QPixmap m_windowPanel;
    QPixmap m_titlePlaque;
    QPixmap m_infoStrip;
    QPixmap m_tabSelected;
    QPixmap m_tabNormal;
    QPixmap m_listPanel;
    QPixmap m_detailPanel;
    QPixmap m_detailCard;
    QPixmap m_statsSheet;
    QPixmap m_rowNormal;
    QPixmap m_rowSelected;
    QPixmap m_rowDisabled;
    QPixmap m_slotFrame;
    QPixmap m_buttonGreen;
    QPixmap m_buttonBlue;
    QPixmap m_buttonRed;
    QPixmap m_footerHint;
    QPixmap m_iconHeart;
    QPixmap m_iconLightning;
    QPixmap m_iconCoin;
    QPixmap m_iconFish;
    QPixmap m_iconSun;
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
};
