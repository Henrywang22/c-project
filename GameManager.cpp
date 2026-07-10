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

bool bossHazardHitsRect(const BossHazard& hazard, const QRectF& rect)
{
    if (hazard.type == BossHazardType::SoulSong && !hazard.target.isNull()) {
        const qreal padding = qMax<qreal>(0.0, hazard.radius);
        return lineIntersectsRect(QLineF(hazard.position, hazard.target),
                                  rect.adjusted(-padding, -padding, padding, padding));
    }
    if (hazard.radius > 0.0) {
        return distanceSquared(closestPointInRect(rect, hazard.position), hazard.position)
            <= hazard.radius * hazard.radius;
    }
    return !hazard.rect.isEmpty() && hazard.rect.intersects(rect);
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
    else if (dynamic_cast<const ElectricRay*>(enemy)) {
        fileManager.markEnemyDiscovered(3, "Electric Ray");
    }
    else if (dynamic_cast<const PoisonJellyfish*>(enemy)) {
        fileManager.markEnemyDiscovered(4, "Poison Jellyfish");
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

std::vector<QRectF> bossGeneratedSolidRects(const Boss* boss)
{
    std::vector<QRectF> solids;
    if (!boss || !boss->alive) return solids;

    auto addUniqueSolid = [&](const QRectF& rect) {
        for (const QRectF& existing : solids) {
            if (QLineF(existing.center(), rect.center()).length() < 10.0)
                return;
        }
        solids.push_back(rect);
    };

    for (const BossHazard& hazard : boss->getHazards()) {
        if (!hazard.active) continue;
        if (hazard.type == BossHazardType::ReefHitbox && !hazard.rect.isEmpty()) {
            addUniqueSolid(hazard.rect);
        }
        else if (hazard.type == BossHazardType::ResonancePillar) {
            const QPointF c = hazard.position;
            const bool destroyed = hazard.visualStage >= 10;
            const bool bursting = hazard.visualStage >= 4;
            const qreal w = destroyed ? 76.0 : (bursting ? 90.0 : 94.0);
            const qreal h = destroyed ? 48.0 : (bursting ? 84.0 : 104.0);
            const qreal yOffset = destroyed ? 22.0 : 4.0;
            addUniqueSolid(QRectF(c.x() - w / 2.0,
                                  c.y() + yOffset - h / 2.0,
                                  w, h));
        }
    }
    return solids;
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

    if (!dynamic_cast<Boss*>(enemy)) {
        enemy->applyKnockback(origin, strength);
        return;
    }

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
    for (int i = 0; i < 5; ++i) {
        m_enemyDiscoveryRecorded[i] = fileManager.isEnemyDiscovered(i);
    }
    WaveSystem::instance().reset();
    WeatherSystem::instance().reset();
    applyStageConfig();
    ObstacleManager::instance().generateLevel(
        stage,
        Config::GameConfig::stageConfig(stage).reefCount,
        Config::GameConfig::stageConfig(stage).whirlpoolCount
    );
    for (int i = 0; i < Config::GameConfig::stageConfig(stage).initialFish; i++) spawnFish();
    m_attackCooldown.invalidate();
}

GameManager::~GameManager()
{
    clearStageEntities();
    ObstacleManager::instance().clear();
}

void GameManager::update(qreal deltaTime)
{
    if (gameOver || victory) return;
    m_deltaTime = qBound<qreal>(0.001, deltaTime, 0.05);

    Player& p = Player::instance();

    WaveSystem::instance().update(m_deltaTime);
    WeatherSystem::instance().update(m_deltaTime);
    p.update(m_deltaTime);

    // A boss encounter belongs to the current stage.  Do not let movement,
    // waves or dash carry the player beyond later stage finish lines while the
    // encounter is still active.
    if (bossSpawned && boss && boss->alive) {
        const qreal bossReachRight = boss->collider().right() + 260.0;
        const qreal encounterRight = std::min<qreal>(
            Config::GameConfig::RIGHT_BORDER,
            std::max<qreal>(Config::GameConfig::stageConfig(stage).targetDistance + 140.0,
                            bossReachRight));
        if (p.worldPos().x() > encounterRight) {
            QPointF pos = p.worldPos();
            pos.setX(encounterRight);
            p.setWorldPos(pos);
        }
    }
    updateLightningHazard(p);

    if (p.isDead()) { gameOver = true; return; }

    gameTimer++;
    m_gameSecondsAccumulator += m_deltaTime;
    while (m_gameSecondsAccumulator >= 1.0) {
        ++p.gameSeconds;
        m_gameSecondsAccumulator -= 1.0;
    }
    p.distance = playerX();

    const int px = playerX();
    const int py = playerY();

    fish.erase(std::remove_if(fish.begin(), fish.end(),
        [](Fish* f) {
            if (f->caught || f->escaped) { delete f; return true; }
            return false;
        }), fish.end());

    auto eraseInactive = [px](auto& enemies) {
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [px](auto* enemy) {
                if (!enemy->alive || enemy->position().x() < px - 1800) {
                    delete enemy;
                    return true;
                }
                return false;
            }), enemies.end());
    };

    eraseInactive(sharks);
    eraseInactive(swordfishes);
    eraseInactive(octopuses);
    eraseInactive(specialEnemies);

    ObstacleManager::instance().update(m_deltaTime);

    for (auto f : fish) {
        f->update(px, py);
        if (!f->lockedForCatch && f->x < px - 1800) {
            f->escaped = true;
        }
    }
    for (auto s : sharks)      s->update(p);
    for (auto s : swordfishes) s->update(p);
    for (auto o : octopuses)   o->update(p);
    for (auto e : specialEnemies) e->update(p);

    const auto terrain = terrainColliders();
    const auto bossSolids = bossGeneratedSolidRects(boss);
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
        for (const QRectF& bossSolid : bossSolids) {
            applyEnemyAvoidanceNudge(enemy, bossSolid, p.worldPos());
        }
    };
    for (auto s : sharks)      nudgeEnemyAroundSolids(s);
    for (auto s : swordfishes) nudgeEnemyAroundSolids(s);
    for (auto o : octopuses)   nudgeEnemyAroundSolids(o);
    for (auto e : specialEnemies) nudgeEnemyAroundSolids(e);

    // 接入 B 模块真实接口，仅传入 p
    if (boss && boss->alive) {
        boss->update(p);
        applyBossEffectsToCreatures();
    }

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
        int rayCount = 0;
        int jellyCount = 0;
        for (Enemy* enemy : specialEnemies) {
            if (dynamic_cast<ElectricRay*>(enemy)) ++rayCount;
            else if (dynamic_cast<PoisonJellyfish*>(enemy)) ++jellyCount;
        }
        if (cfg.electricRayCap > 0 && cfg.electricRaySpawnInterval > 0 &&
            spawnTimer % cfg.electricRaySpawnInterval == 0 &&
            rayCount < cfg.electricRayCap) {
            spawnElectricRay();
        }
        if (cfg.jellyfishCap > 0 && cfg.jellyfishSpawnInterval > 0 &&
            spawnTimer % cfg.jellyfishSpawnInterval == 0 &&
            jellyCount < cfg.jellyfishCap) {
            spawnPoisonJellyfish();
        }
    }

    recordVisibleEnemyDiscoveries();

    resolveEntitySolids();

    checkCollisions();

    if (p.isDead()) {
        stageClear = false;
        gameOver = true;
        return;
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

    if (stage > Config::GameConfig::STAGE_COUNT) victory = true;
}

