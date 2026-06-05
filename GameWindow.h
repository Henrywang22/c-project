#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QRect>
#include <QString>
#include <QVector>
#include "GameManager.h"

enum GameState
{
    STATE_INTRO,    // 介绍/说明画面
    STATE_MENU,     // 主菜单
    STATE_STAGE_START,
    STATE_STAGE_CLEAR,
    STATE_PLAYING,  // 游戏中
    STATE_PAUSED,   // 暂停
    STATE_DEFEAT,   // 失败
    STATE_VICTORY   // 胜利
};

class GameWindow : public QWidget {
    Q_OBJECT

public:
    GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override; // 新增：鼠标点击事件

private slots:
    void gameLoop();

private:
    QTimer* timer;
    GameManager* gm;
    GameState    state = STATE_MENU;

    // 按键状态
    bool keyUp = false;
    bool keyDown = false;
    bool keyLeft = false;
    bool keyRight = false;
    bool keyShift = false;

    // 捕鱼状态
    Fish* targetFish = nullptr;
    bool  isFishing = false;
    int   fishClickCount = 0;
    int   fishTimer = 0;
    int   menuHoverIndex = -1;
    bool  promptButtonHover = false;
    bool  victoryScoreSaved = false;
    bool  testModeEnabled = false;

    enum class AttackProjectileKind {
        Bullet,
        Harpoon
    };

    struct AttackProjectileEffect {
        QPointF startWorld;
        QPointF endWorld;
        int ageMs = 0;
        int lifetimeMs = 140;
        int travelMs = 140;
        int radius = 4;
        QColor color = QColor(255, 238, 150);
        AttackProjectileKind kind = AttackProjectileKind::Bullet;
    };

    QVector<AttackProjectileEffect> attackProjectiles;

    struct HitFeedbackEffect {
        QPointF worldPos;
        int ageMs = 0;
        int lifetimeMs = 180;
    };

    struct FloatingNotice {
        QString title;
        QString body;
        int ageMs = 0;
        int lifetimeMs = 2400;
        bool active = false;
    };

    QVector<HitFeedbackEffect> hitFeedbacks;
    FloatingNotice floatingNotice;

    // 绘制函数
    void drawIntro(QPainter& p);
    void drawMenu(QPainter& p);
    void drawStageStartPrompt(QPainter& p);
    void drawStageClearPrompt(QPainter& p);
    void drawGame(QPainter& p);
    void drawSea(QPainter& p);
    void drawStageDecorations(QPainter& p);
    void drawFish(QPainter& p);
    void drawObstacles(QPainter& p);
    void drawSharks(QPainter& p);
    void drawBossHazards(QPainter& p);
    void drawPlayer(QPainter& p);
    void drawAttackProjectiles(QPainter& p);
    void drawHitFeedbacks(QPainter& p);
    void drawWaves(QPainter& p);
    void drawWeatherEffects(QPainter& p);
    void drawHUD(QPainter& p);
    void drawTestModeOverlay(QPainter& p);
    void drawFloatingNotice(QPainter& p);
    void drawFishingHUD(QPainter& p);
    void drawPaused(QPainter& p);
    void drawDefeat(QPainter& p);
    void drawVictory(QPainter& p);

