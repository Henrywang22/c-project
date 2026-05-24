#ifndef BOSS_H
#define BOSS_H

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QtGlobal>

class Player;

enum class BossType {
    FiveHeadShark,
    TaliMonster,
    Siren
};

enum class BossPhase {
    Phase1,
    Phase2,
    Dead
};

enum class BossHazardType {
    BombWarning,
    BombHitbox,
    MeleeHitbox,
    MouthStrike,
    EyeSector,
    CloneExplosionWarning,
    CloneExplosionHitbox,
    SoulSong,
    ElegyWarning,
    SeaweedZone,
    ReefWarning,
    ReefHitbox,
    ResonancePillar
};

enum class BossSpawnType {
    Shark,
    TaliClone,
    SirenPhantom
};

struct BossHazard {
    BossHazardType type;
    QPointF position;
    QRectF rect;
    qreal radius = 0.0;
    qreal angleDegrees = 0.0;
    qreal arcDegrees = 0.0;
    qreal durationMs = 0.0;
    qreal elapsedMs = 0.0;
    int damage = 0;
    bool active = true;
};

struct BossSpawnRequest {
    BossSpawnType type;
    QPointF position;
    QPointF direction;
    int hp = 0;
    int damage = 0;
};

class Boss
{
public:
    explicit Boss(BossType type, const QPointF& spawnPos);
    virtual ~Boss() = default;

    virtual void update(qreal deltaTimeMs, Player* player) = 0;
    virtual QRectF collider() const = 0;

    virtual QPointF worldPos() const;
    virtual BossType type() const;
    virtual BossPhase phase() const;

    virtual int hp() const;
    virtual int maxHp() const;
    virtual bool isAlive() const;
    virtual bool isInvulnerable() const;
    virtual bool isStunned() const;
    virtual bool isHoldingPlayer() const;

    virtual void takeDamage(int damage);
    virtual void applyShockStun(qreal durationMs);
    virtual void forceReleasePlayer();
    virtual void onPlayerCollision(Player* player);

    const QList<BossHazard>& hazards() const;
    QList<BossSpawnRequest> takeSpawnRequests();

protected:
    void setPhase(BossPhase phase);
    void die();

    int scaledDamage(int baseDamage) const;
    void addHazard(const BossHazard& hazard);
    void updateHazards(qreal deltaTimeMs);
    void requestSpawn(BossSpawnType type, const QPointF& position,
                      const QPointF& direction = QPointF(), int hp = 0, int damage = 0);

protected:
    BossType m_type;
    BossPhase m_phase = BossPhase::Phase1;
    QPointF m_worldPos;

    int m_hp = 1;
    int m_maxHp = 1;
    bool m_alive = true;
    bool m_invulnerable = false;
    bool m_enraged = false;
    bool m_holdingPlayer = false;

    qreal m_stunRemainingMs = 0.0;
    QList<BossHazard> m_hazards;
    QList<BossSpawnRequest> m_spawnRequests;
};

class FiveHeadSharkBoss : public Boss
{
public:
    explicit FiveHeadSharkBoss(const QPointF& spawnPos);

    void update(qreal deltaTimeMs, Player* player) override;
    QRectF collider() const override;
    void onPlayerCollision(Player* player) override;

private:
    void updatePatrol(qreal deltaTimeMs);
    void updateMelee(qreal deltaTimeMs, Player* player);
    void updateSummon(qreal deltaTimeMs);
    void updateBombardment(qreal deltaTimeMs);

private:
    qreal m_patrolDirection = 1.0;
    qreal m_summonTimerMs = 0.0;
    qreal m_bombardmentTimerMs = 0.0;
    qreal m_meleeCooldownMs = 0.0;
    qreal m_meleeStateTimerMs = 0.0;
};

class TaliMonsterBoss : public Boss
{
public:
    explicit TaliMonsterBoss(const QPointF& spawnPos);

    void update(qreal deltaTimeMs, Player* player) override;
    QRectF collider() const override;
    void forceReleasePlayer() override;

private:
    void updatePhase1(qreal deltaTimeMs, Player* player);
    void updatePhase2(qreal deltaTimeMs, Player* player);
    void updateMovement(qreal deltaTimeMs, Player* player);
    void updateMouthStrike(qreal deltaTimeMs, Player* player);
    void updateEyeSweep(qreal deltaTimeMs, Player* player);
    void spawnClone();
    void onCloneDeath(bool explosionNearBoss);

private:
    bool m_cloneSpawned = false;
    bool m_cloneAlive = false;
    bool m_phase2InvulnerabilityEnded = false;
    qreal m_mouthTimerMs = 0.0;
    qreal m_eyeSweepTimerMs = 0.0;
    qreal m_eyeSweepRemainingMs = 0.0;
    qreal m_eyeAngleDegrees = 0.0;
    int m_mouthStrikeIndex = 0;
};

class SirenBoss : public Boss
{
public:
    explicit SirenBoss(const QPointF& spawnPos);

    void update(qreal deltaTimeMs, Player* player) override;
    QRectF collider() const override;
    void takeDamage(int damage) override;

private:
    void updatePhase1(qreal deltaTimeMs, Player* player);
    void updatePhase2(qreal deltaTimeMs, Player* player);
    void updateSoulSong(qreal deltaTimeMs, Player* player);
    void updateElegy(qreal deltaTimeMs, Player* player);
    void updateEndlessReturn(qreal deltaTimeMs, Player* player);
    void updateResonancePillars(qreal deltaTimeMs);
    void applyNaturalDecay(qreal deltaTimeMs);
    void checkPhase2StaminaCheckpoints(Player* player);

private:
    bool m_phantomSpawned = false;
    bool m_checkpoint75Used = false;
    bool m_checkpoint50Used = false;
    bool m_checkpoint25Used = false;
    qreal m_soulSongTimerMs = 0.0;
    qreal m_elegyTimerMs = 0.0;
    qreal m_endlessReturnTimerMs = 0.0;
};

#endif // BOSS_H