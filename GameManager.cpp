#include "GameManager.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <QLineF>
#include <QRandomGenerator>
#include "GameConfig.h"

namespace {
qreal distanceSquared(const QPointF& a, const QPointF& b)
{
    const qreal dx = a.x() - b.x();
    const qreal dy = a.y() - b.y();
    return dx * dx + dy * dy;
}

QPointF closestPointInRect(const QRectF& rect, const QPointF& point)
{
    return QPointF(
        std::clamp(point.x(), rect.left(), rect.right()),
        std::clamp(point.y(), rect.top(), rect.bottom())
    );
}

bool rectWithinRange(const QRectF& rect, const QPointF& origin, qreal range)
{
    const QPointF closest = closestPointInRect(rect, origin);
    return distanceSquared(closest, origin) <= range * range;
}

bool targetHitsEnemy(const Enemy* enemy, const QPointF& target, const QPointF& origin, qreal range)
{
    if (!enemy) return false;
    const QRectF hitbox = enemy->collider();
    return hitbox.contains(target) && rectWithinRange(hitbox, origin, range);
}

bool lineIntersectsRect(const QLineF& line, const QRectF& rect)
{
    if (rect.contains(line.p1()) || rect.contains(line.p2())) {
        return true;
    }

    const QLineF edges[] = {
        QLineF(rect.topLeft(), rect.topRight()),
        QLineF(rect.topRight(), rect.bottomRight()),
        QLineF(rect.bottomRight(), rect.bottomLeft()),
        QLineF(rect.bottomLeft(), rect.topLeft())
    };

    QPointF intersection;
    for (const auto& edge : edges) {
        if (line.intersects(edge, &intersection) == QLineF::BoundedIntersection) {
            return true;
        }
    }

    return false;
}

QPointF clampedAttackEnd(const QPointF& origin, const QPointF& target, qreal range)
{
    const qreal dx = target.x() - origin.x();
    const qreal dy = target.y() - origin.y();
    const qreal length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.001 || length <= range) {
        return target;
    }

    return QPointF(origin.x() + dx / length * range,
                   origin.y() + dy / length * range);
}

bool isGunWeapon(const Weapon* weapon)
{
    if (!weapon) return false;
    const std::string type = weapon->getTypeCode();
    return type == "Pistol" || type == "Shotgun";
}

bool isHarpoonWeapon(const Weapon* weapon)
{
    return weapon && weapon->getTypeCode() == "Harpoon";
}

bool harpoonLineHitsRect(const QLineF& line, const QRectF& rect)
{
    if (rect.isEmpty()) return false;
    const qreal pad = Config::HARPOON_PROJECTILE_HIT_PADDING;
    return lineIntersectsRect(line, rect.adjusted(-pad, -pad, pad, pad));
}

qreal harpoonHitDistance(const QLineF& line, const QRectF& rect, bool& hit)
{
    hit = false;
    if (rect.isEmpty()) return 0.0;

    const qreal pad = Config::HARPOON_PROJECTILE_HIT_PADDING;
    const QRectF hitbox = rect.adjusted(-pad, -pad, pad, pad);
    if (hitbox.contains(line.p1())) {
        hit = true;
        return 0.0;
    }

    qreal best = 1e18;
    QPointF intersection;
    const QLineF edges[] = {
        QLineF(hitbox.topLeft(), hitbox.topRight()),
        QLineF(hitbox.topRight(), hitbox.bottomRight()),
        QLineF(hitbox.bottomRight(), hitbox.bottomLeft()),
        QLineF(hitbox.bottomLeft(), hitbox.topLeft())
    };
    for (const auto& edge : edges) {
        if (line.intersects(edge, &intersection) == QLineF::BoundedIntersection) {
            best = std::min(best, QLineF(line.p1(), intersection).length());
            hit = true;
        }
    }

    if (!hit && hitbox.contains(line.p2())) {
        hit = true;
        return line.length();
    }

    return best;
}

void recordEnemyDiscovery(FileManager& fileManager, const Enemy* enemy)
{
    if (!enemy) return;

    if (dynamic_cast<const Shark*>(enemy)) {
        fileManager.markEnemyDiscovered(0, "Shark");
    }
    else if (dynamic_cast<const Swordfish*>(enemy)) {
        fileManager.markEnemyDiscovered(1, "Swordfish");
    }
    else if (dynamic_cast<const Octopus*>(enemy)) {
        fileManager.markEnemyDiscovered(2, "Octopus");
    }
}

void recordBossDiscovery(FileManager& fileManager, BossKind kind)
{
    switch (kind) {
    case BossKind::FiveHeadShark:
        fileManager.markBossDiscovered(0, "Five Head Shark");
        break;
    case BossKind::TaliMonster:
        fileManager.markBossDiscovered(1, "Tali Monster");
        break;
    case BossKind::Siren:
        fileManager.markBossDiscovered(2, "Siren");
        break;
    }
}

