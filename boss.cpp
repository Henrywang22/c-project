#include "Boss.h"
#include "Player.h"
#include <QLineF>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr int FrameMs = 16;
constexpr int ArenaTop = 60;
constexpr int ArenaBottom = 700;

int clampInt(int value, int low, int high)
{
    return std::max(low, std::min(value, high));
}

QPointF stepToward(const QPointF& from, const QPointF& to, qreal amount)
{
    QPointF delta = to - from;
    qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (length <= 0.001) return from;
    return QPointF(from.x() + delta.x() / length * amount,
                   from.y() + delta.y() / length * amount);
}

bool rectHitsPlayer(const QRectF& rect, const Player& player)
{
    return rect.intersects(player.collider());
}

bool circleHitsPlayer(const QPointF& center, qreal radius, const Player& player)
{
    return QLineF(center, player.worldPos()).length() <= radius;
}
}

Boss::Boss(BossKind bossKind, int x, int y, int maxHpValue, int attackValue, int dropValueValue)
    : Enemy(x, y), kind(bossKind)
{
    hp = maxHpValue;
    maxHp = maxHpValue;
    attack = attackValue;
    dropValue = dropValueValue;
}

void Boss::update(Player& player)
{
    if (!alive) return;
    updateTimers();
    updateHazards();
    enraged = hp <= maxHp / 2;
    if (!stunned()) updateBoss(player);
}

bool Boss::collidesWithPlayer(int px, int py)
{
    int dx = px - x;
    int dy = py - y;
    return dx * dx + dy * dy < 40 * 40;
}

bool Boss::canBeHitAt(int targetX, int targetY) const
{
    if (!alive || invulnerable) return false;
    return std::abs(x - targetX) < 100 && std::abs(y - targetY) < 100;
}

