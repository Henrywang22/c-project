// Boss.cpp
#include "Boss.h"

#include <QLineF>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr int FrameMs = 16;
constexpr int ArenaTop = 70;
constexpr int ArenaBottom = 690;
constexpr int ArenaRightOffset = 120;

int clampInt(int value, int low, int high)
{
    return std::max(low, std::min(value, high));
}

float angleTo(const QPointF& from, const QPointF& to)
{
    const QPointF d = to - from;
    return qRadiansToDegrees(std::atan2(d.y(), d.x()));
}

QPointF stepToward(const QPointF& from, const QPointF& to, float amount)
{
    const QPointF d = to - from;
    const float length = std::sqrt(float(d.x() * d.x() + d.y() * d.y()));
    if (length <= 0.001f) return from;
    return QPointF(from.x() + d.x() / length * amount,
                   from.y() + d.y() / length * amount);
}

bool circleHitsPlayer(const QPointF& center, qreal radius, const Player& player)
{
    return QLineF(center, player.worldPos()).length() <= radius;
}

bool rectHitsPlayer(const QRectF& rect, const Player& player)
{
    return rect.intersects(player.collider());
}
}

Boss::Boss(BossType type, int startX, int startY, int startMaxHp, int startAttack, int startDropValue)
    : Enemy(startX, startY), bossType(type)
{
    hp = startMaxHp;
    maxHp = startMaxHp;
    attack = startAttack;
    dropValue = startDropValue;
}

void Boss::update(Player& player)
{
    if (!alive) return;

    updateCommonTimers(FrameMs);
    updateHazards(FrameMs);

    if (hp <= maxHp / 2) enraged = true;
    if (!isStunned()) updateBoss(player);
}

bool Boss::collidesWithPlayer(int px, int py)
{
    const int dx = px - x;
    const int dy = py - y;
    return dx * dx + dy * dy < 45 * 45;
}

void Boss::takeDamage(int damage)
{
    if (!alive || invulnerable) return;
    hp -= damage;
    if (hp <= 0) {
        hp = 0;
        alive = false;
        state = PHASE2;
    }
}

void Boss::applyShockStun(int durationMs)
{
    stunRemainingMs = std::max(stunRemainingMs, durationMs);
    holdingPlayer = false;
}

void Boss::forceReleasePlayer()
{
    holdingPlayer = false;
}

std::vector<BossSpawnRequest> Boss::takeSpawnRequests()
{
    std::vector<BossSpawnRequest> out = spawnRequests;
    spawnRequests.clear();
    return out;
}

void Boss::spawnMinions(std::vector<Shark*>& sharks)
{
    for (const BossSpawnRequest& request : takeSpawnRequests()) {
        if (request.type == BossSpawnType::Shark) {
            sharks.push_back(new Shark((int)request.position.x(), (int)request.position.y()));
        }
    }
}

int Boss::scaledDamage(int baseDamage) const
{
    return enraged ? int(baseDamage * 1.2f) : baseDamage;
}

void Boss::setPhase(State nextState)
{
    state = nextState;
}

void Boss::updateCommonTimers(int frameMs)
{
    stunRemainingMs = std::max(0, stunRemainingMs - frameMs);
}

void Boss::updateHazards(int frameMs)
{
    for (BossHazard& hazard : hazards) {
        hazard.elapsedMs += frameMs;
        if (hazard.durationMs > 0 && hazard.elapsedMs >= hazard.durationMs)
            hazard.active = false;
    }

    hazards.erase(std::remove_if(hazards.begin(), hazards.end(),
        [](const BossHazard& hazard) { return !hazard.active; }), hazards.end());
}

void Boss::addHazard(const BossHazard& hazard)
{
    hazards.push_back(hazard);
}

void Boss::requestSpawn(BossSpawnType type, const QPointF& position,
                        const QPointF& direction, int requestHp, int requestDamage)
{
    spawnRequests.push_back({ type, position, direction, requestHp, requestDamage });
}

QPointF Boss::playerPos(const Player& player) const
{
    return player.worldPos();
}

