#pragma once

#include "Enemy.h"
#include <QPointF>
#include <QRectF>
#include <vector>

enum class BossKind {
    FiveHeadShark,
    TaliMonster,
    Siren
};

enum class BossHazardType {
    BombWarning,
    BombHitbox,
    MeleeHitbox,
    SummonMarker,
    MouthStrike,
    EyeSector,
    CloneExplosionWarning,
    SoulSong,
    ElegyWarning,
    SeaweedZone,
    ReefHitbox,
    ResonancePillar,
    ResonanceBacklash
};

enum class BossVisualAction {
    Idle,
    Bite,
    Cast,
    Summon,
    Hit,
    PhaseTransition,
    SoulSongWindup,
    SoulSong,
    ElegyWindup,
    Elegy,
    Death
};

struct BossHazard {
    BossHazardType type;
    QPointF position;
    QRectF rect;
    qreal radius = 0.0;
    qreal durationMs = 0.0;
    qreal elapsedMs = 0.0;
    int damage = 0;
    bool active = true;
    int visualStage = 0;
    QPointF target = QPointF();
};

struct BossSpawnRequest {
    QPointF position;
};

class Boss : public Enemy {
public:
    enum State { PHASE1, PHASE2 };

    Boss(BossKind kind, int x, int y, int maxHp, int attack, int dropValue);
    ~Boss() override = default;

    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;
    virtual bool canBeHitAt(int targetX, int targetY) const;
    virtual void takeDamage(int damage);
    virtual void applyShockStun(int durationMs);
    virtual void forceReleasePlayer();
    virtual bool isInvulnerable() const { return invulnerable; }
    virtual bool getSecondaryTarget(QPointF& outPos, int& outHp, int& outMaxHp) const;
    virtual bool getCompanionVisual(QPointF& outPos, bool& outStunned) const;

    void spawnMinions(std::vector<Shark*>& sharks);
    const std::vector<BossHazard>& getHazards() const { return hazards; }
    BossVisualAction visualAction() const { return visualActionValue; }
    qreal visualActionProgress() const;
    bool isDying() const { return dying; }
    bool isStunnedByShock() const { return stunRemainingMs > 0; }

    BossKind kind;
    State state = PHASE1;
    bool minionSpawned = false;

protected:
    virtual void updateBoss(Player& player) = 0;
    void updateTimers();
    void updateHazards();
    void addHazard(const BossHazard& hazard);
    void requestSharkSpawn(const QPointF& position);
    int scaledDamage(int baseDamage) const;
    void setVisualAction(BossVisualAction action, int durationMs);
    void startDeathAnimation();
    QPointF position() const { return QPointF(x, y); }
    bool stunned() const { return stunRemainingMs > 0; }

    bool invulnerable = false;
    bool enraged = false;
    bool holdingPlayer = false;
    int stunRemainingMs = 0;
    bool dying = false;
    int deathRemainingMs = 0;
    BossVisualAction visualActionValue = BossVisualAction::Idle;
    int visualActionDurationMs = 0;
    int visualActionRemainingMs = 0;
    std::vector<BossHazard> hazards;
    std::vector<BossSpawnRequest> sharkSpawnRequests;
};

class FiveHeadSharkBoss : public Boss {
public:
    FiveHeadSharkBoss(int x, int y);
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePatrol(Player& player);
    void updateMelee(Player& player);
    void updateSummon(Player& player);
    void updateBombardment(Player& player);

    int patrolDir = 1;
    int summonTimerMs = 4200;
    int bombardmentTimerMs = 9000;
    int bombardmentCastMs = 0;
    int meleeCooldownMs = 0;
    int meleeWindupMs = 0;
    int meleeRecoveryMs = 0;
    int meleePressureMs = 0;
    int contactCooldownMs = 0;
    QPointF lastPlayerPos;
    QPointF estimatedPlayerVelocity;
    bool hasLastPlayerPos = false;
    bool phase1SummonUsed = false;
    bool phase2SummonUsed = false;
    bool phase2SummonPrimed = false;
    std::vector<QRectF> pendingBombRects;
};