    void updateFishing();
    void updateAttackProjectiles();
    void updateHitFeedbacks();
    void updateFloatingNotice();
    Fish* nearestFishInWeaponRange(const Weapon* weapon) const;
    void resetFishingState(bool releaseTarget = true);
    Config::FishingResult calibrationFishingResult() const;
    qreal calibrationMarkerRatio() const;
    qreal calibrationWindowSize(bool perfect) const;
    void finishFishing(Config::FishingResult result);
    void spawnGunProjectiles(const QPointF& targetWorld, const Weapon* weapon);
    void spawnHarpoonProjectile(const QPointF& targetWorld, const Weapon* weapon);
    void spawnHitFeedback(const QPointF& worldPos);
    void showFloatingNotice(const QString& title, const QString& body);
    void notifyWeaponBrokenIfNeeded(const Weapon* weapon, bool wasBroken);
    bool isGunWeapon(const Weapon* weapon) const;
    bool isHarpoonWeapon(const Weapon* weapon) const;
    void saveVictoryHighScore();
    void openShop();
    void openTestModeShop();
    void openBackpack();
    void openEncyclopedia();
    bool useQuickItemSlot(int hotbarIndex);
    void toggleTestMode();
    void applyTestModeBenefits();
    void startNewGame();
    void continueGame();
    void confirmStagePrompt();
    QRect menuButtonRect(int index) const;
    int menuButtonAt(const QPoint& pos) const;
    QRect stagePromptButtonRect() const;

    // 图片资源
    QPixmap imgSardine;
    QPixmap imgTuna;
    QPixmap imgEel;
    QPixmap imgGolden;
    QPixmap imgAnchovy;
    QPixmap imgClownfish;
    QPixmap imgMackerel;
    QPixmap imgSeaBream;
    QPixmap imgLanternfish;
    QPixmap imgGrouper;
    QPixmap imgKoi;
    QPixmap imgCrystalFish;
    QPixmap imgShark;
    QPixmap imgSwordfish;
    QPixmap imgOctopus;
    QPixmap imgBoat;
    QPixmap imgSeaBackground;
    QPixmap imgWaveOverlay;
    QPixmap imgObstacleReef;
    QPixmap imgObstacleWhirlpool;
    QPixmap imgStormLightning;
    QPixmap imgHarpoonProjectile;
    QPixmap imgWoodNoticeBoard;
    QPixmap imgWoodNoticeButton;
    QPixmap imgNoticeIconInfo;
    QPixmap imgRainCluster;
    QPixmap imgLightningWarningRing;
    QPixmap imgBossWarningRing;
    QPixmap imgBossWarningRect;
    QPixmap imgShockwaveRing;
    QPixmap imgWeaponRangeRing;
    QPixmap imgHitSpark;
    QPixmap imgMuzzleFlash;
    QPixmap imgStageDecor[6];
    QPixmap imgPlayerMove[5][4];
    QPixmap imgPlayerBoost[5][4];
    QPixmap imgMenuBackground;
    QPixmap imgMenuPanel;
    QPixmap imgMenuTitlePlaque;
    QPixmap imgMenuRecordPanel;
    QPixmap imgMenuButtonNormal;
    QPixmap imgMenuButtonHover;
    QPixmap imgMenuButtonDisabled;
    QPixmap imgStageStartPrompt;
    QPixmap imgStageClearPrompt;
    QPixmap imgHudTopStatusBar;
    QPixmap imgHudHealthFill;
    QPixmap imgHudStaminaFill;
    QPixmap imgHudEquipmentPanel;
    QPixmap imgHudHotbar;
    QPixmap imgHudMinimapPanel;
    QPixmap imgHudLogPanel;
    QPixmap imgHudSlotNormal;
    QPixmap imgHudSlotSelected;
    QPixmap imgHudIconHeart;
    QPixmap imgHudIconLightning;
    QPixmap imgHudIconCoin;
    QPixmap imgHudIconFish;
    QPixmap imgHudIconSun;
    QPixmap imgHudIconCompass;
    QPixmap imgFishingQtePanel;
    QPixmap imgIconWeaponRod;
    QPixmap imgIconWeaponNet;
    QPixmap imgIconWeaponHarpoon;
    QPixmap imgIconWeaponPistol;
    QPixmap imgIconWeaponShotgun;
    QPixmap imgIconItemFood;
    QPixmap imgIconItemRepairT1;
    QPixmap imgIconItemRepairT2;
    QPixmap imgIconItemRepairT3;
    QPixmap imgIconItemEmergencyRepair;
};