struct SolidBody {
    Fish* fish = nullptr;
    Enemy* enemy = nullptr;
    qreal mass = 1.0;
};

QRectF solidRect(const SolidBody& body)
{
    if (body.fish) return body.fish->collider();
    if (body.enemy) return body.enemy->collider();
    return QRectF();
}

QPointF solidPosition(const SolidBody& body)
{
    if (body.fish) return body.fish->position();
    if (body.enemy) return body.enemy->position();
    return QPointF();
}

void setSolidPosition(const SolidBody& body, const QPointF& pos)
{
    if (body.fish) {
        body.fish->setPosition(pos);
    }
    else if (body.enemy) {
        body.enemy->setPosition(pos);
    }
}

QPointF clampedSolidPosition(const SolidBody& body, QPointF pos)
{
    const QRectF rect = solidRect(body);
    const qreal halfW = rect.width() / 2.0;
    const qreal halfH = rect.height() / 2.0;
    pos.setX(std::clamp(pos.x(), halfW, static_cast<qreal>(Config::GameConfig::RIGHT_BORDER) - halfW));
    pos.setY(std::clamp(pos.y(),
                        static_cast<qreal>(Config::GameConfig::TOP_BORDER) + halfH,
                        static_cast<qreal>(Config::GameConfig::BOTTOM_BORDER) - halfH));
    return pos;
}

void moveSolidBy(const SolidBody& body, const QPointF& delta)
{
    if (std::abs(delta.x()) <= 0.001 && std::abs(delta.y()) <= 0.001) return;
    setSolidPosition(body, clampedSolidPosition(body, solidPosition(body) + delta));
}

void applyHitKnockback(Enemy* enemy, const QPointF& origin, qreal strength)
{
    if (!enemy || !enemy->alive || strength <= 0.0) return;

    QPointF dir = enemy->position() - origin;
    qreal length = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (length <= 0.001) {
        dir = QPointF(1.0, 0.0);
        length = 1.0;
    }

    const QRectF hitbox = enemy->collider();
    const qreal halfW = hitbox.width() / 2.0;
    const qreal halfH = hitbox.height() / 2.0;
    QPointF next = enemy->position() + QPointF(dir.x() / length * strength, dir.y() / length * strength);
    next.setX(std::clamp(next.x(), halfW, static_cast<qreal>(Config::GameConfig::RIGHT_BORDER) - halfW));
    next.setY(std::clamp(next.y(),
                         static_cast<qreal>(Config::GameConfig::TOP_BORDER) + halfH,
                         static_cast<qreal>(Config::GameConfig::BOTTOM_BORDER) - halfH));
    enemy->setPosition(next);
}

QPointF minimumSeparationVector(const QRectF& a, const QRectF& b)
{
    if (!a.intersects(b)) return QPointF();

    const qreal overlapX = std::min(a.right(), b.right()) - std::max(a.left(), b.left());
    const qreal overlapY = std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top());
    if (overlapX <= 0.0 || overlapY <= 0.0) return QPointF();

    const QPointF ac = a.center();
    const QPointF bc = b.center();
    if (overlapX < overlapY) {
        const qreal dir = ac.x() < bc.x() ? -1.0 : 1.0;
        return QPointF(dir * (overlapX + 0.75), 0.0);
    }

    const qreal dir = ac.y() < bc.y() ? -1.0 : 1.0;
    return QPointF(0.0, dir * (overlapY + 0.75));
}

void separateSolidPair(const SolidBody& a, const SolidBody& b)
{
    const QRectF rectA = solidRect(a);
    const QRectF rectB = solidRect(b);
    const QPointF pushA = minimumSeparationVector(rectA, rectB);
    if (pushA.isNull()) return;

    const qreal totalMass = qMax<qreal>(0.001, a.mass + b.mass);
    moveSolidBy(a, pushA * (b.mass / totalMass));
    moveSolidBy(b, QPointF(-pushA.x(), -pushA.y()) * (a.mass / totalMass));
}

void separateSolidFromObstacle(const SolidBody& body, const QRectF& obstacleRect)
{
    const QPointF push = minimumSeparationVector(solidRect(body), obstacleRect);
    if (push.isNull()) return;
    moveSolidBy(body, push);
    if (body.fish) {
        if (std::abs(push.x()) > 0.001) body.fish->vx = -body.fish->vx;
        if (std::abs(push.y()) > 0.001) body.fish->vy = -body.fish->vy;
    }
}