void Boss::takeDamage(int damage)
{
    if (!alive || invulnerable) return;
    hp -= damage;
    if (hp <= 0) {
        hp = 0;
        alive = false;
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

bool Boss::getSecondaryTarget(QPointF& outPos, int& outHp, int& outMaxHp) const
{
    Q_UNUSED(outPos);
    Q_UNUSED(outHp);
    Q_UNUSED(outMaxHp);
    return false;
}

void Boss::spawnMinions(std::vector<Shark*>& sharks)
{
    for (const auto& request : sharkSpawnRequests) {
        sharks.push_back(new Shark((int)request.position.x(), (int)request.position.y()));
    }
    sharkSpawnRequests.clear();
    minionSpawned = true;
}

void Boss::updateTimers()
{
    stunRemainingMs = std::max(0, stunRemainingMs - FrameMs);
}

void Boss::updateHazards()
{
    for (auto& hazard : hazards) {
        hazard.elapsedMs += FrameMs;
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

void Boss::requestSharkSpawn(const QPointF& spawnPos)
{
    sharkSpawnRequests.push_back({ spawnPos });
}

int Boss::scaledDamage(int baseDamage) const
{
    return enraged ? int(baseDamage * 1.2f) : baseDamage;
}

FiveHeadSharkBoss::FiveHeadSharkBoss(int x, int y)
    : Boss(BossKind::FiveHeadShark, x, y, 2000, 50, 500)
{
    speed = 2.0f;
}

bool FiveHeadSharkBoss::collidesWithPlayer(int px, int py)
{
    int dx = px - x;
    int dy = py - y;
    return dx * dx + dy * dy < 42 * 42;
}

void FiveHeadSharkBoss::updateBoss(Player& player)
{
    updatePatrol();
    updateMelee(player);
    updateSummon();
    updateBombardment(player);
}

void FiveHeadSharkBoss::updatePatrol()
{
    y += int(speed * patrolDir);
    if (y <= ArenaTop + 40) {
        y = ArenaTop + 40;
        patrolDir = 1;
    } else if (y >= ArenaBottom - 40) {
        y = ArenaBottom - 40;
        patrolDir = -1;
    }
}

void FiveHeadSharkBoss::updateMelee(Player& player)
{
    meleeCooldownMs = std::max(0, meleeCooldownMs - FrameMs);
    meleeRecoveryMs = std::max(0, meleeRecoveryMs - FrameMs);

    if (meleeWindupMs > 0) {
        meleeWindupMs -= FrameMs;
        if (meleeWindupMs <= 0) {
            QRectF hitRect(x - 130, y - 42, 130, 84);
            addHazard({ BossHazardType::MeleeHitbox, hitRect.center(), hitRect, 0, 250, 0, scaledDamage(100), true });
            if (rectHitsPlayer(hitRect, player))
                player.takeDurabilityDamage(scaledDamage(100));
            meleeRecoveryMs = 600;
            meleeCooldownMs = 3000;
        }
        return;
    }

    if (meleeCooldownMs > 0 || meleeRecoveryMs > 0) return;

    QRectF triggerRect(x - 150, y - 55, 150, 110);
    if (rectHitsPlayer(triggerRect, player)) {
        meleeWindupMs = 500;
        addHazard({ BossHazardType::MeleeHitbox, triggerRect.center(), triggerRect, 0, 500, 0, 0, true });
    }
}

void FiveHeadSharkBoss::updateSummon()
{
    summonTimerMs -= FrameMs;
    if (summonTimerMs > 0) return;

    requestSharkSpawn(QPointF(x - 90, y));
    requestSharkSpawn(QPointF(x, y - 90));
    requestSharkSpawn(QPointF(x, y + 90));
    summonTimerMs = 20000;
    minionSpawned = false;
}

void FiveHeadSharkBoss::updateBombardment(Player& player)
{
    if (bombardmentCastMs > 0) {
        bombardmentCastMs -= FrameMs;
        if (bombardmentCastMs <= 0) {
            for (const QRectF& rect : pendingBombRects) {
                addHazard({ BossHazardType::BombHitbox, rect.center(), rect, 0, 300, 0, scaledDamage(150), true });
                if (rectHitsPlayer(rect, player))
                    player.takeDurabilityDamage(scaledDamage(150));
            }
            pendingBombRects.clear();
        }
        return;
    }

    bombardmentTimerMs -= FrameMs;
    if (bombardmentTimerMs > 0) return;

    pendingBombRects.clear();
    int px = int(player.worldPos().x());
    int py = int(player.worldPos().y());
    for (int i = 0; i < 10; ++i) {
        int bx = px - 350 + std::rand() % 700;
        int by = clampInt(py - 220 + std::rand() % 440, ArenaTop, ArenaBottom);
        pendingBombRects.push_back(QRectF(bx - 24, by - 24, 48, 48));
    }

    const int offsets[6][2] = {{-70, -55}, {-70, 55}, {-15, -75}, {-15, 75}, {45, -45}, {45, 45}};
    for (const auto& offset : offsets)
        pendingBombRects.push_back(QRectF(x + offset[0] - 24, y + offset[1] - 24, 48, 48));

    for (const QRectF& rect : pendingBombRects)
        addHazard({ BossHazardType::BombWarning, rect.center(), rect, 0, 1500, 0, 0, true });

    bombardmentCastMs = 1500;
    bombardmentTimerMs = 20000;
}

TaliMonsterBoss::TaliMonsterBoss(int x, int y)
    : Boss(BossKind::TaliMonster, x, y, 2250, 100, 800)
{
    speed = 0.8f;
}

bool TaliMonsterBoss::canBeHitAt(int targetX, int targetY) const
{
    if (!alive) return false;

    if (state == PHASE2 && invulnerable && cloneAlive) {
        return QLineF(clonePos, QPointF(targetX, targetY)).length() <= 80;
    }

    return Boss::canBeHitAt(targetX, targetY);
}

bool TaliMonsterBoss::getSecondaryTarget(QPointF& outPos, int& outHp, int& outMaxHp) const
{
    if (!(state == PHASE2 && cloneAlive)) {
        return false;
    }

    outPos = clonePos;
    outHp = cloneHp;
    outMaxHp = 1200;
    return true;
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
        state = PHASE2;
        hp = 2000;
        maxHp = 2000;
        invulnerable = true;
        enraged = false;
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
    if (state == PHASE1) updatePhase1(player);
    else updatePhase2(player);
}

void TaliMonsterBoss::updatePhase1(Player& player)
{
    updateMovement(player);
    updateMouthStrike(player);
    updateEyeSweep(player);
}

void TaliMonsterBoss::updatePhase2(Player& player)
{
    updateClone(player);
    updateMouthStrike(player);
    updateEyeSweep(player);
}

void TaliMonsterBoss::updateMovement(Player& player)
{
    if (state == PHASE2) return;
    QPointF next = stepToward(position(), player.worldPos(), speed);
    x = int(next.x());
    y = clampInt(int(next.y()), ArenaTop, ArenaBottom);
}

void TaliMonsterBoss::updateMouthStrike(Player& player)
{
    if (mouthSequenceTimerMs > 0) {
        mouthSequenceTimerMs -= FrameMs;
        if (mouthSequenceTimerMs > 0) return;

        ++mouthStrikeIndex;
        bool grabStrike = mouthStrikeIndex >= 4;
        int stabDamage = phase2InvulnerabilityEnded ? 120 : 100;
        QPointF target = player.worldPos();
        QRectF hitRect(std::min((qreal)x, target.x()) - 20,
                       std::min((qreal)y, target.y()) - 20,
                       std::abs(target.x() - x) + 40,
                       std::abs(target.y() - y) + 40);
        addHazard({ BossHazardType::MouthStrike, hitRect.center(), hitRect, 0, grabStrike ? 300.0 : 250.0, 0, stabDamage, true });
        if (rectHitsPlayer(hitRect, player)) {
            player.takeDurabilityDamage(stabDamage);
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

    mouthTimerMs -= FrameMs;
    if (mouthTimerMs <= 0) {
        mouthStrikeIndex = 0;
        mouthSequenceTimerMs = 550;
    }
}

void TaliMonsterBoss::updateEyeSweep(Player& player)
{
    if (eyeSweepRemainingMs > 0) {
        eyeSweepRemainingMs -= FrameMs;
        addHazard({ BossHazardType::EyeSector, position(), QRectF(), 190, 100, 0, 0, true });
        if (circleHitsPlayer(position(), 190, player))
            player.applySpeedReduction(0.2);
        return;
    }

    eyeSweepTimerMs -= FrameMs;
    if (eyeSweepTimerMs <= 0) {
        eyeSweepRemainingMs = 30000;
        eyeSweepTimerMs = 60000;
    }
}

void TaliMonsterBoss::spawnClone()
{
    cloneSpawned = true;
    cloneAlive = true;
    cloneHp = 1200;
    clonePos = QPointF(x - 240, y);
}

void TaliMonsterBoss::updateClone(Player& player)
{
    if (!cloneAlive) {
        if (cloneExplosionTimerMs > 0) {
            cloneExplosionTimerMs -= FrameMs;
            if (cloneExplosionTimerMs <= 0) finishCloneExplosion(player);
        }
        return;
    }

    clonePos = stepToward(clonePos, player.worldPos(), 1.5);
    if (circleHitsPlayer(clonePos, 80, player))
        player.applySpeedReduction(0.1);
}

void TaliMonsterBoss::startCloneExplosion()
{
    cloneAlive = false;
    cloneExplosionTimerMs = 2000;
    addHazard({ BossHazardType::CloneExplosionWarning, clonePos, QRectF(), 170, 2000, 0, 0, true });
}

void TaliMonsterBoss::finishCloneExplosion(Player& player)
{
    hp -= int(maxHp * 0.30f);
    if (QLineF(clonePos, position()).length() <= 190)
        hp -= int(maxHp * 0.20f);
    if (circleHitsPlayer(clonePos, 170, player)) {
        player.maxDurability = std::max(1, int(player.maxDurability * 0.8f));
        player.maxStamina = std::max(1, int(player.maxStamina * 0.9f));
        // 上限降低后同步 clamp 当前值，防止超出新上限
        player.restoreDurability(0);
        player.restoreStamina(0);
    }
    invulnerable = false;
    phase2InvulnerabilityEnded = true;
    applyShockStun(5000);
}

SirenBoss::SirenBoss(int x, int y)
    : Boss(BossKind::Siren, x, y, 3000, 30, 1200)
{
    speed = 0.0f;
}

bool SirenBoss::canBeHitAt(int targetX, int targetY) const
{
    if (state == PHASE2) return false;
    return Boss::canBeHitAt(targetX, targetY);
}

void SirenBoss::takeDamage(int damage)
{
    if (state == PHASE2) return;

    hp -= damage;
    if (hp > 0) return;

    state = PHASE2;
    hp = 3000;
    maxHp = 3000;
    invulnerable = true;
    phantomSpawned = false;
}

void SirenBoss::updateBoss(Player& player)
{
    if (state == PHASE1) updatePhase1(player);
    else updatePhase2(player);
}

void SirenBoss::updatePhase1(Player& player)
{
    updateSoulSong(player);
    updatePhantom(player);
}

void SirenBoss::updatePhase2(Player& player)
{
    applyNaturalDecay();
    checkStaminaCheckpoints(player);
    updateSoulSong(player);
    updateElegy(player);
    updateEndlessReturn(player);

    addHazard({ BossHazardType::SeaweedZone, position(), QRectF(), 90, 100, 0, 10, true });
    if (circleHitsPlayer(position(), 90, player))
        player.takeDurabilityDamage(1);

    if (poisonRemainingMs > 0) {
        poisonRemainingMs -= FrameMs;
        player.applySpeedReduction(0.5);
        if (poisonRemainingMs % 1000 < FrameMs)
            player.takeDurabilityDamage(20);
        if (poisonRemainingMs <= 0)
            player.maxStamina = std::max(1, int(player.maxStamina * 0.9f));
    }
}

void SirenBoss::updateSoulSong(Player& player)
{
    if (soulSongCastMs > 0) {
        soulSongCastMs -= FrameMs;
        if (soulSongCastMs <= 0) {
            QPointF target = player.worldPos();
            QRectF beam(std::min((qreal)x, target.x()) - 25,
                        std::min((qreal)y, target.y()) - 25,
                        std::abs(target.x() - x) + 50,
                        std::abs(target.y() - y) + 50);
            addHazard({ BossHazardType::SoulSong, beam.center(), beam, 0, 700, 0, 30, true });
            if (rectHitsPlayer(beam, player))
                player.takeDurabilityDamage(30);
            if (phantomSpawned && beam.contains(phantomPos))
                phantomStunMs += 5000;
        }
        return;
    }

    soulSongTimerMs -= FrameMs;
    if (soulSongTimerMs <= 0) {
        soulSongCastMs = 2000;
        soulSongTimerMs = 20000;
    }
}

void SirenBoss::updatePhantom(Player& player)
{
    if (!phantomSpawned) {
        phantomSpawned = true;
        phantomPos = QPointF(x - 220, y);
    }
    if (phantomStunMs > 0) {
        phantomStunMs -= FrameMs;
        return;
    }
    phantomPos = stepToward(phantomPos, player.worldPos(), 2.2);
    if (circleHitsPlayer(phantomPos, 36, player))
        player.takeDurabilityDamage(1);
}

void SirenBoss::updateElegy(Player& player)
{
    if (elegyCastMs > 0) {
        elegyCastMs -= FrameMs;
        player.applySpeedReduction(0.25);
        if (elegyCastMs <= 0)
            poisonRemainingMs = 10000;
        return;
    }

    elegyTimerMs -= FrameMs;
    if (elegyTimerMs <= 0) {
        elegyCastMs = 3000;
        elegyTimerMs = 30000;
        addHazard({ BossHazardType::ElegyWarning, position(), QRectF(), 240, 3000, 0, 0, true });
    }
}

void SirenBoss::updateEndlessReturn(Player& player)
{
    endlessReturnTimerMs -= FrameMs;
    if (endlessReturnTimerMs > 0) return;

    int px = int(player.worldPos().x());
    int py = int(player.worldPos().y());
    for (int i = 0; i < 10; ++i) {
        int rx = px - 260 + std::rand() % 520;
        int ry = clampInt(py - 180 + std::rand() % 360, ArenaTop, ArenaBottom);
        QRectF reefRect(rx - 24, ry - 24, 48, 48);
        addHazard({ BossHazardType::ReefHitbox, QPointF(rx, ry), reefRect, 0, 20000, 0, 50, true });
        if (rectHitsPlayer(reefRect, player))
            player.takeDurabilityDamage(50);
    }

    endlessReturnTimerMs = 20000;
}

void SirenBoss::applyNaturalDecay()
{
    naturalDecayTimerMs += FrameMs;
    if (naturalDecayTimerMs < 1000) return;
    naturalDecayTimerMs -= 1000;
    hp -= 15;
    if (hp <= 0) {
        hp = 0;
        alive = false;
    }
}

void SirenBoss::checkStaminaCheckpoints(Player& player)
{
    float ratio = float(hp) / float(maxHp);
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
