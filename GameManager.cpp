#include "GameManager.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "GameConfig.h"

GameManager::GameManager()
{
    player = new Player(100, 360);
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
    delete player;
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

    gameTimer++;
    if (gameTimer % 60 == 0) player->gameSeconds++;

    // 清理鱼
    fish.erase(std::remove_if(fish.begin(), fish.end(),
        [](Fish* f) {
            if (f->caught || f->escaped) { delete f; return true; }
            return false;
        }), fish.end());

    waves.update(*player);
    weather.update(*player);

    for (auto f : fish)        f->update(player->x, player->y);
    for (auto s : sharks)      s->update(*player);
    for (auto s : swordfishes) s->update(*player);
    for (auto o : octopuses)   o->update(*player);
    if (boss && boss->alive)   boss->update(*player);

    for (auto o : obstacles)
        o->isVisible(player->x - cameraX, player->y);
    // 船始终居中
    cameraX = player->x - 640;
    if (cameraX < 0) cameraX = 0;

    spawnTimer++;

    int aliveFish = 0;
    for (auto f : fish)
        if (!f->caught && !f->escaped) aliveFish++;
    if (spawnTimer % 300 == 0 && aliveFish < 5) spawnFish();

    if (spawnTimer % 400 == 0 && !bossSpawned) spawnShark();
    if (spawnTimer % 500 == 0) spawnSwordfish();
    if (spawnTimer % 800 == 0 && octopuses.size() < 3) spawnOctopus();

    if (player->distance > stage * 2000 && !bossSpawned) {
        spawnBoss(stage);
        bossSpawned = true;
    }

    checkCollisions();

    if (player->durability <= 0) gameOver = true;
    if (player->stamina <= 0)    player->speed = 1;
    if (stage > 5) victory = true;
}

void GameManager::spawnFish()
{
    int x = player->x + 300 + rand() % 600;
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
    int count = 6 + stage * 2;
    for (int i = 0; i < count; i++) {
        // 最近的障碍离玩家至少800距离，给足反应空间
        int x = player->x + 800 + rand() % (stage * 800 + 1200);
        int y = 80 + rand() % 580;
        if (rand() % 3 == 0)
            obstacles.push_back(new Whirlpool(x, y));
        else
            obstacles.push_back(new Reef(x, y));
    }
}

void GameManager::spawnShark()
{
    int x = player->x + 200 + rand() % 150;
    int y = 80 + rand() % 580;
    sharks.push_back(new Shark(x, y));
}

void GameManager::spawnSwordfish()
{
    int x = player->x + 300 + rand() % 400;
    int y = 80 + rand() % 580;
    swordfishes.push_back(new Swordfish(x, y));
}

void GameManager::spawnOctopus()
{
    int x = player->x + 300 + rand() % 400;
    int y = 80 + rand() % 580;
    octopuses.push_back(new Octopus(x, y));
}

void GameManager::spawnBoss(int stageNum)
{
    if (boss) { delete boss; boss = nullptr; }
    int x = player->x + 500;
    int y = 360;
    boss = new Boss(x, y);
}