void applyEnemyAvoidanceNudge(Enemy* enemy, const QRectF& solidRect, const QPointF& playerPos)
{
    if (!enemy || !enemy->alive || solidRect.isEmpty()) return;

    const QRectF influence = solidRect.adjusted(-105.0, -82.0, 105.0, 82.0);
    const QPointF pos = enemy->position();
    if (!influence.contains(pos)) return;

    QPointF toPlayer = playerPos - pos;
    qreal toPlayerLen = std::sqrt(toPlayer.x() * toPlayer.x() + toPlayer.y() * toPlayer.y());
    if (toPlayerLen <= 0.001) {
        toPlayer = QPointF(1.0, 0.0);
        toPlayerLen = 1.0;
    }

    QPointF tangent(-toPlayer.y() / toPlayerLen, toPlayer.x() / toPlayerLen);
    QPointF away = pos - solidRect.center();
    qreal awayLen = std::sqrt(away.x() * away.x() + away.y() * away.y());
    if (awayLen <= 0.001) {
        away = QPointF(tangent.y(), -tangent.x());
        awayLen = 1.0;
    }

    const qreal dot = tangent.x() * away.x() + tangent.y() * away.y();
    if (dot < 0.0) {
        tangent = -tangent;
    }

    const QPointF nudge(
        tangent.x() * 1.65 + away.x() / awayLen * 0.55,
        tangent.y() * 1.65 + away.y() / awayLen * 0.55
    );
    enemy->setPosition(pos + nudge);
}

std::vector<QLineF> gunAttackLines(const Weapon* weapon, const QPointF& origin, const QPointF& target)
{
    std::vector<QLineF> lines;
    if (!weapon) return lines;

    const QPointF end = clampedAttackEnd(origin, target, weapon->getRange());
    const qreal dx = end.x() - origin.x();
    const qreal dy = end.y() - origin.y();
    const qreal length = std::sqrt(dx * dx + dy * dy);

    if (length <= 0.001) {
        lines.emplace_back(origin, QPointF(origin.x() + weapon->getRange(), origin.y()));
        return lines;
    }

    const std::string type = weapon->getTypeCode();
    if (type == "Shotgun") {
        const qreal sideX = -dy / length;
        const qreal sideY = dx / length;
        const int offsets[] = { -34, -17, 0, 17, 34 };
        for (int offset : offsets) {
            lines.emplace_back(
                origin,
                QPointF(end.x() + sideX * offset, end.y() + sideY * offset)
            );
        }
        return lines;
    }

    lines.emplace_back(origin, end);
    return lines;
}
}

GameManager::GameManager()
{
    WaveSystem::instance().reset();
    WeatherSystem::instance().reset();
    applyStageConfig();
    ObstacleManager::instance().generateLevel(
        stage,
        Config::GameConfig::stageConfig(stage).reefCount,
        Config::GameConfig::stageConfig(stage).whirlpoolCount
    );
    for (int i = 0; i < Config::GameConfig::stageConfig(stage).initialFish; i++) spawnFish();
    m_attackCooldown.start();
}

GameManager::~GameManager()
{
    clearStageEntities();
    ObstacleManager::instance().clear();
}

void GameManager::update()
{
    if (gameOver || victory) return;

    Player& p = Player::instance();

    WaveSystem::instance().update(m_deltaTime);
    WeatherSystem::instance().update(m_deltaTime);
    p.update(m_deltaTime);
    updateLightningHazard(p);

    if (p.isDead()) { gameOver = true; return; }

    gameTimer++;
    if (gameTimer % 60 == 0) p.gameSeconds++;
    p.distance = playerX();

    fish.erase(std::remove_if(fish.begin(), fish.end(),
        [](Fish* f) {
            if (f->caught || f->escaped) { delete f; return true; }
            return false;
        }), fish.end());

    auto eraseDead = [](auto& enemies) {
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](auto* enemy) {
                if (!enemy->alive) { delete enemy; return true; }
                return false;
            }), enemies.end());
    };

    eraseDead(sharks);
    eraseDead(swordfishes);
    eraseDead(octopuses);

    int px = playerX();
    int py = playerY();

    ObstacleManager::instance().update(m_deltaTime);

    for (auto f : fish)        f->update(px, py);
    for (auto s : sharks)      s->update(p);
    for (auto s : swordfishes) s->update(p);
    for (auto o : octopuses)   o->update(p);

    const auto terrain = terrainColliders();
    auto nudgeEnemyAroundSolids = [&](Enemy* enemy) {
        if (!enemy || !enemy->alive) return;
        for (auto* obstacle : ObstacleManager::instance().obstacles()) {
            if (obstacle) {
                applyEnemyAvoidanceNudge(enemy, obstacle->collider(), p.worldPos());
            }
        }
        for (const QRectF& terrainRect : terrain) {
            applyEnemyAvoidanceNudge(enemy, terrainRect, p.worldPos());
        }
    };
    for (auto s : sharks)      nudgeEnemyAroundSolids(s);
    for (auto s : swordfishes) nudgeEnemyAroundSolids(s);
    for (auto o : octopuses)   nudgeEnemyAroundSolids(o);

    // 接入 B 模块真实接口，仅传入 p
    if (boss && boss->alive) boss->update(p);

    cameraX = px - 640;
    if (cameraX < 0) cameraX = 0;

    spawnTimer++;

    int aliveFish = 0;
    for (auto f : fish)
        if (!f->caught && !f->escaped) aliveFish++;
    const auto& cfg = Config::GameConfig::stageConfig(stage);
    const int fishCap = cfg.fishCap;
    const int fishInterval = qMax(1, cfg.fishSpawnInterval);
    if (spawnTimer % fishInterval == 0 && aliveFish < fishCap) spawnFish();

    if (!bossSpawned) {
        if (cfg.sharkCap > 0 && cfg.sharkSpawnInterval > 0 &&
            spawnTimer % cfg.sharkSpawnInterval == 0 &&
            static_cast<int>(sharks.size()) < cfg.sharkCap) {
            spawnShark();
        }
        if (cfg.swordfishCap > 0 && cfg.swordfishSpawnInterval > 0 &&
            spawnTimer % cfg.swordfishSpawnInterval == 0 &&
            static_cast<int>(swordfishes.size()) < cfg.swordfishCap) {
            spawnSwordfish();
        }
        if (cfg.octopusCap > 0 && cfg.octopusSpawnInterval > 0 &&
            spawnTimer % cfg.octopusSpawnInterval == 0 &&
            static_cast<int>(octopuses.size()) < cfg.octopusCap) {
            spawnOctopus();
        }
    }

    if (!stageClear && px >= stageBossTriggerX()) {
        if (cfg.hasBoss) {
            if (!bossSpawned) {
                spawnBoss(stage);
                bossSpawned = true;
            }
        }
        else {
            stageClear = true;
        }
    }

    resolveEntitySolids();

    checkCollisions();

    if (stage > Config::GameConfig::STAGE_COUNT) victory = true;
}