int GameManager::stageBossTriggerX() const
{
    int trigger = Config::GameConfig::stageConfig(stage).targetDistance;
    return std::max(0, std::min(trigger, Config::GameConfig::RIGHT_BORDER));
}

std::vector<QRectF> GameManager::terrainColliders() const
{
    std::vector<QRectF> rects;
    const int stageStart = Config::GameConfig::stageStartDistance(stage);
    const int stageEnd = Config::GameConfig::stageConfig(stage).targetDistance;
    const int stageLength = qMax(1, stageEnd - stageStart);

    for (int i = 0; i < Config::GameConfig::STAGE_DECOR_COUNT; ++i) {
        const auto& decor = Config::GameConfig::STAGE_DECORS[i];
        if (decor.stage != stage) continue;

        const qreal worldX = stageStart + stageLength * decor.stageRatio;
        rects.emplace_back(
            worldX - decor.colliderWidth / 2.0,
            decor.y + decor.colliderOffsetY - decor.colliderHeight / 2.0,
            decor.colliderWidth,
            decor.colliderHeight
        );
    }

    for (int i = 0; i < Config::GameConfig::TERRAIN_PROP_COUNT; ++i) {
        const auto& prop = Config::GameConfig::TERRAIN_PROPS[i];
        if (prop.stage != stage) continue;

        const qreal worldX = stageStart + stageLength * prop.stageRatio;
        rects.emplace_back(
            worldX - prop.colliderWidth / 2.0,
            prop.y - prop.colliderHeight / 2.0,
            prop.colliderWidth,
            prop.colliderHeight
        );
    }

    return rects;
}

