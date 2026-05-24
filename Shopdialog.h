#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include "Player.h"
#include "Weapon.h"
#include "Item.h"
#include "ItemFactory.h"
#include "GameConfig.h"

// ============================================================
// ShopDialog — 过关后弹出的商店界面
// 支持购买消耗品、武器、属性升级
// ============================================================
class ShopDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShopDialog(QWidget* parent = nullptr);
    ~ShopDialog();

private:
    // UI构建
    void setupUI();
    QWidget* makeConsumableSection();
    QWidget* makeWeaponSection();
    QWidget* makeUpgradeSection();

    // 购买逻辑
    void buyItem(Item* item);
    void buyWeapon(Weapon* weapon);
    void updateCoinsLabel();

    // UI元素
    QLabel* m_coinsLabel = nullptr;
};