int GameManager::stageBossTriggerX() const
{
    int trigger = Config::GameConfig::stageConfig(stage).targetDistance;
    return std::max(0, std::min(trigger, Config::GameConfig::RIGHT_BORDER));
}

std::vector<QRectF> GameManager::terrainColliders() const
{
    struct TerrainSpec {
        int stage;
        qreal ratio;
        qreal y;
        qreal width;
        qreal height;
    };

    static const TerrainSpec specs[] = {
        {1, 0.18, 132, 330, 150},
        {1, 0.78, 640, 300, 120},

        {2, 0.16, 132, 250, 170},
        {2, 0.52, 622, 280, 130},
        {2, 0.78, 196, 240, 120},

        {3, 0.20, 612, 330, 132},
        {3, 0.50, 130, 260, 120},
        {3, 0.76, 620, 260, 116},

        {4, 0.18, 620, 330, 150},
        {4, 0.50, 118, 280, 126},
        {4, 0.78, 622, 240, 115},

        {5, 0.18, 128, 330, 150},
        {5, 0.48, 628, 310, 140},
        {5, 0.78, 610, 300, 130},

        {6, 0.16, 130, 320, 140},
        {6, 0.42, 618, 340, 150},
        {6, 0.68, 132, 280, 130},
        {6, 0.86, 604, 300, 140}
    };

    std::vector<QRectF> rects;
    const int stageStart = Config::GameConfig::stageStartDistance(stage);
    const int stageEnd = Config::GameConfig::stageConfig(stage).targetDistance;
    const int stageLength = qMax(1, stageEnd - stageStart);

    for (const TerrainSpec& spec : specs) {
        if (spec.stage != stage) {
            continue;
        }
        const qreal worldX = stageStart + stageLength * spec.ratio;
        rects.emplace_back(
            worldX - spec.width / 2.0,
            spec.y - spec.height / 2.0,
            spec.width,
            spec.height
        );
    }

    return rects;
}

void GameManager::resolveEntitySolids()
{
    std::vector<SolidBody> bodies;
    bodies.reserve(fish.size() + sharks.size() + swordfishes.size() + octopuses.size() + 1);

    for (auto* f : fish) {
        if (!f || f->caught || f->escaped) continue;
        bodies.push_back({f, nullptr, 0.55});
    }
    for (auto* s : sharks) {
        if (!s || !s->alive) continue;
        bodies.push_back({nullptr, s, 1.25});
    }
    for (auto* s : swordfishes) {
        if (!s || !s->alive) continue;
        bodies.push_back({nullptr, s, 1.1});
    }
    for (auto* o : octopuses) {
        if (!o || !o->alive) continue;
        bodies.push_back({nullptr, o, 0.95});
    }
    if (boss && boss->alive) {
        bodies.push_back({nullptr, boss, 3.2});
    }

    if (bodies.empty()) return;

    const auto& obstacles = ObstacleManager::instance().obstacles();
    const auto terrain = terrainColliders();
    for (int iteration = 0; iteration < 4; ++iteration) {
        for (const auto& body : bodies) {
            setSolidPosition(body, clampedSolidPosition(body, solidPosition(body)));
            for (auto* obstacle : obstacles) {
                if (!obstacle) continue;
                separateSolidFromObstacle(body, obstacle->collider());
            }
            for (const QRectF& terrainRect : terrain) {
                separateSolidFromObstacle(body, terrainRect);
            }
        }

        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                separateSolidPair(bodies[i], bodies[j]);
            }
        }
    }
}

