#include "Boss.h"
#include "Player.h"
#include "GameConfig.h"
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

qreal clampReal(qreal value, qreal low, qreal high)
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

QPointF normalizedOr(const QPointF& value, const QPointF& fallback)
{
    const qreal length = std::sqrt(value.x() * value.x() + value.y() * value.y());
    if (length <= 0.001) return fallback;
    return QPointF(value.x() / length, value.y() / length);
}

QRectF beamBounds(const QPointF& from, const QPointF& to, qreal halfWidth)
{
    return QRectF(QPointF(std::min(from.x(), to.x()) - halfWidth,
                         std::min(from.y(), to.y()) - halfWidth),
                  QPointF(std::max(from.x(), to.x()) + halfWidth,
                         std::max(from.y(), to.y()) + halfWidth)).normalized();
}

qreal distancePointToSegment(const QPointF& point, const QPointF& from, const QPointF& to)
{
    const QPointF segment = to - from;
    const qreal lengthSq = segment.x() * segment.x() + segment.y() * segment.y();
    if (lengthSq <= 0.001)
        return QLineF(point, from).length();

    const QPointF relative = point - from;
    const qreal t = clampReal((relative.x() * segment.x() + relative.y() * segment.y()) /
                                  lengthSq,
                              0.0, 1.0);
    const QPointF projected(from.x() + segment.x() * t,
                            from.y() + segment.y() * t);
    return QLineF(point, projected).length();
}

qreal projectionOnSegment(const QPointF& point, const QPointF& from, const QPointF& to)
{
    const QPointF segment = to - from;
    const qreal lengthSq = segment.x() * segment.x() + segment.y() * segment.y();
    if (lengthSq <= 0.001)
        return 0.0;

    const QPointF relative = point - from;
    return clampReal((relative.x() * segment.x() + relative.y() * segment.y()) /
                         lengthSq,
                     0.0, 1.0);
}

bool segmentIntersectsRect(const QPointF& from, const QPointF& to, const QRectF& rect)
{
    if (rect.contains(from) || rect.contains(to)) return true;

    QLineF line(from, to);
    const QLineF edges[4] = {
        QLineF(rect.topLeft(), rect.topRight()),
        QLineF(rect.topRight(), rect.bottomRight()),
        QLineF(rect.bottomRight(), rect.bottomLeft()),
        QLineF(rect.bottomLeft(), rect.topLeft())
    };
    QPointF hit;
    for (const QLineF& edge : edges) {
        if (line.intersects(edge, &hit) == QLineF::BoundedIntersection)
            return true;
    }
    return false;
}

bool segmentHitsPlayer(const QPointF& from, const QPointF& to, qreal halfWidth,
                       const Player& player)
{
    const QRectF rect = player.collider();
    if (!beamBounds(from, to, halfWidth).intersects(rect))
        return false;
    if (segmentIntersectsRect(from, to, rect))
        return true;

    const QPointF samples[8] = {
        rect.topLeft(), rect.topRight(), rect.bottomLeft(), rect.bottomRight(),
        QPointF(rect.center().x(), rect.top()),
        QPointF(rect.center().x(), rect.bottom()),
        QPointF(rect.left(), rect.center().y()),
        QPointF(rect.right(), rect.center().y())
    };
    for (const QPointF& sample : samples) {
        if (distancePointToSegment(sample, from, to) <= halfWidth)
            return true;
    }
    return false;
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
    if (dying) return;
    enraged = hp <= maxHp / 2;
    if (kind == BossKind::FiveHeadShark)
        state = enraged ? PHASE2 : PHASE1;
    if (!stunned()) updateBoss(player);
}

bool Boss::collidesWithPlayer(int px, int py)
{
    QRectF playerRect(
        px - Config::GameConfig::PLAYER_COLLIDER_WIDTH / 2.0,
        py - Config::GameConfig::PLAYER_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::PLAYER_COLLIDER_WIDTH,
        Config::GameConfig::PLAYER_COLLIDER_HEIGHT
    );
    return collider().intersects(playerRect);
}

QRectF Boss::collider() const
{
    return QRectF(
        x - Config::GameConfig::BOSS_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::BOSS_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::BOSS_COLLIDER_WIDTH,
        Config::GameConfig::BOSS_COLLIDER_HEIGHT
    );
}

bool Boss::canBeHitAt(int targetX, int targetY) const
{
    if (!alive || invulnerable) return false;
    return collider().contains(QPointF(targetX, targetY));
}

