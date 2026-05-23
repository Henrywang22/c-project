#include "GameManager.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "GameConfig.h"

GameManager::GameManager()
{
    currentWeapon = new Harpoon(
        "铁制鱼叉",
        Config::PRICE_HARPOON_T1,
        Config::DMG_HARPOON_T1,
        Config::DUR_HARPOON_T1,
        Config::RANGE_HARPOON,
        Config::CONS_HARPOON);

    spawnObstacles();
    for (int i = 0; i < 5; i++) spawnFish();
}

GameManager::~GameManager()
{
    delete currentWeapon;
    if (boss) delete boss;
    for (auto f : fish)        delete f;
    for (auto o : obstacles)   delete o;
    for (auto s : sharks)      delete s;
    for (auto s : swordfishes) delete s;
    for (auto o : octopuses)   delete o;
}

void GameManager::update()
{
    if (gameOver || victory) return;

    Player& p = Player::instance();

    // 更新天气和海浪
    WaveSystem::instance().update(m_deltaTime);
    WeatherSystem::instance().update(m_deltaTime);

    // 更新Player
    p.update(m_deltaTime);

    if (p.isDead()) { gameOver = true; return; }

    gameTimer++;
    if (gameTimer % 60 == 0) p.gameSeconds++;

    // 更新距离（用x坐标）
    p.distance = playerX();

    // 清理鱼
    fish.erase(std::remove_if(fish.begin(), fish.end(),
        [](Fish* f) {
            if (f->caught || f->escaped) { delete f; return true; }
            return false;
        }), fish.end());

    int px = playerX();
    int py = playerY();

    for (auto f : fish)        f->update(px, py);
    for (auto s : sharks)      s->update(px, py);
    for (auto s : swordfishes) s->update(px, py);
    for (auto o : octopuses)   o->update(px, py);
    if (boss && boss->alive)   boss->update(px, py);

    cameraX = px - 640;
    if (cameraX < 0) cameraX = 0;

    spawnTimer++;

    int aliveFish = 0;
    for (auto f : fish)
        if (!f->caught && !f->escaped) aliveFish++;
    if (spawnTimer % 300 == 0 && aliveFish < 5) spawnFish();

    if (spawnTimer % 400 == 0 && !bossSpawned) spawnShark();
    if (spawnTimer % 500 == 0) spawnSwordfish();
    if (spawnTimer % 800 == 0 && (int)octopuses.size() < 3) spawnOctopus();

    if (px > stage * 2000 && !bossSpawned) {
        spawnBoss(stage);
        bossSpawned = true;
    }

    checkCollisions();

    if (p.durability() <= 0) gameOver = true;
    if (stage > 5) victory = true;
}

void GameManager::spawnFish()
{
    int px = playerX();
    int x = px + 300 + rand() % 600;
    int y = 80 + rand() % 580;
    int r = rand() % 10;
    Fish* f;
    if (r < 4) f = new Sardine(x, y);
    else if (r < 7) f = new Tuna(x, y);
    else if (r < 9) f = new DeepSeaEel(x, y);
    else            f = new GoldenFish(x, y);
    fish.push_back(f);
}

void GameManager::spawnObstacles()
{
    int px = playerX();
    int count = 6 + stage * 2;
    for (int i = 0; i < count; i++) {
        int x = px + 800 + rand() % (stage * 800 + 1200);
        int y = 80 + rand() % 580;
        if (rand() % 3 == 0)
            obstacles.push_back(new Whirlpool(x, y));
        else
            obstacles.push_back(new Reef(x, y));
    }
}

void GameManager::spawnShark()
{
    int px = playerX();
    int x = px + 200 + rand() % 150;
    int y = 80 + rand() % 580;
    sharks.push_back(new Shark(x, y));
}

void GameManager::spawnSwordfish()
{
    int px = playerX();
    int x = px + 300 + rand() % 400;
    int y = 80 + rand() % 580;
    swordfishes.push_back(new Swordfish(x, y));
}

void GameManager::spawnOctopus()
{
    int px = playerX();
    int x = px + 300 + rand() % 400;
    int y = 80 + rand() % 580;
    octopuses.push_back(new Octopus(x, y));
}

void GameManager::spawnBoss(int stageNum)
{
    if (boss) { delete boss; boss = nullptr; }
    int x = playerX() + 500;
    int y = 360;
    boss = new Boss(x, y);
}