class TaliMonsterBoss : public Boss {
public:
    TaliMonsterBoss(int x, int y);
    bool canBeHitAt(int targetX, int targetY) const override;
    void takeDamage(int damage) override;
    void forceReleasePlayer() override;
    bool getSecondaryTarget(QPointF& outPos, int& outHp, int& outMaxHp) const override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePhase1(Player& player);
    void updatePhase2(Player& player);
    void updateMovement(Player& player);
    void updateMouthStrike(Player& player);
    void updateEyeSweep(Player& player);
    void spawnClone();
    void updateClone(Player& player);
    void startCloneExplosion();
    void finishCloneExplosion(Player& player);

    bool cloneSpawned = false;
    bool cloneAlive = false;
    bool phase2InvulnerabilityEnded = false;
    QPointF clonePos;
    int cloneHp = 0;
    int cloneExplosionTimerMs = 0;
    int mouthTimerMs = 15000;
    int mouthSequenceTimerMs = 0;
    int mouthStrikeIndex = 0;
    int eyeSweepTimerMs = 60000;
    int eyeSweepRemainingMs = 0;
};

class SirenBoss : public Boss {
public:
    SirenBoss(int x, int y);
    QRectF collider() const override;
    bool canBeHitAt(int targetX, int targetY) const override;
    void takeDamage(int damage) override;
    void applyShockStun(int durationMs) override;
    bool getCompanionVisual(QPointF& outPos, bool& outStunned) const override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePhase1(Player& player);
    void updatePhase2(Player& player);
    void updateSoulSong(Player& player);
    void updatePhantom(Player& player);
    void updateElegy(Player& player);
    void updateEndlessReturn(Player& player);
    void updateResonancePillars(Player& player);
    void updateMovement(Player& player);
    QPointF tridentTipWorld() const;
    bool chargeResonancePillar(int index);
    bool chargeResonancePillarFromLine(const QPointF& from, const QPointF& to,
                                       qreal halfWidth, qreal* outPillarT = nullptr);
    void chargeResonancePillarsFromElegy(Player& player, qreal radius);
    void resolveResonancePillarCollision(Player& player);
    void applyNaturalDecay();
    void restorePhaseCheckpoint(Player& player);

    bool phantomSpawned = false;
    QPointF phantomPos;
    QPointF lastPlayerPos;
    QPointF estimatedPlayerVelocity;
    bool hasLastPlayerPos = false;
    int phantomStunMs = 0;
    int phaseTransitionMs = 0;
    int soulSongTimerMs = 6500;
    int soulSongCastMs = 0;
    int soulSongCastDurationMs = 0;
    int soulSongBeamCount = 0;
    QPointF soulSongStarts[6];
    QPointF soulSongTargets[6];
    int elegyTimerMs = 10500;
    int elegyCastMs = 0;
    int elegyPulseMs = 0;
    int elegyTickMs = 0;
    int elegyExposureMs = 0;
    int elegyHoldMs = 0;
    QPointF elegyCenter;
    int endlessReturnTimerMs = 20000;
    int naturalDecayTimerMs = 0;
    int phaseMotionTimerMs = 0;
    int phantomContactTickMs = 0;
    int seaweedTickMs = 0;
    int seaweedFieldTimerMs = 4600;
    int reefContactCooldownMs = 0;
    int resonanceVisualRefreshMs = 0;
    bool resonancePillarsPlaced = false;
    QPointF resonancePillarPositions[3];
    int resonancePillarCharges[3] = {0, 0, 0};
    int resonancePillarBurstMs[3] = {0, 0, 0};
    bool resonancePillarDestroyed[3] = {false, false, false};
    bool elegyResonanceTriggered[3] = {false, false, false};
    bool elegySucceeded = false;
    bool staminaCheckpointUsed[3] = {false, false, false};
};