FiveHeadSharkBoss::FiveHeadSharkBoss(int startX, int startY)
    : Boss(BossType::FiveHeadShark, startX, startY, 2000, 50, 500)
{
    speed = 2.0f;
}

bool FiveHeadSharkBoss::collidesWithPlayer(int px, int py)
{
    const int dx = px - x;
    const int dy = py - y;
    return dx * dx + dy * dy < 42 * 42;
}

void FiveHeadSharkBoss::updateBoss(Player& player)
{
    updatePatrol(FrameMs);
    updateMelee(player, FrameMs);
    updateSummon(FrameMs);
    updateBombardment(player, FrameMs);
}

void FiveHeadSharkBoss::updatePatrol(int)
{
    y += int(speed * patrolDirection);
    if (y < ArenaTop + 40) {
        y = ArenaTop + 40;
        patrolDirection = 1.0f;
    }
    if (y > ArenaBottom - 40) {
        y = ArenaBottom - 40;
        patrolDirection = -1.0f;
    }
}

void FiveHeadSharkBoss::updateMelee(Player& player, int frameMs)
{
    meleeCooldownMs = std::max(0, meleeCooldownMs - frameMs);
    if (meleePhase == 0 && meleeCooldownMs > 0) return;

    const QRectF threatRect(x - 145, y - 50, 145, 100);
    if (meleePhase == 0 && rectHitsPlayer(threatRect, player)) {
        meleePhase = 1;
        meleeStateMs = 500;
        addHazard({ BossHazardType::MeleeHitbox, QPointF(x - 70, y),
                    threatRect, 0, 0, 0, 500, 0, 0, true, false });
        return;
    }

    if (meleePhase == 0) return;

    meleeStateMs -= frameMs;
    if (meleeStateMs > 0) return;

    if (meleePhase == 1) {
        const QRectF hitRect(x - 130, y - 42, 130, 84);
        addHazard({ BossHazardType::MeleeHitbox, QPointF(x - 65, y),
                    hitRect, 0, 0, 0, 250, 0, scaledDamage(100), true, false });
        if (rectHitsPlayer(hitRect, player))
            player.takeDurabilityDamage(scaledDamage(100));
        meleePhase = 2;
        meleeStateMs = 600;
        return;
    }

    meleePhase = 0;
    meleeCooldownMs = 3000;
}

void FiveHeadSharkBoss::updateSummon(int frameMs)
{
    summonTimerMs -= frameMs;
    if (summonTimerMs > 0) return;

    requestSpawn(BossSpawnType::Shark, QPointF(x - 90, y));
    requestSpawn(BossSpawnType::Shark, QPointF(x, y - 90));
    requestSpawn(BossSpawnType::Shark, QPointF(x, y + 90));
    minionSpawned = true;
    summonTimerMs = 20000;
}

void FiveHeadSharkBoss::updateBombardment(Player& player, int frameMs)
{
    if (bombardmentCastMs > 0) {
        bombardmentCastMs -= frameMs;
        if (bombardmentCastMs <= 0) {
            for (const QRectF& rect : pendingBombRects) {
                addHazard({ BossHazardType::BombHitbox, rect.center(), rect,
                            0, 0, 0, 300, 0, scaledDamage(150), true, false });
                if (rectHitsPlayer(rect, player))
                    player.takeDurabilityDamage(scaledDamage(150));
            }
            pendingBombRects.clear();
        }
        return;
    }

    bombardmentTimerMs -= frameMs;
    if (bombardmentTimerMs > 0) return;

    pendingBombRects.clear();
    for (int i = 0; i < 10; ++i) {
        const int bx = int(player.worldPos().x()) - 360 + std::rand() % 720;
        const int by = ArenaTop + std::rand() % (ArenaBottom - ArenaTop);
        pendingBombRects.push_back(QRectF(bx - 24, by - 24, 48, 48));
    }

    const int offsets[6][2] = {{-70, -55}, {-70, 55}, {-15, -75}, {-15, 75}, {45, -45}, {45, 45}};
    for (const auto& offset : offsets)
        pendingBombRects.push_back(QRectF(x + offset[0] - 24, y + offset[1] - 24, 48, 48));

    for (const QRectF& rect : pendingBombRects) {
        addHazard({ BossHazardType::BombWarning, rect.center(), rect,
                    0, 0, 0, 1500, 0, 0, true, false });
    }

    bombardmentCastMs = 1500;
    bombardmentTimerMs = 20000;
}