void GameManager::recordVisibleEnemyDiscoveries()
{
    auto recordIfVisible = [this](Enemy* enemy) {
        if (!enemy || !enemy->alive) return;
        const QRectF screenRect = enemy->collider().translated(-cameraX, 0);
        if (!screenRect.intersects(QRectF(0, Config::GameConfig::TOP_BORDER,
                                          1280,
                                          Config::GameConfig::BOTTOM_BORDER -
                                              Config::GameConfig::TOP_BORDER))) return;

        int id = -1;
        const char* name = "";
        if (dynamic_cast<Shark*>(enemy)) {
            id = 0;
            name = "Shark";
        }
        else if (dynamic_cast<Swordfish*>(enemy)) {
            id = 1;
            name = "Swordfish";
        }
        else if (dynamic_cast<Octopus*>(enemy)) {
            id = 2;
            name = "Octopus";
        }
        else if (dynamic_cast<ElectricRay*>(enemy)) {
            id = 3;
            name = "Electric Ray";
        }
        else if (dynamic_cast<PoisonJellyfish*>(enemy)) {
            id = 4;
            name = "Poison Jellyfish";
        }

        if (id >= 0 && !m_enemyDiscoveryRecorded[id]) {
            fileManager.markEnemyDiscovered(id, name);
            m_enemyDiscoveryRecorded[id] = true;
        }
    };

    for (Shark* enemy : sharks) recordIfVisible(enemy);
    for (Swordfish* enemy : swordfishes) recordIfVisible(enemy);
    for (Octopus* enemy : octopuses) recordIfVisible(enemy);
    for (Enemy* enemy : specialEnemies) recordIfVisible(enemy);
}