void Boss::takeDamage(int damage)
{
    if (!alive || invulnerable) return;
    hp -= damage;
    if (hp <= 0) {
        hp = 0;
        startDeathAnimation();
    } else {
        setVisualAction(BossVisualAction::Hit, 240);
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

bool Boss::getCompanionVisual(QPointF& outPos, bool& outStunned) const
{
    Q_UNUSED(outPos);
    Q_UNUSED(outStunned);
    return false;
}

void Boss::spawnMinions(std::vector<Shark*>& sharks)
{
    if (sharkSpawnRequests.empty()) return;
    const int spawnLimit = kind == BossKind::FiveHeadShark
        ? static_cast<int>(sharkSpawnRequests.size())
        : static_cast<int>(sharkSpawnRequests.size());
    int spawned = 0;
    for (const auto& request : sharkSpawnRequests) {
        if (spawned >= spawnLimit) break;
        sharks.push_back(new Shark((int)request.position.x(), (int)request.position.y()));
        ++spawned;
    }
    sharkSpawnRequests.clear();
    minionSpawned = true;
}

void Boss::updateTimers()
{
    stunRemainingMs = std::max(0, stunRemainingMs - FrameMs);
    if (visualActionRemainingMs > 0) {
        visualActionRemainingMs = std::max(0, visualActionRemainingMs - FrameMs);
        if (visualActionRemainingMs == 0 && !dying)
            visualActionValue = BossVisualAction::Idle;
    }
    if (dying) {
        deathRemainingMs = std::max(0, deathRemainingMs - FrameMs);
        if (deathRemainingMs == 0)
            alive = false;
    }
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

void Boss::setVisualAction(BossVisualAction action, int durationMs)
{
    if (dying) return;
    visualActionValue = action;
    visualActionDurationMs = std::max(1, durationMs);
    visualActionRemainingMs = visualActionDurationMs;
}

qreal Boss::visualActionProgress() const
{
    if (visualActionDurationMs <= 0) return 0.0;
    return std::clamp(
        1.0 - static_cast<qreal>(visualActionRemainingMs) / visualActionDurationMs,
        0.0,
        1.0
    );
}

void Boss::startDeathAnimation()
{
    if (dying) return;
    dying = true;
    invulnerable = true;
    hazards.clear();
    visualActionValue = BossVisualAction::Death;
    visualActionDurationMs = 1760;
    visualActionRemainingMs = 1760;
    deathRemainingMs = 1760;
}

FiveHeadSharkBoss::FiveHeadSharkBoss(int x, int y)
    : Boss(BossKind::FiveHeadShark, x, y, 2900, 18, 700)
{
    speed = 2.0f;
}

bool FiveHeadSharkBoss::collidesWithPlayer(int px, int py)
{
    return Boss::collidesWithPlayer(px, py);
}

QRectF FiveHeadSharkBoss::collider() const
{
    return QRectF(x - 170.0, y - 92.0, 340.0, 184.0);
}

void FiveHeadSharkBoss::updateBoss(Player& player)
{
    const QPointF playerPos = player.worldPos();
    if (hasLastPlayerPos) {
        estimatedPlayerVelocity = playerPos - lastPlayerPos;
    }
    lastPlayerPos = playerPos;
    hasLastPlayerPos = true;

    contactCooldownMs = std::max(0, contactCooldownMs - FrameMs);
    if (collider().intersects(player.collider()) && contactCooldownMs <= 0 &&
        player.canTakeDamage()) {
        player.takeDurabilityDamage(scaledDamage(15));
        player.applyRebound(normalizedOr(player.worldPos() - position(), QPointF(facingX, 0.0)) * 1.4);
        contactCooldownMs = 900;
    }
    const qreal distanceToPlayer = QLineF(position(), playerPos).length();
    const qreal playerFrameSpeed = std::hypot(estimatedPlayerVelocity.x(), estimatedPlayerVelocity.y());
    if (distanceToPlayer > 520.0) {
        bombardmentTimerMs = std::max(0, bombardmentTimerMs - FrameMs * (enraged ? 2 : 1));
    }
    if (distanceToPlayer > 620.0 || playerFrameSpeed > 3.2) {
        summonTimerMs = std::max(0, summonTimerMs - FrameMs);
    }
    if (distanceToPlayer < 360.0 && std::abs(playerPos.y() - y) < 150.0) {
        meleeCooldownMs = std::max(0, meleeCooldownMs - FrameMs);
    }
    updatePatrol(player);
    updateMelee(player);
    updateSummon(player);
    updateBombardment(player);
}

void FiveHeadSharkBoss::updatePatrol(Player& player)
{
    const QPointF playerPos = player.worldPos();
    const qreal dx = playerPos.x() - x;
    const qreal dy = playerPos.y() - y;
    const qreal distance = std::sqrt(dx * dx + dy * dy);

    if (std::abs(dx) > 6.0)
        facingX = dx > 0.0 ? 1.0f : -1.0f;

    const qreal closingBoost = distance > 520.0 ? 1.42 : 1.0;
    const qreal maxHorizontalStep = speed * (enraged ? 2.45 : 2.05) * closingBoost;
    // The solid body keeps the player roughly 200 px from the boss centre.
    // Stay at a usable bite distance instead of trying to overlap the player.
    const qreal desiredGap = 218.0;
    const qreal desiredDx = facingX * desiredGap;
    const qreal gapError = dx - desiredDx;
    if (std::abs(gapError) > 34.0) {
        const bool needsCatchUp = std::abs(gapError) > 150.0 || distance > 520.0;
        const qreal playerStep = player.currentSpeed() * (FrameMs / 1000.0);
        const qreal catchUpStep = needsCatchUp
            ? std::min<qreal>(7.0, std::max(maxHorizontalStep, playerStep * (enraged ? 1.20 : 1.10)))
            : maxHorizontalStep;
        const qreal response = needsCatchUp ? 0.034 : 0.022;
        const qreal step = std::clamp(gapError * response, -catchUpStep, catchUpStep);
        x += static_cast<int>(std::round(step));
    }

    const qreal patrolStep = speed * 0.42 * patrolDir;
    const qreal verticalAggression = distance < 340.0 ? 0.020 : 0.014;
    const qreal chaseBias = std::clamp(dy * verticalAggression, -speed * 1.18, speed * 1.18);
    y += static_cast<int>(std::round(patrolStep + chaseBias));

    x = clampInt(x, 125, Config::GameConfig::RIGHT_BORDER - 125);
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
    const auto biteRect = [&]() {
        // The old 155 px box ended inside the 170 px body collider, so solid
        // separation made a bite practically impossible.  This now covers the
        // visible heads and the forward slash in the supplied animation.
        const qreal reach = 292.0;
        const qreal height = 176.0;
        return facingX > 0.0f
            ? QRectF(x, y - height / 2.0, reach, height)
            : QRectF(x - reach, y - height / 2.0, reach, height);
    };

    meleeCooldownMs = std::max(0, meleeCooldownMs - FrameMs);
    meleeRecoveryMs = std::max(0, meleeRecoveryMs - FrameMs);

    if (meleeWindupMs > 0) {
        meleeWindupMs -= FrameMs;
        if (meleeWindupMs > 210) {
            const QPointF toward = normalizedOr(player.worldPos() - position(), QPointF(facingX, 0.0));
            x += static_cast<int>(std::round(toward.x() * (enraged ? 2.6 : 2.0)));
            y += static_cast<int>(std::round(toward.y() * (enraged ? 1.6 : 1.2)));
            x = clampInt(x, 125, Config::GameConfig::RIGHT_BORDER - 125);
            y = clampInt(y, ArenaTop + 40, ArenaBottom - 40);
        }
        if (meleeWindupMs <= 0) {
            QRectF hitRect = biteRect();
            addHazard({ BossHazardType::MeleeHitbox, hitRect.center(), hitRect, 0, 250, 0, scaledDamage(30), true });
            if (rectHitsPlayer(hitRect, player) && player.canTakeDamage()) {
                player.takeDurabilityDamage(scaledDamage(30));
                player.applyRebound(normalizedOr(player.worldPos() - position(), QPointF(facingX, 0.0)) * 3.0);
            }
            meleeRecoveryMs = 600;
            meleeCooldownMs = enraged ? 2300 : 2850;
        }
        return;
    }

    if (meleeCooldownMs > 0 || meleeRecoveryMs > 0) return;

    QRectF triggerRect = biteRect();
    const QPointF predictedPlayer = player.worldPos() + estimatedPlayerVelocity * 8.0;
    const QRectF predictiveRect = triggerRect.adjusted(-34.0, -24.0, 34.0, 24.0);
    if (rectHitsPlayer(triggerRect, player) || predictiveRect.contains(predictedPlayer)) {
        meleeWindupMs = enraged ? 430 : 500;
        setVisualAction(BossVisualAction::Bite, 1120);
        addHazard({ BossHazardType::MeleeHitbox, triggerRect.center(), triggerRect, 0,
                    static_cast<qreal>(meleeWindupMs), 0, 0, true });
    }
}

void FiveHeadSharkBoss::updateSummon(Player& player)
{
    if (state == PHASE2 && !phase2SummonPrimed) {
        phase2SummonPrimed = true;
        summonTimerMs = 1900;
    }

    bool& phaseSummonUsed = state == PHASE2 ? phase2SummonUsed : phase1SummonUsed;
    if (phaseSummonUsed) return;

    const QPointF playerPos = player.worldPos();
    const qreal distanceToPlayer = QLineF(position(), playerPos).length();
    const qreal playerFrameSpeed = std::hypot(estimatedPlayerVelocity.x(), estimatedPlayerVelocity.y());
    summonTimerMs -= FrameMs * ((distanceToPlayer > 560.0 || playerFrameSpeed > 3.0) ? 2 : 1);
    if (summonTimerMs > 0) return;

    const QPointF retreatDir = normalizedOr(estimatedPlayerVelocity, QPointF(facingX, 0.0));
    const qreal flankY = playerPos.y() < y ? 120.0 : -120.0;
    const QPointF side(-retreatDir.y(), retreatDir.x());
    const QPointF spawnPoints[4] = {
        QPointF(x + facingX * 130.0, y),
        QPointF(playerPos.x() + retreatDir.x() * 180.0,
                clampInt(qRound(playerPos.y() + flankY), ArenaTop + 55, ArenaBottom - 55)),
        QPointF(playerPos.x() - facingX * 210.0,
                clampInt(qRound(playerPos.y() - flankY * 0.55), ArenaTop + 55, ArenaBottom - 55)),
        QPointF(playerPos.x() + side.x() * 245.0,
                clampInt(qRound(playerPos.y() + side.y() * 245.0), ArenaTop + 55, ArenaBottom - 55))
    };
    for (const QPointF& point : spawnPoints) {
        QPointF clamped(
            std::clamp(point.x(), 95.0, static_cast<qreal>(Config::GameConfig::RIGHT_BORDER - 95)),
            std::clamp(point.y(), static_cast<qreal>(ArenaTop + 55), static_cast<qreal>(ArenaBottom - 55))
        );
        requestSharkSpawn(clamped);
        addHazard({ BossHazardType::SummonMarker, clamped,
                    QRectF(clamped.x() - 56.0, clamped.y() - 56.0, 112.0, 112.0),
                    0, 1300, 0, 0, true });
    }
    setVisualAction(BossVisualAction::Summon, 1200);
    phaseSummonUsed = true;
    minionSpawned = false;
}

void FiveHeadSharkBoss::updateBombardment(Player& player)
{
    if (bombardmentCastMs > 0) {
        bombardmentCastMs -= FrameMs;
        if (bombardmentCastMs <= 0) {
            for (const QRectF& rect : pendingBombRects) {
                addHazard({ BossHazardType::BombHitbox, rect.center(), rect, 0, 360, 0, scaledDamage(40), true });
                if (rectHitsPlayer(rect, player))
                    player.takeDurabilityDamage(scaledDamage(40));
            }
            pendingBombRects.clear();
        }
        return;
    }

    bombardmentTimerMs -= FrameMs;
    if (bombardmentTimerMs > 0) return;

    pendingBombRects.clear();
    const qreal playerFrameSpeed = std::hypot(estimatedPlayerVelocity.x(), estimatedPlayerVelocity.y());
    const qreal leadFrames = playerFrameSpeed > 2.6 ? 24.0 : 18.0;
    QPointF predicted = player.worldPos() + estimatedPlayerVelocity * leadFrames;
    predicted.setX(std::clamp(predicted.x(), 120.0, static_cast<qreal>(Config::GameConfig::RIGHT_BORDER - 120)));
    predicted.setY(std::clamp(predicted.y(), static_cast<qreal>(ArenaTop + 70), static_cast<qreal>(ArenaBottom - 70)));
    const QPointF travelDir = normalizedOr(estimatedPlayerVelocity, QPointF(facingX, 0.0));
    const QPointF perpendicular(-travelDir.y(), travelDir.x());
    const QPointF escapeDir = normalizedOr(
        estimatedPlayerVelocity * 0.85 +
            normalizedOr(player.worldPos() - position(), QPointF(facingX, 0.0)) * 0.35,
        travelDir);
    const QPointF tacticalPoints[5] = {
        predicted,
        predicted + escapeDir * 155.0,
        predicted - escapeDir * 185.0,
        predicted + perpendicular * 150.0,
        predicted - perpendicular * 150.0
    };
    for (const QPointF& point : tacticalPoints) {
        const int bx = clampInt(qRound(point.x()), 92, Config::GameConfig::RIGHT_BORDER - 92);
        const int by = clampInt(qRound(point.y()), ArenaTop + 70, ArenaBottom - 70);
        pendingBombRects.push_back(QRectF(bx - 68, by - 68, 136, 136));
    }

    const int offsets[4][2] = {{-82, -58}, {-82, 58}, {20, -72}, {20, 72}};
    for (const auto& offset : offsets)
        pendingBombRects.push_back(QRectF(x + offset[0] - 68, y + offset[1] - 68, 136, 136));

    for (const QRectF& rect : pendingBombRects)
        addHazard({ BossHazardType::BombWarning, rect.center(), rect, 0, 1500, 0, 0, true });

    bombardmentCastMs = 1500;
    bombardmentTimerMs = enraged ? 10500 : 13000;
    setVisualAction(BossVisualAction::Cast, 1500);
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
        const QRectF cloneRect(
            clonePos.x() - Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH / 2.0,
            clonePos.y() - Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT / 2.0,
            Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH,
            Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT
        );
        return cloneRect.contains(QPointF(targetX, targetY));
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
    auto addMouthWarning = [&]() {
        QPointF target = player.worldPos();
        QRectF warnRect(std::min((qreal)x, target.x()) - 20,
                        std::min((qreal)y, target.y()) - 20,
                        std::abs(target.x() - x) + 40,
                        std::abs(target.y() - y) + 40);
        addHazard({ BossHazardType::MouthStrike, warnRect.center(), warnRect, 0,
                    (qreal)mouthSequenceTimerMs, 0, 0, true });
    };

    if (mouthSequenceTimerMs > 0) {
        mouthSequenceTimerMs -= FrameMs;
        if (mouthSequenceTimerMs > 0) return;

        ++mouthStrikeIndex;
        bool grabStrike = mouthStrikeIndex >= 4;
        int stabDamage = phase2InvulnerabilityEnded ? 50 : 40;
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
                player.takeDurabilityDamage(phase2InvulnerabilityEnded ? 75 : 60);
            }
        }

        if (grabStrike) {
            mouthStrikeIndex = 0;
            mouthTimerMs = (state == PHASE2 && invulnerable) ? 23000 : 15000;
            mouthSequenceTimerMs = 0;
        } else {
            mouthSequenceTimerMs = 650;
            addMouthWarning();
        }
        return;
    }

    mouthTimerMs -= FrameMs;
    if (mouthTimerMs <= 0) {
        mouthStrikeIndex = 0;
        mouthSequenceTimerMs = 850;
        addMouthWarning();
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
    : Boss(BossKind::Siren, x, y, 4200, 30, 1500)
{
    speed = 1.15f;
}

QRectF SirenBoss::collider() const
{
    return QRectF(x - 96.0, y - 132.0, 192.0, 264.0);
}

bool SirenBoss::canBeHitAt(int targetX, int targetY) const
{
    if (state == PHASE2) return false;
    return Boss::canBeHitAt(targetX, targetY);
}

void SirenBoss::takeDamage(int damage)
{
    if (!alive || dying || state == PHASE2 || invulnerable) return;

    hp -= damage;
    if (hp > 0) {
        setVisualAction(BossVisualAction::Hit, 240);
        return;
    }

    state = PHASE2;
    hp = 4400;
    maxHp = 4400;
    invulnerable = true;
    phantomSpawned = false;
    naturalDecayTimerMs = 0;
    phaseMotionTimerMs = 0;
    resonancePillarsPlaced = false;
    resonanceVisualRefreshMs = 0;
    for (int i = 0; i < 3; ++i) {
        resonancePillarCharges[i] = 0;
        resonancePillarBurstMs[i] = 0;
        resonancePillarDestroyed[i] = false;
        elegyResonanceTriggered[i] = false;
        staminaCheckpointUsed[i] = false;
    }
    elegyHoldMs = 0;
    elegySucceeded = false;
    soulSongTimerMs = 3600;
    elegyTimerMs = 7200;
    phaseTransitionMs = 1280;
    setVisualAction(BossVisualAction::PhaseTransition, phaseTransitionMs);
}

void SirenBoss::applyShockStun(int durationMs)
{
    Boss::applyShockStun(durationMs);
    if (phantomSpawned) {
        phantomStunMs = std::max(phantomStunMs, durationMs + 600);
    }
}

bool SirenBoss::getCompanionVisual(QPointF& outPos, bool& outStunned) const
{
    if (state != PHASE1 || !phantomSpawned || dying) return false;
    outPos = phantomPos;
    outStunned = phantomStunMs > 0;
    return true;
}

void SirenBoss::updateBoss(Player& player)
{
    const QPointF playerPos = player.worldPos();
    if (hasLastPlayerPos)
        estimatedPlayerVelocity = playerPos - lastPlayerPos;
    lastPlayerPos = playerPos;
    hasLastPlayerPos = true;

    if (std::abs(playerPos.x() - x) > 8.0)
        facingX = playerPos.x() >= x ? 1.0f : -1.0f;

    if (state == PHASE1) updatePhase1(player);
    else updatePhase2(player);
}

QPointF SirenBoss::tridentTipWorld() const
{
    const qreal tipOffsetX = facingX > 0.0f ? 86.0 : -86.0;
    return QPointF(x + tipOffsetX, y - 145.0);
}

void SirenBoss::updateMovement(Player& player)
{
    const QPointF playerPos = player.worldPos();
    const QPointF predictedPlayer = playerPos + estimatedPlayerVelocity *
        (state == PHASE2 ? 18.0 : 12.0);
    const QPointF toPlayer = predictedPlayer - position();
    const qreal distance = QLineF(position(), playerPos).length();
    const QPointF forward = normalizedOr(toPlayer, QPointF(facingX, 0.0));
    const QPointF side(-forward.y(), forward.x());
    const QPointF travel = normalizedOr(estimatedPlayerVelocity, forward);
    const QPointF travelSide(-travel.y(), travel.x());

    const bool castingElegy = state == PHASE2 && elegyCastMs > 0;
    const qreal preferredMin = castingElegy ? 205.0 : (state == PHASE2 ? 390.0 : 300.0);
    const qreal preferredMax = castingElegy ? 315.0 : (state == PHASE2 ? 590.0 : 470.0);
    QPointF move(0.0, 0.0);
    if (distance < preferredMin)
        move -= QPointF(forward.x() * (preferredMin - distance) * 0.018,
                        forward.y() * (preferredMin - distance) * 0.018);
    else if (distance > preferredMax)
        move += QPointF(forward.x() * (distance - preferredMax) * 0.014,
                        forward.y() * (distance - preferredMax) * 0.014);

    const qreal orbit = std::sin((x + y + phaseMotionTimerMs) * 0.018) *
                        (state == PHASE2 ? 1.25 : 0.85);
    move += QPointF(side.x() * orbit, side.y() * orbit);
    const qreal interceptBias = state == PHASE2 ? 1.05 : 0.62;
    move += QPointF(travelSide.x() * interceptBias,
                    travelSide.y() * interceptBias);
    move.ry() += clampReal((playerPos.y() - y) * 0.006, -0.8, 0.8);

    if (state == PHASE2 && resonancePillarsPlaced) {
        for (int i = 0; i < 3; ++i) {
            if (resonancePillarDestroyed[i]) continue;
            QPointF away = position() - resonancePillarPositions[i];
            const qreal pillarDistance = std::hypot(away.x(), away.y());
            if (pillarDistance > 0.001 && pillarDistance < 260.0) {
                const qreal repulsion = (260.0 - pillarDistance) * 0.020;
                move += QPointF(away.x() / pillarDistance * repulsion,
                                away.y() / pillarDistance * repulsion);
            }
        }
    }

    const qreal maxStep = speed * (castingElegy ? 2.05 : (state == PHASE2 ? 1.62 : 1.22));
    x += qRound(clampReal(move.x(), -maxStep, maxStep));
    y += qRound(clampReal(move.y(), -maxStep, maxStep));
    x = clampInt(x, 150, Config::GameConfig::RIGHT_BORDER - 150);
    y = clampInt(y, ArenaTop + 125, ArenaBottom - 88);
}

void SirenBoss::updatePhase1(Player& player)
{
    updateMovement(player);
    updateSoulSong(player);
    updatePhantom(player);
}

void SirenBoss::updatePhase2(Player& player)
{
    phantomSpawned = false;
    if (phaseTransitionMs > 0) {
        phaseTransitionMs = std::max(0, phaseTransitionMs - FrameMs);
        return;
    }
    phaseMotionTimerMs += FrameMs;
    applyNaturalDecay();
    restorePhaseCheckpoint(player);
    if (dying) return;
    updateMovement(player);
    if (elegyCastMs > 0 && std::abs(player.worldPos().x() - x) > 8.0)
        facingX = player.worldPos().x() >= x ? 1.0f : -1.0f;
    updateResonancePillars(player);
    restorePhaseCheckpoint(player);
    if (elegyCastMs <= 0 && elegyTimerMs > FrameMs)
        updateSoulSong(player);
    else
        soulSongTimerMs = std::max(soulSongTimerMs, 800);
    updateElegy(player);
    restorePhaseCheckpoint(player);
    updateEndlessReturn(player);
    resolveResonancePillarCollision(player);

    seaweedFieldTimerMs = std::max(0, seaweedFieldTimerMs - FrameMs);
    if (seaweedFieldTimerMs <= 0) {
        const QPointF velocityDir = normalizedOr(
            estimatedPlayerVelocity,
            normalizedOr(player.worldPos() - position(), QPointF(facingX, 0.0)));
        const QPointF side(-velocityDir.y(), velocityDir.x());
        const QPointF predicted(player.worldPos().x() + estimatedPlayerVelocity.x() * 22.0,
                                player.worldPos().y() + estimatedPlayerVelocity.y() * 22.0);
        const QPointF center(
            clampReal(predicted.x() + velocityDir.x() * 90.0,
                      175.0, Config::GameConfig::RIGHT_BORDER - 175.0),
            clampReal(predicted.y() + velocityDir.y() * 90.0,
                      ArenaTop + 100.0, ArenaBottom - 90.0));
        const qreal sideSign = player.worldPos().y() < y ? 1.0 : -1.0;
        const QPointF fields[2] = {
            center,
            QPointF(clampReal(center.x() + side.x() * sideSign * 230.0,
                              175.0, Config::GameConfig::RIGHT_BORDER - 175.0),
                    clampReal(center.y() + side.y() * sideSign * 230.0,
                              ArenaTop + 100.0, ArenaBottom - 90.0))
        };
        for (const QPointF& field : fields) {
            addHazard({ BossHazardType::SeaweedZone, field, QRectF(),
                        158, 6500, 0, 8, true });
        }
        const qreal playerFrameSpeed = std::hypot(estimatedPlayerVelocity.x(), estimatedPlayerVelocity.y());
        seaweedFieldTimerMs = playerFrameSpeed > 2.8 ? 6000 : 6800;
    }
    seaweedTickMs = std::max(0, seaweedTickMs - FrameMs);
    for (const BossHazard& hazard : hazards) {
        if (!hazard.active || hazard.type != BossHazardType::SeaweedZone)
            continue;
        if (!circleHitsPlayer(hazard.position, hazard.radius, player))
            continue;
        player.applySpeedReduction(0.32);
        if (seaweedTickMs <= 0) {
            player.takeDurabilityDamage(hazard.damage);
            seaweedTickMs = 1000;
        }
    }

    reefContactCooldownMs = std::max(0, reefContactCooldownMs - FrameMs);
    for (const BossHazard& hazard : hazards) {
        if (!hazard.active || hazard.type != BossHazardType::ReefHitbox) continue;
        if (!hazard.rect.intersects(player.collider())) continue;

        QPointF away = player.worldPos() - hazard.position;
        qreal length = std::sqrt(away.x() * away.x() + away.y() * away.y());
        if (length <= 0.001) {
            away = QPointF(1.0, 0.0);
            length = 1.0;
        }
        player.setWorldPos(player.worldPos() +
            QPointF(away.x() / length * 4.0, away.y() / length * 4.0));
        if (reefContactCooldownMs <= 0 && player.canTakeDamage()) {
            player.takeDurabilityDamage(35);
            reefContactCooldownMs = 900;
        }
    }
}

void SirenBoss::updateSoulSong(Player& player)
{
    if (soulSongCastMs > 0) {
        soulSongCastMs = std::max(0, soulSongCastMs - FrameMs);
        if (soulSongCastMs <= 0) {
            const qreal maxHalfWidth = state == PHASE2 ? 38.0 : 34.0;
            bool playerHit = false;
            bool phantomHit = false;
            for (int i = 0; i < soulSongBeamCount; ++i) {
                const QPointF from = soulSongStarts[i];
                const QPointF to = soulSongTargets[i];
                addHazard({ BossHazardType::SoulSong, from,
                            beamBounds(from, to, maxHalfWidth),
                            maxHalfWidth, 760, 0,
                            state == PHASE2 ? 34 : 30,
                            true, i, to });

                qreal pillarT = 2.0;
                const bool pillarAbsorbed =
                    state == PHASE2 &&
                    chargeResonancePillarFromLine(from, to, maxHalfWidth, &pillarT);
                if (!playerHit &&
                    segmentHitsPlayer(from, to, maxHalfWidth, player)) {
                    const qreal playerT =
                        projectionOnSegment(player.worldPos(), from, to);
                    if (!pillarAbsorbed || pillarT > playerT + 0.04)
                        playerHit = true;
                }
                if (phantomSpawned &&
                    distancePointToSegment(phantomPos, from, to) <= maxHalfWidth)
                    phantomHit = true;
            }

            if (playerHit) {
                player.takeDurabilityDamage(state == PHASE2 ? 34 : 30);
                player.applyInputReverse(15000);
            }
            if (phantomHit)
                phantomStunMs += 5000;
            setVisualAction(BossVisualAction::SoulSong, 860);
            soulSongBeamCount = 0;
        }
        return;
    }

    soulSongTimerMs -= FrameMs;
    if (soulSongTimerMs <= 0) {
        soulSongCastDurationMs = state == PHASE2 ? 1900 : 2200;
        soulSongCastMs = soulSongCastDurationMs;
        soulSongTimerMs = state == PHASE2 ? 13500 : 16500;
        soulSongBeamCount = state == PHASE2 ? 6 : 5;

        const QPointF playerPos = player.worldPos();
        const qreal playerFrameSpeed = std::hypot(estimatedPlayerVelocity.x(), estimatedPlayerVelocity.y());
        const QPointF pursuit = normalizedOr(
            estimatedPlayerVelocity * 0.82 +
                normalizedOr(playerPos - position(), QPointF(facingX, 0.0)) * 0.32,
            normalizedOr(playerPos - position(), QPointF(facingX, 0.0)));
        const QPointF side(-pursuit.y(), pursuit.x());
        QPointF predicted = playerPos + estimatedPlayerVelocity *
            (playerFrameSpeed > 2.5 ? (state == PHASE2 ? 36.0 : 28.0)
                                    : (state == PHASE2 ? 28.0 : 22.0));
        predicted.setX(clampReal(predicted.x(), 110.0,
                                 Config::GameConfig::RIGHT_BORDER - 110.0));
        predicted.setY(clampReal(predicted.y(), ArenaTop + 70.0,
                                 ArenaBottom - 70.0));

        const qreal maxHalfWidth = state == PHASE2 ? 38.0 : 34.0;
        const qreal sideSpacing = state == PHASE2 ? 340.0 : 365.0;
        const qreal pursuitSpacing = state == PHASE2 ? 405.0 : 430.0;
        for (int i = 0; i < soulSongBeamCount; ++i) {
            const QPointF centers[6] = {
                predicted,
                predicted + side * sideSpacing,
                predicted - side * sideSpacing,
                predicted + pursuit * (pursuitSpacing * 0.82),
                predicted - pursuit * (pursuitSpacing * 0.95),
                predicted + side * (sideSpacing * 0.68) -
                    pursuit * (pursuitSpacing * 0.52)
            };
            const QPointF directions[6] = {
                side,
                normalizedOr(pursuit * 0.74 + side * 0.67, side),
                normalizedOr(pursuit * 0.74 - side * 0.67, side),
                pursuit,
                normalizedOr(side * 0.84 - pursuit * 0.54, side),
                normalizedOr(side * 0.84 + pursuit * 0.54, side)
            };
            QPointF center(
                clampReal(centers[i].x(), 85.0,
                          Config::GameConfig::RIGHT_BORDER - 85.0),
                clampReal(centers[i].y(), ArenaTop + 45.0,
                          ArenaBottom - 45.0));
            const QPointF direction = normalizedOr(directions[i], QPointF(1.0, 0.0));
            QPointF from = center - direction * 760.0;
            QPointF to = center + direction * 760.0;
            from.setX(clampReal(from.x(), 35.0,
                                Config::GameConfig::RIGHT_BORDER - 35.0));
            from.setY(clampReal(from.y(), ArenaTop + 20.0,
                                ArenaBottom - 20.0));
            to.setX(clampReal(to.x(), 35.0,
                              Config::GameConfig::RIGHT_BORDER - 35.0));
            to.setY(clampReal(to.y(), ArenaTop + 20.0,
                              ArenaBottom - 20.0));
            soulSongStarts[i] = from;
            soulSongTargets[i] = to;
            addHazard({ BossHazardType::SoulSong, from,
                        beamBounds(from, to, maxHalfWidth),
                        maxHalfWidth,
                        static_cast<qreal>(soulSongCastDurationMs + 120),
                        0, 0, true, i, to });
        }
        setVisualAction(BossVisualAction::SoulSongWindup, soulSongCastMs);
    }
}

void SirenBoss::updatePhantom(Player& player)
{
    if (state != PHASE1)
        return;
    if (!phantomSpawned) {
        phantomSpawned = true;
        phantomPos = QPointF(x - facingX * 220.0, y);
    }
    if (phantomStunMs > 0) {
        phantomStunMs -= FrameMs;
        return;
    }
    const qreal chaseSpeed = QLineF(phantomPos, player.worldPos()).length() > 260.0 ? 2.55 : 1.95;
    phantomPos = stepToward(phantomPos, player.worldPos(), chaseSpeed);
    phantomContactTickMs = std::max(0, phantomContactTickMs - FrameMs);
    if (circleHitsPlayer(phantomPos, 36, player) && phantomContactTickMs <= 0) {
        player.takeDurabilityDamage(8);
        phantomContactTickMs = 1000;
    }
}

void SirenBoss::updateElegy(Player& player)
{
    constexpr int ElegyTotalMs = 5600;
    constexpr int ElegyWindupMs = 1400;
    constexpr int ElegyActiveMs = ElegyTotalMs - ElegyWindupMs;
    constexpr int ElegyHoldRequirementMs = 3000;
    constexpr qreal ElegyRadius = 365.0;

    if (elegyCastMs > 0) {
        const int previousMs = elegyCastMs;
        elegyCastMs -= FrameMs;
        elegyCenter = position();
        const bool wasWindup = previousMs > ElegyActiveMs;
        const bool isActive = elegyCastMs <= ElegyActiveMs;

        if (wasWindup && isActive)
            setVisualAction(BossVisualAction::Elegy, ElegyActiveMs);

        if (isActive) {
            const int activeElapsed = qBound(0, ElegyActiveMs - std::max(0, elegyCastMs), ElegyActiveMs);
            const int visualStage = qBound(1, 1 + activeElapsed / 900, 3);
            elegyPulseMs = std::max(0, elegyPulseMs - FrameMs);
            if (elegyPulseMs <= 0) {
                addHazard({ BossHazardType::ElegyWarning, elegyCenter, QRectF(),
                            ElegyRadius, 900, 0, 1, true, visualStage });
                elegyPulseMs = 120;
            }

            const qreal distance = QLineF(elegyCenter, player.worldPos()).length();
            const bool inRange = distance <= ElegyRadius;
            if (inRange) {
                const qreal closeness = qBound<qreal>(0.0, 1.0 - distance / ElegyRadius, 1.0);
                player.applySpeedReduction(0.34 + closeness * 0.16);
                const QPointF pulled = stepToward(player.worldPos(), elegyCenter,
                                                  0.42 + closeness * 0.55);
                player.setWorldPos(pulled);
                elegyExposureMs += FrameMs;
                elegyTickMs = std::max(0, elegyTickMs - FrameMs);
                if (elegyTickMs <= 0) {
                    player.takeDurabilityDamage(2);
                    elegyTickMs = 650;
                }
                if (!elegySucceeded && player.isSpaceHeld() && !player.isMoving()) {
                    elegyHoldMs += FrameMs;
                    player.applySpeedReduction(0.92);
                    if (elegyHoldMs >= ElegyHoldRequirementMs) {
                        elegySucceeded = true;
                        player.restoreStaminaToFull();
                        addHazard({ BossHazardType::ElegyWarning, elegyCenter, QRectF(),
                                    ElegyRadius, 900, 0, 0, true, 4 });
                    }
                }
                else if (!elegySucceeded) {
                    elegyHoldMs = std::max(0, elegyHoldMs - FrameMs * 2);
                }
            }
            else if (!elegySucceeded) {
                elegyHoldMs = std::max(0, elegyHoldMs - FrameMs * 2);
            }
            if (state == PHASE2)
                chargeResonancePillarsFromElegy(player, ElegyRadius);
        }

        if (elegyCastMs <= 0) {
            if (!elegySucceeded && elegyExposureMs >= 650) {
                player.applyPoison(10000);
            }
            addHazard({ BossHazardType::ElegyWarning, elegyCenter, QRectF(),
                        ElegyRadius, 1100, 0, 0, true, 4 });
        }
        return;
    }

    elegyTimerMs -= FrameMs;
    if (elegyTimerMs <= 0) {
        elegyCenter = position();
        elegyCastMs = ElegyTotalMs;
        elegyPulseMs = 0;
        elegyTickMs = 0;
        elegyExposureMs = 0;
        elegyHoldMs = 0;
        elegySucceeded = false;
        for (bool& triggered : elegyResonanceTriggered)
            triggered = false;
        elegyTimerMs = 30000;
        setVisualAction(BossVisualAction::ElegyWindup, ElegyWindupMs);
        addHazard({ BossHazardType::ElegyWarning, elegyCenter, QRectF(),
                    ElegyRadius, ElegyWindupMs, 0, 0, true, 0 });
    }
}

void SirenBoss::updateEndlessReturn(Player& player)
{
    endlessReturnTimerMs -= FrameMs;
    if (endlessReturnTimerMs > 0) return;

    int px = int(player.worldPos().x());
    int py = int(player.worldPos().y());
    for (int i = 0; i < 10; ++i) {
        const qreal angle = (i / 10.0) * 6.28318530718 + (std::rand() % 35) * 0.01;
        const qreal distance = 145.0 + (std::rand() % 230);
        int rx = px + qRound(std::cos(angle) * distance);
        int ry = py + qRound(std::sin(angle) * distance);
        rx = clampInt(rx, 95, Config::GameConfig::RIGHT_BORDER - 95);
        ry = clampInt(ry, ArenaTop + 55, ArenaBottom - 55);
        QRectF reefRect(rx - 40, ry - 32, 80, 64);
        addHazard({ BossHazardType::ReefHitbox, QPointF(rx, ry), reefRect, 0, 20000, 0, 50, true });
    }

    endlessReturnTimerMs = 20000;
}

bool SirenBoss::chargeResonancePillar(int index)
{
    constexpr int ResonanceShatterMs = 1280;
    if (index < 0 || index >= 3) return false;
    if (!resonancePillarsPlaced || resonancePillarDestroyed[index]) return false;
    if (resonancePillarBurstMs[index] > 0) return false;
    if (resonancePillarCharges[index] >= 3) return false;

    ++resonancePillarCharges[index];
    addHazard({ BossHazardType::ResonancePillar, resonancePillarPositions[index],
                QRectF(), 72, 520, 0, resonancePillarCharges[index],
                true, resonancePillarCharges[index] });

    if (resonancePillarCharges[index] < 3)
        return true;

    resonancePillarBurstMs[index] = ResonanceShatterMs;
    return true;
}

bool SirenBoss::chargeResonancePillarFromLine(const QPointF& from, const QPointF& to,
                                              qreal halfWidth, qreal* outPillarT)
{
    if (!resonancePillarsPlaced) return false;

    int bestIndex = -1;
    qreal bestT = 2.0;
    for (int i = 0; i < 3; ++i) {
        if (resonancePillarDestroyed[i] ||
            resonancePillarBurstMs[i] > 0) continue;
        const QPointF pillarPos = resonancePillarPositions[i];
        const qreal t = projectionOnSegment(pillarPos, from, to);
        if (t <= 0.03 || t >= 0.995) continue;
        const qreal hitDistance = distancePointToSegment(pillarPos, from, to);
        if (hitDistance > halfWidth + 48.0) continue;
        if (t < bestT) {
            bestT = t;
            bestIndex = i;
        }
    }

    if (bestIndex < 0)
        return false;

    if (outPillarT)
        *outPillarT = bestT;
    return chargeResonancePillar(bestIndex);
}

void SirenBoss::chargeResonancePillarsFromElegy(Player& player, qreal radius)
{
    if (!resonancePillarsPlaced) return;
    if (QLineF(elegyCenter, player.worldPos()).length() > radius)
        return;

    for (int i = 0; i < 3; ++i) {
        if (elegyResonanceTriggered[i] ||
            resonancePillarDestroyed[i] ||
            resonancePillarBurstMs[i] > 0)
            continue;
        const qreal pillarInWave = QLineF(elegyCenter, resonancePillarPositions[i]).length();
        if (pillarInWave > radius + 72.0)
            continue;
        const qreal playerGuideDistance =
            QLineF(player.worldPos(), resonancePillarPositions[i]).length();
        if (playerGuideDistance > 145.0)
            continue;

        elegyResonanceTriggered[i] = true;
        chargeResonancePillar(i);
    }
}

void SirenBoss::resolveResonancePillarCollision(Player& player)
{
    if (!resonancePillarsPlaced) return;

    for (int i = 0; i < 3; ++i) {
        const QPointF pillarPos = resonancePillarPositions[i];
        const qreal bodyRadius = resonancePillarDestroyed[i] ? 42.0 : 58.0;
        QPointF away = player.worldPos() - pillarPos;
        qreal distance = std::sqrt(away.x() * away.x() + away.y() * away.y());
        const qreal minDistance = bodyRadius + 28.0;
        if (distance >= minDistance)
            continue;
        if (distance <= 0.001) {
            away = QPointF(1.0, 0.0);
            distance = 1.0;
        }
        const qreal push = minDistance - distance;
        player.setWorldPos(player.worldPos() +
            QPointF(away.x() / distance * push, away.y() / distance * push));
    }
}

void SirenBoss::updateResonancePillars(Player& player)
{
    constexpr int ResonanceShatterMs = 1280;
    constexpr int ChargeHoldMs = 180;
    constexpr int ShatterFrameMs = 150;
    constexpr int FinalShatterFrame = 10;

    if (!resonancePillarsPlaced) {
        const int leftX = clampInt(x - 430, 95, Config::GameConfig::RIGHT_BORDER - 95);
        const int rightX = clampInt(x + 430, 95, Config::GameConfig::RIGHT_BORDER - 95);
        const int upperY = clampInt(y - 245, ArenaTop + 75, ArenaBottom - 75);
        const int lowerY = clampInt(y + 245, ArenaTop + 75, ArenaBottom - 75);
        const int farY = y < (ArenaTop + ArenaBottom) / 2
            ? clampInt(y + 305, ArenaTop + 75, ArenaBottom - 75)
            : clampInt(y - 305, ArenaTop + 75, ArenaBottom - 75);
        resonancePillarPositions[0] = QPointF(leftX, upperY);
        resonancePillarPositions[1] = QPointF(rightX, lowerY);
        resonancePillarPositions[2] = QPointF(x, farY);
        for (int i = 0; i < 3; ++i) {
            resonancePillarCharges[i] = 0;
            resonancePillarBurstMs[i] = 0;
            resonancePillarDestroyed[i] = false;
            elegyResonanceTriggered[i] = false;
        }
        resonancePillarsPlaced = true;
        resonanceVisualRefreshMs = 0;
    }

    resonanceVisualRefreshMs = std::max(0, resonanceVisualRefreshMs - FrameMs);
    for (int i = 0; i < 3; ++i) {
        if (resonancePillarBurstMs[i] <= 0 ||
            resonancePillarDestroyed[i])
            continue;

        resonancePillarBurstMs[i] =
            std::max(0, resonancePillarBurstMs[i] - FrameMs);
        if (resonancePillarBurstMs[i] > 0)
            continue;

        const int resonanceTrueDamage = std::max(1, int(maxHp * 0.12f));
        resonancePillarDestroyed[i] = true;
        hp = std::max(0, hp - resonanceTrueDamage);
        addHazard({ BossHazardType::ResonanceBacklash, position(), QRectF(),
                    205, 1050, 0, resonanceTrueDamage, true });
        setVisualAction(BossVisualAction::Hit, 360);
        if (hp <= 0)
            startDeathAnimation();
    }

    for (int i = 0; i < 3; ++i) {
        const QPointF pillarPos = resonancePillarPositions[i];
        if (resonanceVisualRefreshMs <= 0) {
            int visualStage = resonancePillarCharges[i];
            if (resonancePillarDestroyed[i]) {
                visualStage = FinalShatterFrame;
            }
            else if (resonancePillarBurstMs[i] > 0) {
                const int elapsed =
                    ResonanceShatterMs - resonancePillarBurstMs[i];
                if (elapsed < ChargeHoldMs) {
                    visualStage = 3;
                }
                else {
                    visualStage = qBound(
                        4,
                        4 + (elapsed - ChargeHoldMs) / ShatterFrameMs,
                        FinalShatterFrame);
                }
            }
            addHazard({ BossHazardType::ResonancePillar, pillarPos, QRectF(), 48,
                        80, 0,
                        resonancePillarDestroyed[i] ? -1 : resonancePillarCharges[i],
                        true, visualStage });
        }

        if (!resonancePillarDestroyed[i] &&
            resonancePillarBurstMs[i] <= 0 &&
            resonancePillarCharges[i] >= 2 &&
            QLineF(pillarPos, player.worldPos()).length() <= 126.0) {
            player.applySpeedReduction(0.18);
        }
    }

    if (resonanceVisualRefreshMs <= 0)
        resonanceVisualRefreshMs = 80;
}

void SirenBoss::applyNaturalDecay()
{
    naturalDecayTimerMs += FrameMs;
    if (naturalDecayTimerMs < 1000) return;
    naturalDecayTimerMs -= 1000;
    hp -= std::max(1, int(maxHp * 0.0075f));

    // 二阶段仍会被歌声反噬缓慢衰弱；共鸣柱爆裂才是主要输出。
    hp -= std::max(1, int(maxHp * 0.005f));
    if (hp <= 0) {
        hp = 0;
        startDeathAnimation();
    }
}

void SirenBoss::restorePhaseCheckpoint(Player& player)
{
    if (state != PHASE2 || maxHp <= 0 || dying)
        return;

    const qreal ratio = static_cast<qreal>(hp) / maxHp;
    const qreal thresholds[3] = {0.75, 0.50, 0.25};
    for (int i = 0; i < 3; ++i) {
        if (staminaCheckpointUsed[i] || ratio > thresholds[i])
            continue;
        staminaCheckpointUsed[i] = true;
        player.clearMaxStaminaPenalty();
        player.restoreStaminaToFull();
    }
}