TaliMonsterBoss::TaliMonsterBoss(int startX, int startY)
    : Boss(BossType::TaliMonster, startX, startY, 2250, 100, 800)
{
    speed = 0.8f;
}

void TaliMonsterBoss::takeDamage(int damage)
{
    if (state == PHASE2 && invulnerable && cloneAlive) {
        cloneHp -= damage;
        if (cloneHp <= 0) startCloneExplosion();
        return;
    }

    if (invulnerable) return;

    hp -= damage;
    if (hp > 0) return;

    if (state == PHASE1) {
        setPhase(PHASE2);
        hp = 2000;
        maxHp = 2000;
        invulnerable = true;
        enraged = false;
        x += 220;
        y = 360;
        spawnClone();
    } else {
        hp = 0;
        alive = false;
    }
}

void TaliMonsterBoss::forceReleasePlayer()
{
    holdingPlayer = false;
}

void TaliMonsterBoss::updateBoss(Player& player)
{
    if (state == PHASE1) updatePhase1(player, FrameMs);
    else updatePhase2(player, FrameMs);
}

void TaliMonsterBoss::updatePhase1(Player& player, int frameMs)
{
    updateMovement(player);
    updateMouthStrike(player, frameMs);
    updateEyeSweep(player, frameMs);
}

void TaliMonsterBoss::updatePhase2(Player& player, int frameMs)
{
    if (!cloneSpawned) spawnClone();
    updateClone(player, frameMs);
    updateMouthStrike(player, frameMs);
    updateEyeSweep(player, frameMs);
}

void TaliMonsterBoss::updateMovement(Player& player)
{
    if (state == PHASE2) return;
    const QPointF next = stepToward(QPointF(x, y), player.worldPos(), speed);
    x = int(next.x());
    y = clampInt(int(next.y()), ArenaTop, ArenaBottom);
}

void TaliMonsterBoss::updateMouthStrike(Player& player, int frameMs)
{
    if (mouthSequenceTimerMs > 0) {
        mouthSequenceTimerMs -= frameMs;
        if (mouthSequenceTimerMs > 0) return;

        ++mouthStrikeIndex;
        const bool grabStrike = (mouthStrikeIndex >= 4);
        const int baseDamage = grabStrike ? (phase2InvulnerabilityEnded ? 120 : 100)
                                          : (phase2InvulnerabilityEnded ? 120 : 100);
        const QPointF target = player.worldPos();
        const float angle = angleTo(QPointF(x, y), target);
        const QRectF hitRect(std::min((qreal)x, target.x()) - 20, std::min((qreal)y, target.y()) - 20,
                             std::abs(target.x() - x) + 40, std::abs(target.y() - y) + 40);

        addHazard({ BossHazardType::MouthStrike, hitRect.center(), hitRect,
                    0, angle, 0, grabStrike ? 300.0 : 250.0, 0, baseDamage, true, false });
        if (rectHitsPlayer(hitRect, player)) {
            player.takeDurabilityDamage(baseDamage);
            if (grabStrike) {
                holdingPlayer = true;
                player.takeDurabilityDamage(phase2InvulnerabilityEnded ? 230 : 200);
            }
        }

        if (grabStrike) {
            mouthStrikeIndex = 0;
            mouthTimerMs = (state == PHASE2 && invulnerable) ? 23000 : 15000;
            mouthSequenceTimerMs = 0;
        } else {
            mouthSequenceTimerMs = 250;
        }
        return;
    }

    mouthTimerMs -= frameMs;
    if (mouthTimerMs <= 0) {
        mouthStrikeIndex = 0;
        mouthSequenceTimerMs = 550;
    }
}

