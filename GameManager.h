#pragma once
#include <vector>
#include "Player.h"
#include "Fish.h"
#include "Obstacle.h"
#include "NormalEnemy.h"
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

    Player* player;
    std::vector<Fish*>     fish;
    std::vector<Obstacle*> obstacles;
    std::vector<Shark*>    sharks;
    std::vector<Swordfish*> swordfishes;
    std::vector<Octopus*>  octopuses;
    Boss* boss = nullptr;

    WaveSystem    waves;
    WeatherSystem weather;
    FileManager   fileManager;
    Weapon* currentWeapon = nullptr;

    int stage = 1;
    int killCount = 0;
    bool bossSpawned = false;
    bool gameOver = false;
    bool stageClear = false;
    bool victory = false;
    int gameTimer = 0;
    int cameraX = 0;

private:
    int spawnTimer = 0;
};