void GameManager::applyStageConfig()
{
    const auto& cfg = Config::GameConfig::stageConfig(stage);
    WaveSystem::instance().configureStage(
        cfg.waveChancePerFrame,
        cfg.waveRightWeight,
        cfg.waveLeftWeight
    );
    WeatherSystem::instance().configureStage(
        cfg.sunnyWeight,
        cfg.fogWeight,
        cfg.stormWeight,
        cfg.weatherMinFrames,
        cfg.weatherMaxFrames,
        cfg.lightningChanceDenominator
    );
}

void GameManager::updateLightningHazard(Player& player)
{
    if (WeatherSystem::instance().currentWeather() != WeatherType::STORM) {
        lightningWarningActive = false;
        lightningStrikeActive = false;
        lightningWarningFrames = 0;
        lightningStrikeFrames = 0;
        return;
    }

    if (!lightningWarningActive && !lightningStrikeActive &&
        WeatherSystem::instance().shouldTriggerLightning()) {
        const int offsetX = QRandomGenerator::global()->bounded(-90, 91);
        const int offsetY = QRandomGenerator::global()->bounded(-70, 71);
        const qreal targetX = std::clamp(
            player.worldPos().x() + offsetX,
            80.0,
            static_cast<qreal>(Config::GameConfig::RIGHT_BORDER - 80)
        );
        const qreal targetY = std::clamp(
            player.worldPos().y() + offsetY,
            static_cast<qreal>(Config::GameConfig::TOP_BORDER + 50),
            static_cast<qreal>(Config::GameConfig::BOTTOM_BORDER - 50)
        );
        lightningTarget = QPointF(targetX, targetY);
        lightningWarningActive = true;
        lightningWarningFrames = 55;
    }

    if (lightningWarningActive) {
        --lightningWarningFrames;
        if (lightningWarningFrames <= 0) {
            lightningWarningActive = false;
            lightningStrikeActive = true;
            lightningStrikeFrames = 14;

            const qreal dx = player.worldPos().x() - lightningTarget.x();
            const qreal dy = player.worldPos().y() - lightningTarget.y();
            if (dx * dx + dy * dy <= 82.0 * 82.0) {
                player.takeDurabilityDamage(Config::GameConfig::STORM_LIGHTNING_DAMAGE);
            }
        }
    }

    if (lightningStrikeActive) {
        --lightningStrikeFrames;
        if (lightningStrikeFrames <= 0) {
            lightningStrikeActive = false;
        }
    }
}

void GameManager::spawnFish()
{
    const auto& cfg = Config::GameConfig::stageConfig(stage);
    int px = playerX();
    int x = px + 300 + QRandomGenerator::global()->bounded(600);
    x = std::min(x, Config::GameConfig::RIGHT_BORDER - 50);
    int y = 80 + QRandomGenerator::global()->bounded(580);
    const int totalWeight = qMax(1, cfg.sardineWeight + cfg.tunaWeight + cfg.eelWeight + cfg.goldenWeight);
    int r = QRandomGenerator::global()->bounded(totalWeight);
    Fish* f;
    if (r < cfg.sardineWeight) {
        const int roll = QRandomGenerator::global()->bounded(stage >= 2 ? 100 : 70);
        if (roll < 45) f = new Sardine(x, y);
        else if (roll < 75) f = new Anchovy(x, y);
        else f = new Clownfish(x, y);
    }
    else if ((r -= cfg.sardineWeight) < cfg.tunaWeight) {
        const int roll = QRandomGenerator::global()->bounded(stage >= 3 ? 100 : 80);
        if (roll < 42) f = new Tuna(x, y);
        else if (roll < 74) f = new Mackerel(x, y);
        else f = new SeaBream(x, y);
    }
    else if ((r -= cfg.tunaWeight) < cfg.eelWeight) {
        const int roll = QRandomGenerator::global()->bounded(stage >= 4 ? 100 : 80);
        if (roll < 42) f = new DeepSeaEel(x, y);
        else if (roll < 72) f = new Lanternfish(x, y);
        else f = new Grouper(x, y);
    }
    else {
        const int roll = QRandomGenerator::global()->bounded(stage >= 5 ? 100 : 80);
        if (roll < 44) f = new GoldenFish(x, y);
        else if (roll < 74) f = new KoiFish(x, y);
        else f = new CrystalFish(x, y);
    }
    fish.push_back(f);
}

void GameManager::spawnObstacles()
{
    ObstacleManager::instance().clear();
    const auto& cfg = Config::GameConfig::stageConfig(stage);
    ObstacleManager::instance().generateLevel(stage, cfg.reefCount, cfg.whirlpoolCount);
}

void GameManager::clearStageEntities()
{
    if (boss) {
        delete boss;
        boss = nullptr;
    }

    for (auto f : fish)        delete f;
    for (auto s : sharks)      delete s;
    for (auto s : swordfishes) delete s;
    for (auto o : octopuses)   delete o;

    fish.clear();
    sharks.clear();
    swordfishes.clear();
    octopuses.clear();
}