void TaliMonsterBoss::updateEyeSweep(Player& player, int frameMs)
{
    if (eyeSweepRemainingMs > 0) {
        eyeSweepRemainingMs -= frameMs;
        float targetAngle = angleTo(QPointF(x, y), player.worldPos());
        float diff = targetAngle - eyeAngleDegrees;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        const float step = 0.9f;
        eyeAngleDegrees += std::max(-step, std::min(step, diff));

        addHazard({ BossHazardType::EyeSector, QPointF(x, y), QRectF(),
                    190, eyeAngleDegrees, 90, 100, 0, 0, true, false });
        if (circleHitsPlayer(QPointF(x, y), 190, player))
            player.applySpeedReduction(0.2);
        return;
    }

    eyeSweepTimerMs -= frameMs;
    if (eyeSweepTimerMs <= 0) {
        eyeSweepRemainingMs = 30000;
        eyeSweepTimerMs = 60000;
        eyeAngleDegrees = angleTo(QPointF(x, y), player.worldPos());
    }
}

void TaliMonsterBoss::spawnClone()
{
    cloneSpawned = true;
    cloneAlive = true;
    cloneHp = 1200;
    clonePos = QPointF(x - 260, y);
    requestSpawn(BossSpawnType::TaliClone, clonePos, QPointF(), cloneHp, 50);
}

void TaliMonsterBoss::updateClone(Player& player, int frameMs)
{
    if (!cloneAlive) {
        if (cloneExplosionTimerMs > 0) {
            cloneExplosionTimerMs -= frameMs;
            if (cloneExplosionTimerMs <= 0) finishCloneExplosion(player);
        }
        return;
    }

    clonePos = stepToward(clonePos, player.worldPos(), 1.5f);
    if (circleHitsPlayer(clonePos, 80, player))
        player.applySpeedReduction(0.1);
}

void TaliMonsterBoss::startCloneExplosion()
{
    cloneAlive = false;
    cloneExplosionTimerMs = 2000;
    addHazard({ BossHazardType::CloneExplosionWarning, clonePos, QRectF(),
                170, 0, 0, 2000, 0, 0, true, false });
}

void TaliMonsterBoss::finishCloneExplosion(Player& player)
{
    addHazard({ BossHazardType::CloneExplosionHitbox, clonePos, QRectF(),
                170, 0, 0, 300, 0, 0, true, false });

    hp -= int(maxHp * 0.30f);
    if (QLineF(clonePos, QPointF(x, y)).length() <= 190)
        hp -= int(maxHp * 0.20f);
    if (circleHitsPlayer(clonePos, 170, player)) {
        player.maxDurability = std::max(1, int(player.maxDurability * 0.8f));
        player.maxStamina = std::max(1, int(player.maxStamina * 0.9f));
    }

    invulnerable = false;
    phase2InvulnerabilityEnded = true;
    applyShockStun(5000);
}

SirenBoss::SirenBoss(int startX, int startY)
    : Boss(BossType::Siren, startX, startY, 3000, 30, 1200)
{
    speed = 0.0f;
}

void SirenBoss::takeDamage(int damage)
{
    if (state == PHASE2) return;
    hp -= damage;
    if (hp > 0) return;

    setPhase(PHASE2);
    hp = 3000;
    maxHp = 3000;
    invulnerable = true;
    phantomSpawned = false;
}

void SirenBoss::updateBoss(Player& player)
{
    if (state == PHASE1) updatePhase1(player, FrameMs);
    else updatePhase2(player, FrameMs);
}

void SirenBoss::updatePhase1(Player& player, int frameMs)
{
    updateSoulSong(player, frameMs);
    updatePhantom(player, frameMs);
}

void SirenBoss::updatePhase2(Player& player, int frameMs)
{
    applyNaturalDecay();
    checkPhase2StaminaCheckpoints(player);
    updateSoulSong(player, frameMs);
    updateElegy(player, frameMs);
    updateEndlessReturn(player, frameMs);
    updateResonancePillars();

    if (poisonRemainingMs > 0) {
        poisonRemainingMs -= frameMs;
        player.applySpeedReduction(0.5);
        if (poisonRemainingMs % 1000 < frameMs)
            player.takeDurabilityDamage(20);
        if (poisonRemainingMs <= 0)
            player.maxStamina = std::max(1, int(player.maxStamina * 0.9f));
    }
}