void GameManager::checkCollisions()
{
    // 障碍物
    for (auto o : obstacles) {
        if (!o->visible) continue;
        int dx = player->x - o->x;
        int dy = player->y - o->y;
        int dist2 = dx * dx + dy * dy;
        int r = o->size + 20;
        if (dist2 < r * r) {
            o->triggerEffect(*player);
            if (o->type == REEF) {
                Reef* reef = static_cast<Reef*>(o);
                reef->applyRebound(*player, (float)player->speed);
            }
        }
    }

    // 普通鲨鱼
    for (auto s : sharks) {
        if (!s->alive) continue;
        if (s->collidesWithPlayer(player->x, player->y)) {
            s->attackTimer++;
            if (s->attackTimer >= 60) {
                player->durability -= s->attack;
                if (player->durability < 0) player->durability = 0;
                s->attackTimer = 0;
            }
        }
        else {
            s->attackTimer = 0;
        }
    }

    // 剑鱼冲撞伤害
    for (auto s : swordfishes) {
        if (!s->alive) continue;
        if (s->state == Swordfish::CHARGE &&
            s->collidesWithPlayer(player->x, player->y)) {
            player->durability -= s->attack;
            if (player->durability < 0) player->durability = 0;
            s->state = Swordfish::IDLE;
        }
    }

    // 墨鱼接触
    for (auto o : octopuses) {
        if (!o->alive || o->isInvisible) continue;
        if (o->collidesWithPlayer(player->x, player->y)) {
            o->contactTimer++;
            if (o->contactTimer >= 30)
                player->visionReduced = true;
        }
    }

    // Boss
    if (boss && boss->alive) {
        if (boss->collidesWithPlayer(player->x, player->y)) {
            boss->attackTimer++;
            if (boss->attackTimer >= 60) {
                player->durability -= boss->attack;
                if (player->durability < 0) player->durability = 0;
                boss->attackTimer = 0;
            }
        }
        else {
            boss->attackTimer = 0;
        }
        // Boss二阶段召唤
        if (boss->state == Boss::PHASE2 && !boss->minionSpawned)
            boss->spawnMinions(sharks);

        if (boss->hp <= 0) {
            boss->alive = false;
            player->coins += boss->dropValue;
            killCount++;
            stageClear = true;
        }
    }
}

void GameManager::attackNearest(int damage, int range)
{
    if (currentWeapon && currentWeapon->durability <= 0) return;
    int actualDamage = currentWeapon ? currentWeapon->fire() : damage;

    // 优先攻击Boss
    if (boss && boss->alive) {
        float dx = (float)(player->x - boss->x);
        float dy = (float)(player->y - boss->y);
        if (dx * dx + dy * dy < (float)(range * range)) {
            boss->hp -= actualDamage;
            return;
        }
    }

    // 攻击最近的普通鲨鱼
    Shark* nearest = nullptr;
    float minDist = (float)(range * range);
    for (auto s : sharks) {
        if (!s->alive) continue;
        float dx = (float)(player->x - s->x);
        float dy = (float)(player->y - s->y);
        float dist = dx * dx + dy * dy;
        if (dist < minDist) { minDist = dist; nearest = s; }
    }

    // 攻击最近的剑鱼
    for (auto s : swordfishes) {
        if (!s->alive) continue;
        float dx = (float)(player->x - s->x);
        float dy = (float)(player->y - s->y);
        float dist = dx * dx + dy * dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = nullptr;
            s->hp -= actualDamage;
            if (s->hp <= 0) {
                s->alive = false;
                player->coins += s->dropValue;
                killCount++;
            }
            return;
        }
    }

    if (nearest) {
        nearest->hp -= actualDamage;
        if (nearest->hp <= 0) {
            nearest->alive = false;
            player->coins += nearest->dropValue;
            killCount++;
        }
    }
}

void GameManager::saveAndQuit()
{
    SaveData data;
    data.stage = stage;
    data.distance = player->distance;
    data.coins = player->coins;
    data.durability = player->durability;
    data.stamina = player->stamina;
    data.fishCaught = player->fishCaught;
    data.fishTotalValue = player->fishTotalValue;
    data.gameSeconds = player->gameSeconds;
    data.isDead = false;
    fileManager.saveGame(data);
}

void GameManager::loadSave()
{
    SaveData data;
    if (fileManager.loadGame(data) && !data.isDead) {
        stage = data.stage;
        player->distance = data.distance;
        player->coins = data.coins;
        player->durability = data.durability;
        player->stamina = data.stamina;
        player->fishCaught = data.fishCaught;
        player->fishTotalValue = data.fishTotalValue;
        player->gameSeconds = data.gameSeconds;
    }
}

bool GameManager::isBossDefeated()
{
    return boss && !boss->alive;
}
//.