void GameManager::resetStageRuntime()
{
    clearStageEntities();
    spawnTimer = 0;
    bossSpawned = false;
    stageClear = false;
    gameOver = false;
    victory = false;
    lightningWarningActive = false;
    lightningStrikeActive = false;
    lightningWarningFrames = 0;
    lightningStrikeFrames = 0;

    applyStageConfig();
    spawnObstacles();
    for (int i = 0; i < Config::GameConfig::stageConfig(stage).initialFish; ++i) {
        spawnFish();
    }
}

void GameManager::spawnShark()
{
    int px = playerX();
    int x = px + 200 + QRandomGenerator::global()->bounded(150);
    int y = 80 + QRandomGenerator::global()->bounded(580);
    sharks.push_back(new Shark(x, y));
}

void GameManager::spawnSwordfish()
{
    int px = playerX();
    int x = px + 300 + QRandomGenerator::global()->bounded(400);
    int y = 80 + QRandomGenerator::global()->bounded(580);
    swordfishes.push_back(new Swordfish(x, y));
}

void GameManager::spawnOctopus()
{
    int px = playerX();
    int x = px + 300 + QRandomGenerator::global()->bounded(400);
    int y = 80 + QRandomGenerator::global()->bounded(580);
    octopuses.push_back(new Octopus(x, y));
}