void GameManager::checkCollisions()
{
    Player& p = Player::instance();
    int px = playerX();
    int py = playerY();

    // 障碍物
    for (auto o : obstacles) {
        if (!o->visible) continue;
        int dx = px - o->x;
        int dy = py - o->y;
        int dist2 = dx * dx + dy * dy;
        int r = o->size + 20;
        if (dist2 < r * r) {
            if (o->type == REEF) {
                p.takeDurabilityDamage(10);
                float len = sqrt((float)(dx * dx + dy * dy));
                if (len > 0)
                    p.applyRebound(QPointF(dx / len, dy / len));
            }
            else {
                p.applySpeedReduction(0.5);
            }
        }
    }

    // 普通鲨鱼
    for (auto s : sharks) {
        if (!s->alive) continue;
        if (s->collidesWithPlayer(px, py)) {
            s->attackTimer++;
            if (s->attackTimer >= 60) {
                p.takeDurabilityDamage(s->attack);
                s->attackTimer = 0;
            }
        }
        else {
            s->attackTimer = 0;
        }
    }

    // 剑鱼冲撞
    for (auto s : swordfishes) {
        if (!s->alive) continue;
        if (s->state == Swordfish::CHARGE &&
            s->collidesWithPlayer(px, py)) {
            p.takeDurabilityDamage(s->attack);
            s->state = Swordfish::IDLE;
        }
    }

    // 墨鱼接触
    for (auto o : octopuses) {
        if (!o->alive || o->isInvisible) continue;
        if (o->collidesWithPlayer(px, py)) {
            o->contactTimer++;
            if (o->contactTimer >= 30)
                p.visionReduced = true;
        }
    }

    // Boss
    if (boss && boss->alive) {
        if (boss->collidesWithPlayer(px, py)) {
            boss->attackTimer++;
            if (boss->attackTimer >= 60) {
                p.takeDurabilityDamage(boss->attack);
                boss->attackTimer = 0;
            }
        }
        else {
            boss->attackTimer = 0;
        }

        if (boss->state == Boss::PHASE2 && !boss->minionSpawned)
            boss->spawnMinions(sharks);

        if (boss->hp <= 0) {
            boss->alive = false;
            p.coins += boss->dropValue; // Boss掉落金币
            killCount++;
            stageClear = true;
        }
    }
}

void GameManager::attackNearest(int damage, int range)
{
    if (currentWeapon && currentWeapon->isBroken()) return;
    int actualDamage = currentWeapon ? currentWeapon->fire() : damage;

    int px = playerX();
    int py = playerY();

    if (boss && boss->alive) {
        float dx = (float)(px - boss->x);
        float dy = (float)(py - boss->y);
        if (dx * dx + dy * dy < (float)(range * range)) {
            boss->hp -= actualDamage;
            return;
        }
    }

    Shark* nearest = nullptr;
    float minDist = (float)(range * range);
    for (auto s : sharks) {
        if (!s->alive) continue;
        float dx = (float)(px - s->x);
        float dy = (float)(py - s->y);
        float dist = dx * dx + dy * dy;
        if (dist < minDist) { minDist = dist; nearest = s; }
    }

    for (auto s : swordfishes) {
        if (!s->alive) continue;
        float dx = (float)(px - s->x);
        float dy = (float)(py - s->y);
        float dist = dx * dx + dy * dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = nullptr;
            s->hp -= actualDamage;
            if (s->hp <= 0) {
                s->alive = false;
                Player::instance().coins += s->dropValue;
                killCount++;
            }
            return;
        }
    }

    if (nearest) {
        nearest->hp -= actualDamage;
        if (nearest->hp <= 0) {
            nearest->alive = false;
            Player::instance().coins += nearest->dropValue;
            killCount++;
        }
    }
}

void GameManager::saveAndQuit()
{
    Player& p = Player::instance();
    SaveData data;
    data.stage = stage;
    data.distance = p.distance;
    data.coins = p.coins;
    data.durability = p.durability();
    data.stamina = p.stamina();
    data.fishCaught = p.fishCaught;
    data.fishTotalValue = p.fishTotalValue;
    data.gameSeconds = p.gameSeconds;
    data.isDead = false;
    fileManager.saveGame(data);
}

void GameManager::loadSave()
{
    SaveData data;
    if (fileManager.loadGame(data) && !data.isDead) {
        stage = data.stage;
        Player& p = Player::instance();
        p.coins = data.coins;
        p.fishCaught = data.fishCaught;
        p.fishTotalValue = data.fishTotalValue;
        p.gameSeconds = data.gameSeconds;
    }
}

bool GameManager::isBossDefeated()
{
    return boss && !boss->alive;
}