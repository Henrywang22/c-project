#include "GameManager.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "GameConfig.h"

GameManager::GameManager()
{
    ObstacleManager::instance().generateLevel(stage);
    for (int i = 0; i < 5; i++) spawnFish();
    m_attackCooldown.start();
}

GameManager::~GameManager()
{
    if (boss) delete boss;
    for (auto f : fish)        delete f;
    for (auto s : sharks)      delete s;
    for (auto s : swordfishes) delete s;
    for (auto o : octopuses)   delete o;
    ObstacleManager::instance().clear();
}

void GameManager::update()
{
    if (gameOver || victory) return;

    Player& p = Player::instance();

    WaveSystem::instance().update(m_deltaTime);
    WeatherSystem::instance().update(m_deltaTime);
    p.update(m_deltaTime);

    if (p.isDead()) { gameOver = true; return; }

    gameTimer++;
    if (gameTimer % 60 == 0) p.gameSeconds++;
    p.distance = playerX();

    fish.erase(std::remove_if(fish.begin(), fish.end(),
        [](Fish* f) {
            if (f->caught || f->escaped) { delete f; return true; }
            return false;
        }), fish.end());

    int px = playerX();
    int py = playerY();

    ObstacleManager::instance().update(m_deltaTime);

    for (auto f : fish)        f->update(px, py);
    for (auto s : sharks)      s->update(p);
    for (auto s : swordfishes) s->update(p);
    for (auto o : octopuses)   o->update(p);

    // 接入 B 模块真实接口，仅传入 p
    if (boss && boss->alive) boss->update(p);

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
    if (r < 4)      f = new Sardine(x, y);
    else if (r < 7) f = new Tuna(x, y);
    else if (r < 9) f = new DeepSeaEel(x, y);
    else            f = new GoldenFish(x, y);
    fish.push_back(f);
}

void GameManager::spawnObstacles()
{
    ObstacleManager::instance().clear();
    ObstacleManager::instance().generateLevel(stage);
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
    QPointF spawnPos(playerX() + 500, 360);
    if (stageNum >= 5)
        boss = new SirenBoss(spawnPos.x(), spawnPos.y());
    else if (stageNum >= 3)
        boss = new TaliMonsterBoss(spawnPos.x(), spawnPos.y());
    else
        boss = new FiveHeadSharkBoss(spawnPos.x(), spawnPos.y());
}

void GameManager::checkCollisions()
{
    Player& p = Player::instance();
    int px = playerX();
    int py = playerY();
    QPointF playerPos(px, py);

    // 障碍物碰撞
    const auto& obstacles = ObstacleManager::instance().obstacles();
    for (auto* o : obstacles) {
        if (!o->isVisible(playerPos)) continue;
        QRectF playerRect(px - 20, py - 10, 40, 20);
        if (o->collider().intersects(playerRect)) {
            o->onPlayerCollision(&p);
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
        if (s->state == Swordfish::CHARGE && s->collidesWithPlayer(px, py)) {
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

    // Boss 逻辑
    if (boss && boss->alive) {
        // 调用 Boss.cpp 内真实的召唤小兵接口
        boss->spawnMinions(sharks);
    }

    // Boss 死亡结算
    if (boss && !boss->alive && !stageClear) {
        p.coins += 200;
        killCount++;
        stageClear = true;
    }
}

// 采用鼠标位置决定的真实战斗判定逻辑
// 返回值：
// true  = 命中了敌人 / Boss
// false = 没命中任何敌人
bool GameManager::attackAt(int targetX, int targetY, Weapon* weapon)
{
    if (!weapon || !weapon->canAttack() || weapon->isBroken()) {
        return false;
    }

    // 攻击冷却检查
    if (m_attackCooldown.elapsed() < weapon->getAttackCooldownMs()) {
        return false;
    }

    int px = playerX();
    int py = playerY();

    int range = weapon->getRange();
    int damage = weapon->getDamage();

    // 判断鼠标点击位置是否超出武器射程
    float clickDist =
        float((targetX - px) * (targetX - px) + (targetY - py) * (targetY - py));

    if (clickDist > range * range) {
        return false;
    }

    bool isHit = false;

    // 1. 优先判定 Boss
    if (boss && boss->alive) {
        if (std::abs(boss->x - targetX) < 100 &&
            std::abs(boss->y - targetY) < 100) {

            boss->takeDamage(damage);
            isHit = true;
        }
    }

    // 2. 判定普通鲨鱼
    if (!isHit) {
        for (auto s : sharks) {
            if (!s->alive) continue;

            if (std::abs(s->x - targetX) < 40 &&
                std::abs(s->y - targetY) < 40) {

                s->takeDamage(damage);

                if (!s->alive) {
                    Player::instance().coins += s->dropValue;
                    killCount++;
                }

                isHit = true;
                break;
            }
        }
    }

    // 3. 判定剑鱼
    if (!isHit) {
        for (auto s : swordfishes) {
            if (!s->alive) continue;

            if (std::abs(s->x - targetX) < 40 &&
                std::abs(s->y - targetY) < 40) {

                s->takeDamage(damage);

                if (!s->alive) {
                    Player::instance().coins += s->dropValue;
                    killCount++;
                }

                isHit = true;
                break;
            }
        }
    }

    // 4. 判定墨鱼
    if (!isHit) {
        for (auto o : octopuses) {
            if (!o->alive) continue;

            if (std::abs(o->x - targetX) < 40 &&
                std::abs(o->y - targetY) < 40) {

                o->takeDamage(damage);

                if (!o->alive) {
                    Player::instance().coins += o->dropValue;
                    killCount++;
                }

                isHit = true;
                break;
            }
        }
    }

    // 只有命中敌人才扣除耐久并进入冷却
    // 当前版本保留你们现有设计：没命中不扣耐久，也不进入冷却。
    // 如果后期想改成“空枪也进入冷却”，可以把 restart() 移到 isHit 判断外面。
    if (isHit) {
        weapon->consumeAttackDurability();
        m_attackCooldown.restart();
    }

    return isHit;
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
    data.maxDurability = p.maxDurability;
    data.maxStamina = p.maxStamina;
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
        ObstacleManager::instance().generateLevel(stage);
    }
}

bool GameManager::isBossDefeated()
{
    return boss && !boss->alive;
}