void GameManager::spawnBoss(int stageNum)
{
    if (boss) { delete boss; boss = nullptr; }
    qreal spawnX = playerX() + 500;
    if (spawnX > Config::GameConfig::RIGHT_BORDER - Config::GameConfig::BOSS_EDGE_BUFFER) {
        spawnX = playerX() - Config::GameConfig::BOSS_EDGE_BUFFER;
    }
    spawnX = std::clamp(
        spawnX,
        static_cast<qreal>(Config::GameConfig::BOSS_EDGE_BUFFER),
        static_cast<qreal>(Config::GameConfig::RIGHT_BORDER - Config::GameConfig::BOSS_EDGE_BUFFER)
    );
    QPointF spawnPos(spawnX, 360);
    if (stageNum >= 6)
        boss = new SirenBoss(spawnPos.x(), spawnPos.y());
    else
        boss = new FiveHeadSharkBoss(spawnPos.x(), spawnPos.y());

    if (!boss) return;

    if (stageNum == 1) {
        boss->hp = 900;
        boss->maxHp = 900;
        boss->dropValue = 350;
    }
    else if (stageNum == 2) {
        boss->hp = 1400;
        boss->maxHp = 1400;
        boss->dropValue = 500;
    }
    else if (stageNum == 3) {
        boss->hp = 1600;
        boss->maxHp = 1600;
        boss->dropValue = 700;
    }
    else if (stageNum == 4) {
        boss->hp = 2100;
        boss->maxHp = 2100;
        boss->dropValue = 900;
    }
    else if (stageNum >= 6) {
        boss->hp = 2600;
        boss->maxHp = 2600;
        boss->dropValue = 1200;
    }
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
        if (o->collider().intersects(p.collider())) {
            o->onPlayerCollision(&p);
        }
    }

    for (const QRectF& terrainRect : terrainColliders()) {
        const QPointF push = minimumSeparationVector(p.collider(), terrainRect);
        if (!push.isNull()) {
            p.setWorldPos(p.worldPos() + push);
        }
    }

    // 普通鲨鱼
    for (auto s : sharks) {
        if (!s->alive) continue;
        if (s->biteCollider().intersects(p.collider())) {
            if (s->canBite() && p.canTakeDamage()) {
                p.takeDurabilityDamage(s->attack);
                QPointF away = p.worldPos() - QPointF(s->x, s->y);
                qreal length = std::sqrt(away.x() * away.x() + away.y() * away.y());
                if (length <= 0.001) {
                    away = QPointF(1.0, 0.0);
                    length = 1.0;
                }
                p.applyRebound(QPointF(away.x() / length * 2.0, away.y() / length * 2.0));
                s->startBiteCooldown(p.worldPos());
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
    bool visionBlockedByOctopus = false;
    for (auto o : octopuses) {
        if (!o->alive || o->isInvisible) continue;
        if (o->collidesWithPlayer(px, py)) {
            o->contactTimer++;
            if (o->contactTimer >= 30)
                visionBlockedByOctopus = true;
        }
        else {
            o->contactTimer = 0;
        }
    }
    p.visionReduced = visionBlockedByOctopus;

    // Boss 逻辑
    if (boss && boss->alive) {
        // 调用 Boss.cpp 内真实的召唤小兵接口
        boss->spawnMinions(sharks);
    }

    // Boss 死亡结算
    if (boss && !boss->alive && !stageClear) {
        p.coins += boss->dropValue;
        killCount++;
        recordBossDiscovery(fileManager, boss->kind);
        stageClear = true;
    }
}

// 采用鼠标位置决定的真实战斗判定逻辑
// 返回值：
// true  = 命中了敌人 / Boss
// false = 没命中任何敌人
bool GameManager::attackAt(int targetX, int targetY, Weapon* weapon)
{
    if (!canAttemptAttack(weapon)) {
        return false;
    }

    int px = playerX();
    int py = playerY();
    QPointF playerPos(px, py);
    QPointF targetPos(targetX, targetY);

    int range = weapon->getRange();
    int damage = weapon->getDamage();

    if (isGunWeapon(weapon)) {
        const std::vector<QLineF> attackLines = gunAttackLines(weapon, playerPos, targetPos);
        const bool shotgun = weapon->getTypeCode() == "Shotgun";
        const int damagePerLine = shotgun
            ? std::max(1, damage / std::max<int>(1, static_cast<int>(attackLines.size())))
            : damage;
        bool hitAny = false;

        auto applyLineDamage = [&](Enemy* enemy) {
            if (!enemy || !enemy->alive) return;

            int hitCount = 0;
            const QRectF hitbox = enemy->collider();
            for (const auto& line : attackLines) {
                if (lineIntersectsRect(line, hitbox)) {
                    ++hitCount;
                }
            }

            if (hitCount <= 0) return;

            enemy->takeDamage(damagePerLine * hitCount);
            applyHitKnockback(enemy, playerPos, 18.0);
            if (!enemy->alive) {
                Player::instance().coins += enemy->dropValue;
                killCount++;
                recordEnemyDiscovery(fileManager, enemy);
            }
            hitAny = true;
        };

        if (boss && boss->alive) {
            int hitCount = 0;
            QPointF secondaryPos;
            int secondaryHp = 0;
            int secondaryMaxHp = 0;
            QRectF hitbox = boss->collider();
            const bool secondaryActive = boss->getSecondaryTarget(secondaryPos, secondaryHp, secondaryMaxHp);
            if (secondaryActive) {
                hitbox = QRectF(
                    secondaryPos.x() - Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH / 2.0,
                    secondaryPos.y() - Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT / 2.0,
                    Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH,
                    Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT
                );
            }
            else if (boss->isInvulnerable()) {
                hitbox = QRectF();
            }

            for (const auto& line : attackLines) {
                if (!hitbox.isEmpty() && lineIntersectsRect(line, hitbox)) {
                    ++hitCount;
                }
            }

            if (hitCount > 0) {
                boss->takeDamage(damagePerLine * hitCount);
                if (!secondaryActive) {
                    applyHitKnockback(boss, playerPos, 6.0);
                }
                hitAny = true;
            }
        }

        for (auto s : sharks) {
            applyLineDamage(s);
        }
        for (auto s : swordfishes) {
            applyLineDamage(s);
        }
        for (auto o : octopuses) {
            applyLineDamage(o);
        }

        if (hitAny) {
            weapon->consumeAttackDurability();
        }

        m_attackCooldown.restart();
        return hitAny;
    }

    if (isHarpoonWeapon(weapon)) {
        const QLineF attackLine(playerPos, clampedAttackEnd(playerPos, targetPos, range));
        Enemy* hitEnemy = nullptr;
        bool hitBoss = false;
        qreal nearestDistance = 1e18;

        auto considerHitbox = [&](Enemy* enemy, const QRectF& hitbox, bool bossTarget) {
            if (!enemy || !enemy->alive) return;
            bool hit = false;
            const qreal distance = harpoonHitDistance(attackLine, hitbox, hit);
            if (!hit || distance >= nearestDistance) return;
            nearestDistance = distance;
            hitEnemy = enemy;
            hitBoss = bossTarget;
        };

        if (boss && boss->alive) {
            QPointF secondaryPos;
            int secondaryHp = 0;
            int secondaryMaxHp = 0;
            QRectF hitbox;
            if (boss->getSecondaryTarget(secondaryPos, secondaryHp, secondaryMaxHp)) {
                hitbox = QRectF(
                    secondaryPos.x() - Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH / 2.0,
                    secondaryPos.y() - Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT / 2.0,
                    Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH,
                    Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT
                );
            }
            else if (!boss->isInvulnerable()) {
                hitbox = boss->collider();
            }

            considerHitbox(boss, hitbox, true);
        }

        for (auto s : sharks) {
            considerHitbox(s, s ? s->collider() : QRectF(), false);
        }
        for (auto s : swordfishes) {
            considerHitbox(s, s ? s->collider() : QRectF(), false);
        }
        for (auto o : octopuses) {
            considerHitbox(o, o ? o->collider() : QRectF(), false);
        }

        if (hitEnemy) {
            hitEnemy->takeDamage(damage);
            if (!hitBoss) {
                applyHitKnockback(hitEnemy, playerPos, 22.0);
            }
            if (!hitBoss && !hitEnemy->alive) {
                Player::instance().coins += hitEnemy->dropValue;
                killCount++;
                recordEnemyDiscovery(fileManager, hitEnemy);
            }
            weapon->consumeAttackDurability();
            m_attackCooldown.restart();
            return true;
        }

        return false;
    }

    bool isHit = false;

    // 1. 优先判定 Boss
    if (boss && boss->alive) {
        if (boss->canBeHitAt(targetX, targetY) &&
            distanceSquared(targetPos, playerPos) <= range * range) {
            boss->takeDamage(damage);
            applyHitKnockback(boss, playerPos, 6.0);
            isHit = true;
        }
    }

    // 2. 判定普通鲨鱼
    if (!isHit) {
        for (auto s : sharks) {
            if (!s->alive) continue;

            if (targetHitsEnemy(s, targetPos, playerPos, range)) {
                s->takeDamage(damage);
                applyHitKnockback(s, playerPos, 18.0);

                if (!s->alive) {
                    Player::instance().coins += s->dropValue;
                    killCount++;
                    recordEnemyDiscovery(fileManager, s);
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

            if (targetHitsEnemy(s, targetPos, playerPos, range)) {
                s->takeDamage(damage);
                applyHitKnockback(s, playerPos, 18.0);

                if (!s->alive) {
                    Player::instance().coins += s->dropValue;
                    killCount++;
                    recordEnemyDiscovery(fileManager, s);
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

            if (targetHitsEnemy(o, targetPos, playerPos, range)) {
                o->takeDamage(damage);
                applyHitKnockback(o, playerPos, 18.0);

                if (!o->alive) {
                    Player::instance().coins += o->dropValue;
                    killCount++;
                    recordEnemyDiscovery(fileManager, o);
                }

                isHit = true;
                break;
            }
        }
    }

    // 命中才扣耐久；纯攻击武器即使空枪也进入冷却。
    // 鱼叉是双用工具，未命中时仍允许继续走捕鱼入口。
    if (isHit) {
        weapon->consumeAttackDurability();
    }

    if (isHit || weapon->getRole() == Config::EquipmentRole::AttackWeapon) {
        m_attackCooldown.restart();
    }

    return isHit;
}

bool GameManager::canAttemptAttack(const Weapon* weapon) const
{
    if (!weapon || !weapon->canAttack() || weapon->isBroken()) {
        return false;
    }

    if (!Player::instance().canUseRangedAttack()) {
        return false;
    }

    return m_attackCooldown.elapsed() >= weapon->getAttackCooldownMs();
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
    data.baseSpeed = static_cast<float>(p.baseSpeed());
    data.killCount = killCount;
    fileManager.saveGame(data);
}

void GameManager::loadSave()
{
    SaveData data;
    if (fileManager.loadGame(data) && !data.isDead) {
        clearStageEntities();

        stage = data.stage;
        Player& p = Player::instance();
        p.restoreSavedProgress(
            data.distance,
            data.durability,
            data.stamina,
            data.maxDurability,
            data.maxStamina,
            data.baseSpeed
        );
        p.coins = data.coins;
        p.fishCaught = data.fishCaught;
        p.fishTotalValue = data.fishTotalValue;
        p.gameSeconds = data.gameSeconds;

        spawnTimer = 0;
        bossSpawned = false;
        stageClear = false;
        gameOver = false;
        victory = false;
        cameraX = std::max(0, playerX() - 640);
        killCount = data.killCount;
        lightningWarningActive = false;
        lightningStrikeActive = false;
        lightningWarningFrames = 0;
        lightningStrikeFrames = 0;

        applyStageConfig();
        const auto& cfg = Config::GameConfig::stageConfig(stage);
        ObstacleManager::instance().generateLevel(stage, cfg.reefCount, cfg.whirlpoolCount);
        for (int i = 0; i < cfg.initialFish; ++i) {
            spawnFish();
        }
    }
}

bool GameManager::isBossDefeated()
{
    return boss && !boss->alive;
}

void GameManager::triggerShockWave() {
    Player& p = Player::instance();
    if (!p.isShockActive()) {
        return;
    }

    QRectF area = p.shockArea();

    // 对小怪造成范围伤害
    for (auto s : sharks) {
        if (!s->alive) continue;
        if (area.intersects(s->collider())) {
            s->takeDamage(50);
            if (!s->alive) {
                p.coins += s->dropValue;
                killCount++;
                recordEnemyDiscovery(fileManager, s);
            }
        }
    }
    for (auto s : swordfishes) {
        if (!s->alive) continue;
        if (area.intersects(s->collider())) {
            s->takeDamage(50);
            if (!s->alive) {
                p.coins += s->dropValue;
                killCount++;
                recordEnemyDiscovery(fileManager, s);
            }
        }
    }
    for (auto o : octopuses) {
        if (!o->alive) continue;
        if (area.intersects(o->collider())) {
            o->takeDamage(50);
            if (!o->alive) {
                p.coins += o->dropValue;
                killCount++;
                recordEnemyDiscovery(fileManager, o);
            }
        }
    }

    // 眩晕Boss
    if (boss && boss->alive && area.intersects(boss->collider())) {
        boss->applyShockStun(3000);
    }
}