void GameManager::resolveEntitySolids()
{
    std::vector<SolidBody> bodies;
    bodies.reserve(fish.size() + sharks.size() + swordfishes.size() +
                   octopuses.size() + specialEnemies.size() + 1);

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
    for (auto* e : specialEnemies) {
        if (!e || !e->alive) continue;
        bodies.push_back({nullptr, e, 1.0});
    }
    if (boss && boss->alive) {
        bodies.push_back({nullptr, boss, 3.2});
    }

    if (bodies.empty()) return;

    const auto& obstacles = ObstacleManager::instance().obstacles();
    const auto terrain = terrainColliders();
    const auto bossSolids = bossGeneratedSolidRects(boss);
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
            for (const QRectF& bossSolid : bossSolids) {
                separateSolidFromObstacle(body, bossSolid);
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

void GameManager::applyBossEffectsToCreatures()
{
    if (!boss || !boss->alive || boss->kind != BossKind::Siren) return;

    auto forEachEnemy = [&](const auto& fn) {
        for (Shark* e : sharks) if (e && e->alive) fn(e);
        for (Swordfish* e : swordfishes) if (e && e->alive) fn(e);
        for (Octopus* e : octopuses) if (e && e->alive) fn(e);
        for (Enemy* e : specialEnemies) if (e && e->alive) fn(e);
    };

    for (const BossHazard& hazard : boss->getHazards()) {
        if (!hazard.active) continue;

        if (hazard.type == BossHazardType::SoulSong &&
            hazard.damage > 0 && hazard.elapsedMs <= 24.0) {
            for (Fish* f : fish) {
                if (f && !f->caught && !f->escaped &&
                    bossHazardHitsRect(hazard, f->collider())) {
                    f->applyStun(2400);
                }
            }
            forEachEnemy([&](Enemy* e) {
                if (bossHazardHitsRect(hazard, e->collider()))
                    e->applyStun(2100);
            });
        }
        else if (hazard.type == BossHazardType::SeaweedZone) {
            for (Fish* f : fish) {
                if (f && !f->caught && !f->escaped &&
                    bossHazardHitsRect(hazard, f->collider())) {
                    f->applySlow(150, 0.34);
                }
            }
            forEachEnemy([&](Enemy* e) {
                if (bossHazardHitsRect(hazard, e->collider()))
                    e->applySlow(150, 0.38);
            });
        }
        else if (hazard.type == BossHazardType::ElegyWarning &&
                 hazard.visualStage >= 1 && hazard.visualStage <= 3) {
            for (Fish* f : fish) {
                if (f && !f->caught && !f->escaped &&
                    bossHazardHitsRect(hazard, f->collider())) {
                    f->applySlow(150, 0.52);
                }
            }
            forEachEnemy([&](Enemy* e) {
                if (bossHazardHitsRect(hazard, e->collider()))
                    e->applySlow(150, 0.55);
            });
        }
    }
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
    for (auto e : specialEnemies) delete e;

    fish.clear();
    sharks.clear();
    swordfishes.clear();
    octopuses.clear();
    specialEnemies.clear();
}

void GameManager::resetStageRuntime()
{
    clearStageEntities();
    WaveSystem::instance().reset();
    WeatherSystem::instance().reset();

    // Boss fights may end after the player has drifted beyond one or more
    // later checkpoints.  A new stage always begins at its own entrance.
    Player& player = Player::instance();
    QPointF stageStartPos = player.worldPos();
    stageStartPos.setX(Config::GameConfig::stageStartDistance(stage) + 60.0);
    stageStartPos.setY(std::clamp(
        stageStartPos.y(),
        static_cast<qreal>(Config::GameConfig::TOP_BORDER),
        static_cast<qreal>(Config::GameConfig::BOTTOM_BORDER)));
    player.setWorldPos(stageStartPos);
    player.distance = qRound(stageStartPos.x());

    spawnTimer = 0;
    m_gameSecondsAccumulator = 0.0;
    bossSpawned = false;
    stageClear = false;
    bossClearDelayMs = 0;
    bossRewardSettled = false;
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
    Shark* enemy = new Shark(x, y);
    enemy->applyStageScaling(stage);
    sharks.push_back(enemy);
}

void GameManager::spawnSwordfish()
{
    int px = playerX();
    int x = px + 300 + QRandomGenerator::global()->bounded(400);
    int y = 80 + QRandomGenerator::global()->bounded(580);
    Swordfish* enemy = new Swordfish(x, y);
    enemy->applyStageScaling(stage);
    swordfishes.push_back(enemy);
}

void GameManager::spawnOctopus()
{
    int px = playerX();
    int x = px + 300 + QRandomGenerator::global()->bounded(400);
    int y = 80 + QRandomGenerator::global()->bounded(580);
    Octopus* enemy = new Octopus(x, y);
    enemy->applyStageScaling(stage);
    octopuses.push_back(enemy);
}

void GameManager::spawnElectricRay()
{
    const int x = qMin(
        Config::GameConfig::RIGHT_BORDER - 120,
        playerX() + 360 + QRandomGenerator::global()->bounded(520)
    );
    const int y = 100 + QRandomGenerator::global()->bounded(540);
    ElectricRay* enemy = new ElectricRay(x, y);
    enemy->applyStageScaling(stage);
    specialEnemies.push_back(enemy);
}

void GameManager::spawnPoisonJellyfish()
{
    const int x = qMin(
        Config::GameConfig::RIGHT_BORDER - 120,
        playerX() + 360 + QRandomGenerator::global()->bounded(520)
    );
    const int y = 100 + QRandomGenerator::global()->bounded(540);
    PoisonJellyfish* enemy = new PoisonJellyfish(x, y);
    enemy->applyStageScaling(stage);
    specialEnemies.push_back(enemy);
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
    if (stageNum >= 9)
        boss = new SirenBoss(spawnPos.x(), spawnPos.y());
    else
        boss = new FiveHeadSharkBoss(spawnPos.x(), spawnPos.y());

    if (!boss) return;
    bossClearDelayMs = 0;
    bossRewardSettled = false;

    if (stageNum == 4) {
        boss->hp = 5000;
        boss->maxHp = 5000;
        boss->dropValue = 900;
    }
    else if (stageNum >= 9) {
        boss->hp = 9000;
        boss->maxHp = 9000;
        boss->dropValue = 1800;
    }
    recordBossDiscovery(fileManager, boss->kind);
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
    auto separatePlayerFromEnemy = [&](Enemy* enemy) {
        if (!enemy || !enemy->alive) return;
        const QPointF push = minimumSeparationVector(p.collider(), enemy->collider());
        if (!push.isNull()) {
            p.setWorldPos(p.worldPos() + push);
        }
    };
    for (auto* enemy : sharks) separatePlayerFromEnemy(enemy);
    for (auto* enemy : swordfishes) separatePlayerFromEnemy(enemy);
    for (auto* enemy : octopuses) separatePlayerFromEnemy(enemy);
    for (auto* enemy : specialEnemies) separatePlayerFromEnemy(enemy);

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

    // Boss 逻辑
    if (boss && boss->alive) {
        const QPointF push = minimumSeparationVector(p.collider(), boss->collider());
        if (!push.isNull()) {
            p.setWorldPos(p.worldPos() + push);
        }
        // 调用 Boss.cpp 内真实的召唤小兵接口
        boss->spawnMinions(sharks);
        for (Shark* shark : sharks) {
            if (shark) shark->applyStageScaling(stage);
        }
    }

    // Boss 死亡结算
    if (boss && !boss->alive && !stageClear) {
        if (!bossRewardSettled) {
            p.coins += boss->dropValue;
            killCount++;
            recordBossDiscovery(fileManager, boss->kind);
            if (boss->kind == BossKind::Siren) {
                p.clearMaxStaminaPenalty();
                p.restoreStaminaToFull();
            }
            bossRewardSettled = true;
            bossClearDelayMs = 2600;
        }
        else {
            bossClearDelayMs = std::max(0, bossClearDelayMs - qRound(m_deltaTime * 1000.0));
            if (bossClearDelayMs <= 0)
                stageClear = true;
        }
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
        struct PendingGunHit {
            Enemy* enemy = nullptr;
            bool bossTarget = false;
            bool secondaryTarget = false;
            int pelletCount = 0;
        };
        std::vector<PendingGunHit> pendingHits;

        auto addPendingHit = [&](Enemy* enemy, bool bossTarget, bool secondaryTarget) {
            for (auto& hit : pendingHits) {
                if (hit.enemy == enemy && hit.secondaryTarget == secondaryTarget) {
                    ++hit.pelletCount;
                    return;
                }
            }
            pendingHits.push_back({ enemy, bossTarget, secondaryTarget, 1 });
        };

        const auto terrain = terrainColliders();
        for (const QLineF& line : attackLines) {
            qreal nearestDistance = 1e18;
            Enemy* nearestEnemy = nullptr;
            bool nearestIsBoss = false;
            bool nearestIsSecondary = false;

            auto considerBlocker = [&](const QRectF& rect) {
                bool hit = false;
                const qreal distance = harpoonHitDistance(line, rect, hit);
                if (hit) nearestDistance = qMin(nearestDistance, distance);
            };
            for (Obstacle* obstacle : ObstacleManager::instance().obstacles()) {
                if (obstacle) considerBlocker(obstacle->collider());
            }
            for (const QRectF& rect : terrain) considerBlocker(rect);

            auto considerEnemy = [&](Enemy* enemy, const QRectF& hitbox,
                                     bool bossTarget, bool secondaryTarget = false) {
                if (!enemy || !enemy->alive || hitbox.isEmpty()) return;
                bool hit = false;
                const qreal distance = harpoonHitDistance(line, hitbox, hit);
                if (!hit || distance >= nearestDistance) return;
                nearestDistance = distance;
                nearestEnemy = enemy;
                nearestIsBoss = bossTarget;
                nearestIsSecondary = secondaryTarget;
            };

            if (boss && boss->alive) {
                QPointF secondaryPos;
                int secondaryHp = 0;
                int secondaryMaxHp = 0;
                const bool secondaryActive =
                    boss->getSecondaryTarget(secondaryPos, secondaryHp, secondaryMaxHp);
                QRectF bossHitbox;
                if (secondaryActive) {
                    bossHitbox = QRectF(
                        secondaryPos.x() - Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH / 2.0,
                        secondaryPos.y() - Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT / 2.0,
                        Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH,
                        Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT);
                }
                else if (!boss->isInvulnerable()) {
                    bossHitbox = boss->collider();
                }
                considerEnemy(boss, bossHitbox, true, secondaryActive);
            }
            for (Shark* enemy : sharks)
                considerEnemy(enemy, enemy ? enemy->collider() : QRectF(), false);
            for (Swordfish* enemy : swordfishes)
                considerEnemy(enemy, enemy ? enemy->collider() : QRectF(), false);
            for (Octopus* enemy : octopuses)
                considerEnemy(enemy, enemy ? enemy->collider() : QRectF(), false);
            for (Enemy* enemy : specialEnemies)
                considerEnemy(enemy, enemy ? enemy->collider() : QRectF(), false);

            if (nearestEnemy) addPendingHit(nearestEnemy, nearestIsBoss, nearestIsSecondary);
        }

        bool hitAny = !pendingHits.empty();
        for (const PendingGunHit& hit : pendingHits) {
            hit.enemy->takeDamage(damagePerLine * hit.pelletCount);
            if (hit.bossTarget) {
                if (!hit.secondaryTarget) applyHitKnockback(hit.enemy, playerPos, 6.0);
            }
            else {
                applyHitKnockback(hit.enemy, playerPos, 18.0);
                if (!hit.enemy->alive) {
                    Player::instance().coins += hit.enemy->dropValue;
                    killCount++;
                    recordEnemyDiscovery(fileManager, hit.enemy);
                }
            }
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
        for (auto e : specialEnemies) {
            considerHitbox(e, e ? e->collider() : QRectF(), false);
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
        for (auto e : specialEnemies) {
            if (!e || !e->alive) continue;
            if (targetHitsEnemy(e, targetPos, playerPos, range)) {
                e->takeDamage(damage);
                applyHitKnockback(e, playerPos, 18.0);
                if (!e->alive) {
                    Player::instance().coins += e->dropValue;
                    killCount++;
                    recordEnemyDiscovery(fileManager, e);
                }
                isHit = true;
                break;
            }
        }
    }

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

    if (Player::instance().isStunned() || !Player::instance().canUseRangedAttack()) {
        return false;
    }

    return !m_attackCooldown.isValid() ||
           m_attackCooldown.elapsed() >= weapon->getAttackCooldownMs();
}

bool GameManager::saveAndQuit()
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
    return fileManager.saveGame(data);
}

bool GameManager::loadSave()
{
    SaveData data;
    if (fileManager.loadGame(data) && !data.isDead) {
        clearStageEntities();

        stage = std::clamp(data.stage, 1, Config::GameConfig::STAGE_COUNT);
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
        bossClearDelayMs = 0;
        bossRewardSettled = false;
        gameOver = false;
        victory = false;
        cameraX = std::max(0, playerX() - 640);
        killCount = data.killCount;
        lightningWarningActive = false;
        lightningStrikeActive = false;
        lightningWarningFrames = 0;
        lightningStrikeFrames = 0;
        m_gameSecondsAccumulator = 0.0;

        WaveSystem::instance().reset();
        WeatherSystem::instance().reset();
        applyStageConfig();
        const auto& cfg = Config::GameConfig::stageConfig(stage);
        ObstacleManager::instance().generateLevel(stage, cfg.reefCount, cfg.whirlpoolCount);
        for (int i = 0; i < cfg.initialFish; ++i) {
            spawnFish();
        }
        return true;
    }
    return false;
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

    for (auto f : fish) {
        if (!f || f->caught || f->escaped) continue;
        if (!area.intersects(f->collider())) continue;

        f->applyStun(2200);
        f->fleeing = false;
        f->fleeCooldown = 0;
    }

    // 对小怪造成范围伤害
    for (auto s : sharks) {
        if (!s->alive) continue;
        if (area.intersects(s->collider())) {
            s->applyStun(2200);
        }
    }
    for (auto s : swordfishes) {
        if (!s->alive) continue;
        if (area.intersects(s->collider())) {
            s->applyStun(2200);
        }
    }
    for (auto o : octopuses) {
        if (!o->alive) continue;
        if (area.intersects(o->collider())) {
            o->applyStun(2200);
        }
    }
    for (auto e : specialEnemies) {
        if (!e || !e->alive) continue;
        if (area.intersects(e->collider())) {
            e->applyStun(2200);
        }
    }

    // 眩晕Boss
    if (boss && boss->alive) {
        bool shockBoss = area.intersects(boss->collider());

        QPointF secondaryPos;
        int secondaryHp = 0;
        int secondaryMaxHp = 0;
        if (boss->getSecondaryTarget(secondaryPos, secondaryHp, secondaryMaxHp)) {
            QRectF secondaryRect(
                secondaryPos.x() - Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH / 2.0,
                secondaryPos.y() - Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT / 2.0,
                Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH,
                Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT
            );
            shockBoss = shockBoss || area.intersects(secondaryRect);
        }

        QPointF companionPos;
        bool companionStunned = false;
        if (boss->getCompanionVisual(companionPos, companionStunned)) {
            QRectF companionRect(companionPos.x() - 48.0, companionPos.y() - 48.0, 96.0, 96.0);
            shockBoss = shockBoss || area.intersects(companionRect);
        }

        if (shockBoss) {
            boss->applyShockStun(3200);
        }
    }
}
