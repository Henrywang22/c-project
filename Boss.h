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
    MouthStrike,
    EyeSector,
    CloneExplosionWarning,
    SoulSong,
    ElegyWarning,
    SeaweedZone,
    ReefHitbox
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
    virtual bool canBeHitAt(int targetX, int targetY) const;
    virtual void takeDamage(int damage);
    virtual void applyShockStun(int durationMs);
    virtual void forceReleasePlayer();
    virtual bool isInvulnerable() const { return invulnerable; }
    virtual bool getSecondaryTarget(QPointF& outPos, int& outHp, int& outMaxHp) const;

    void spawnMinions(std::vector<Shark*>& sharks);
    const std::vector<BossHazard>& getHazards() const { return hazards; }

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
    QPointF position() const { return QPointF(x, y); }
    bool stunned() const { return stunRemainingMs > 0; }

    bool invulnerable = false;
    bool enraged = false;
    bool holdingPlayer = false;
    int stunRemainingMs = 0;
    std::vector<BossHazard> hazards;
    std::vector<BossSpawnRequest> sharkSpawnRequests;
};

class FiveHeadSharkBoss : public Boss {
public:
    FiveHeadSharkBoss(int x, int y);
    bool collidesWithPlayer(int px, int py) override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePatrol();
    void updateMelee(Player& player);
    void updateSummon();
    void updateBombardment(Player& player);

    int patrolDir = 1;
    int summonTimerMs = 5000;
    int bombardmentTimerMs = 15000;
    int bombardmentCastMs = 0;
    int meleeCooldownMs = 0;
    int meleeWindupMs = 0;
    int meleeRecoveryMs = 0;
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
    bool canBeHitAt(int targetX, int targetY) const override;
    void takeDamage(int damage) override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePhase1(Player& player);
    void updatePhase2(Player& player);
    void updateSoulSong(Player& player);
    void updatePhantom(Player& player);
    void updateElegy(Player& player);
    void updateEndlessReturn(Player& player);
    void applyNaturalDecay();
    void checkStaminaCheckpoints(Player& player);

    bool phantomSpawned = false;
    QPointF phantomPos;
    int phantomStunMs = 0;
    bool checkpoint75Used = false;
    bool checkpoint50Used = false;
    bool checkpoint25Used = false;
    int soulSongTimerMs = 20000;
    int soulSongCastMs = 0;
    int elegyTimerMs = 30000;
    int elegyCastMs = 0;
    int endlessReturnTimerMs = 20000;
    int naturalDecayTimerMs = 0;
    int poisonRemainingMs = 0;
};
