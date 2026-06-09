#pragma once
#include <vector>
#include <QElapsedTimer>
#include "Player.h"
#include "Fish.h"
#include "Obstacle.h"
#include "Enemy.h"
#include "Boss.h"
#include "WaveSystem.h"
#include "WeatherSystem.h"
#include "FileManager.h"
#include "Weapon.h"
#include "InventorySystem.h"

class GameManager {
public:
    GameManager();
    ~GameManager();

    void update();
    void spawnFish();
    void spawnObstacles();
    void spawnShark();
    void spawnSwordfish();
    void spawnOctopus();
    void spawnElectricRay();
    void spawnPoisonJellyfish();
    void spawnBoss(int stage);
    void checkCollisions();
    void clearStageEntities();
    void resetStageRuntime();

    // 替换原本旧版本的 attackNearest，接入最新战斗判定
    bool attackAt(int targetX, int targetY, class Weapon* weapon);
    bool canAttemptAttack(const class Weapon* weapon) const;
    // 新增：触发主角 E 键震荡波效果
    void triggerShockWave();

    void saveAndQuit();
    void loadSave();
    bool isBossDefeated();
    std::vector<QRectF> terrainColliders() const;

    // 辅助函数取Player坐标
    int playerX() const { return (int)Player::instance().worldPos().x(); }
    int playerY() const { return (int)Player::instance().worldPos().y(); }

    std::vector<Fish*>      fish;
    std::vector<Shark*>     sharks;
    std::vector<Swordfish*> swordfishes;
    std::vector<Octopus*>   octopuses;
    std::vector<Enemy*>     specialEnemies;
    Boss* boss = nullptr;

    FileManager fileManager;

    int stage = 1;
    int killCount = 0;
    bool bossSpawned = false;
    bool gameOver = false;
    bool stageClear = false;
    bool victory = false;
    int gameTimer = 0;
    int cameraX = 0;

    bool lightningWarningActive = false;
    bool lightningStrikeActive = false;
    QPointF lightningTarget;
    int lightningWarningFrames = 0;
    int lightningStrikeFrames = 0;

private:
    int stageBossTriggerX() const;
    void applyStageConfig();
    void updateLightningHazard(Player& player);
    void recordVisibleEnemyDiscoveries();
    void resolveEntitySolids();
    int spawnTimer = 0;
    qreal m_deltaTime = 0.016;
    QElapsedTimer m_attackCooldown;
    bool m_enemyDiscoveryRecorded[5] = {};
};
