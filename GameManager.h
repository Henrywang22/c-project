#pragma once
#include <vector>
#include "Player.h"
#include "Fish.h"
#include "Obstacle.h"
#include "Enemy.h"
#include "Boss.h"
#include "WaveSystem.h"
#include "WeatherSystem.h"
#include "FileManager.h"
#include "Weapon.h"

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
    void spawnBoss(int stage);
    void checkCollisions();
    void attackNearest(int damage, int range);
    void saveAndQuit();
    void loadSave();
    bool isBossDefeated();

    // Player改为引用C同学的单例
    Player& getPlayer() { return Player::instance(); }

    std::vector<Fish*>      fish;
    std::vector<Obstacle*>  obstacles;
    std::vector<Shark*>     sharks;
    std::vector<Swordfish*> swordfishes;
    std::vector<Octopus*>   octopuses;
    Boss* boss = nullptr;

    FileManager fileManager;
    Weapon* currentWeapon = nullptr;

    int stage = 1;
    int killCount = 0;
    bool bossSpawned = false;
    bool gameOver = false;
    bool stageClear = false;
    bool victory = false;
    int gameTimer = 0;
    int cameraX = 0;

    // 辅助函数：从Player单例取坐标
    int playerX() const { return (int)Player::instance().worldPos().x(); }
    int playerY() const { return (int)Player::instance().worldPos().y(); }

private:
    int spawnTimer = 0;
    qreal m_deltaTime = 0.016; // 固定16ms帧时间
};