void SirenBoss::updateSoulSong(Player& player, int frameMs)
{
    if (soulSongCastMs > 0) {
        soulSongCastMs -= frameMs;
        if (soulSongCastMs <= 0) {
            const QPointF target = player.worldPos();
            const QRectF beam(std::min((qreal)x, target.x()) - 25, std::min((qreal)y, target.y()) - 25,
                              std::abs(target.x() - x) + 50, std::abs(target.y() - y) + 50);
            addHazard({ BossHazardType::SoulSong, beam.center(), beam,
                        0, angleTo(QPointF(x, y), target), 0, 700, 0, 30, true, false });
            if (rectHitsPlayer(beam, player))
                player.takeDurabilityDamage(30);
            if (phantomSpawned && beam.contains(phantomPos))
                phantomStunMs += 5000;
        }
        return;
    }

    soulSongTimerMs -= frameMs;
    if (soulSongTimerMs <= 0) {
        soulSongCastMs = 2000;
        soulSongTimerMs = 20000;
    }
}

void SirenBoss::updatePhantom(Player& player, int frameMs)
{
    if (state != PHASE1) return;
    if (!phantomSpawned) {
        phantomSpawned = true;
        phantomPos = QPointF(x - 220, y);
        requestSpawn(BossSpawnType::SirenPhantom, phantomPos);
    }

    if (phantomStunMs > 0) {
        phantomStunMs -= frameMs;
        return;
    }

    phantomPos = stepToward(phantomPos, player.worldPos(), 2.2f);
    if (circleHitsPlayer(phantomPos, 36, player) && frameMs > 0)
        player.takeDurabilityDamage(1);
}

void SirenBoss::updateElegy(Player& player, int frameMs)
{
    if (elegyCastMs > 0) {
        elegyCastMs -= frameMs;
        player.applySpeedReduction(0.25);
        if (elegyCastMs <= 0) {
            // There is no input state for holding Space here yet, so this is a conservative failure path.
            poisonRemainingMs = 10000;
        }
        return;
    }

    elegyTimerMs -= frameMs;
    if (elegyTimerMs <= 0) {
        elegyCastMs = 3000;
        elegyTimerMs = 30000;
        addHazard({ BossHazardType::ElegyWarning, QPointF(x, y), QRectF(),
                    240, 0, 0, 3000, 0, 0, true, false });
    }
}

void SirenBoss::updateEndlessReturn(Player& player, int frameMs)
{
    endlessReturnTimerMs -= frameMs;
    if (endlessReturnTimerMs > 0) return;

    for (int i = 0; i < 10; ++i) {
        const int rx = int(player.worldPos().x()) - 260 + std::rand() % 520;
        const int ry = clampInt(int(player.worldPos().y()) - 180 + std::rand() % 360, ArenaTop, ArenaBottom);
        const QRectF reefRect(rx - 24, ry - 24, 48, 48);
        addHazard({ BossHazardType::ReefHitbox, QPointF(rx, ry), reefRect,
                    0, 0, 0, 20000, 0, 50, true, false });
    }

    endlessReturnTimerMs = 20000;
}

void SirenBoss::updateResonancePillars()
{
    addHazard({ BossHazardType::SeaweedZone, QPointF(x, y), QRectF(),
                90, 0, 0, 100, 0, 10, true, false });
}

void SirenBoss::applyNaturalDecay()
{
    naturalDecayAccumulatorMs += FrameMs;
    if (naturalDecayAccumulatorMs >= 1000) {
        naturalDecayAccumulatorMs -= 1000;
        hp -= 15;
        if (hp <= 0) {
            hp = 0;
            alive = false;
        }
    }
}

void SirenBoss::checkPhase2StaminaCheckpoints(Player& player)
{
    const float ratio = float(hp) / float(maxHp);
    if (!checkpoint75Used && ratio <= 0.75f) {
        checkpoint75Used = true;
        player.restoreStamina(player.maxStamina);
    }
    if (!checkpoint50Used && ratio <= 0.50f) {
        checkpoint50Used = true;
        player.restoreStamina(player.maxStamina);
    }
    if (!checkpoint25Used && ratio <= 0.25f) {
        checkpoint25Used = true;
        player.restoreStamina(player.maxStamina);
    }
}
