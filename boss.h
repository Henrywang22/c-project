#pragma once
#include "Enemy.h"
#include <vector>
#include <QPointF>
#include <QRectF>

// Boss攻击危险区域类型
enum class BossHazardType {
    BombWarning, BombHitbox, MeleeHitbox, MouthStrike,
    EyeSector, CloneExplosionWarning, CloneExplosionHitbox,
    SoulSong, ElegyWarning, SeaweedZone, ReefWarning,
    ReefHitbox, ResonancePillar
};

// Boss召唤类型
enum class BossSpawnType { Shark, TaliClone, SirenPhantom };

// Boss种类
enum class BossKind {
    FiveHeadShark,
    TaliMonster,
    Siren
};

// 危险区域结构
struct BossHazard {
    BossHazardType type;
    QPointF position;
    QRectF  rect;
    qreal   radius = 0.0;
    qreal   angleDegrees = 0.0;
    qreal   arcDegrees = 0.0;
    qreal   durationMs = 0.0;
    qreal   elapsedMs = 0.0;
    int     damage = 0;
    bool    active = true;
    bool    triggered = false;
};

// 召唤请求结构
struct BossSpawnRequest {
    BossSpawnType type;
    QPointF position;
    QPointF dir;
    int hp = 0;
    int dmg = 0;
};

// ============================================================
// Boss 基类 — 继承 Enemy，兼容 GameManager 的调用接口
// ============================================================
class Boss : public Enemy {
public:
    enum State { PHASE1, PHASE2 };

    Boss(BossKind kind, int x, int y, int maxHp, int attack, int dropValue);
    virtual ~Boss() = default;

    // Enemy 接口（GameManager 调用）
    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;

    // Boss 专有接口
    virtual void takeDamage(int damage);
    virtual void applyShockStun(int durationMs);
    virtual void forceReleasePlayer();
    virtual bool isInvulnerable() const { return invulnerable; }

    virtual void spawnMinions(std::vector<Shark*>& sharks) {}
    std::vector<BossSpawnRequest> takeSpawnRequests();
    const std::vector<BossHazard>& hazards() const { return m_hazards; }

    // GameManager 访问的公开成员
    BossKind kind;
    State state = PHASE1;
    bool  minionSpawned = false;
    bool  invulnerable = false;
    bool  enraged = false;
    bool  holdingPlayer = false;
    int   stunRemainingMs = 0;

protected:
    virtual void updateBoss(Player& player) = 0;
    void setPhase(State newState);

    bool isStunned() const { return stunRemainingMs > 0; }
    int  scaledDamage(int base) const;
    void addHazard(const BossHazard& h);
    void updateHazards(int frameMs);
    void updateCommonTimers(int frameMs);
    void requestSpawn(BossSpawnType type, const QPointF& pos,
        const QPointF& dir = QPointF(), int hp = 0, int dmg = 0);

    QPointF position() const { return QPointF(x, y); }

    std::vector<BossHazard>       m_hazards;
    std::vector<BossSpawnRequest> m_spawnRequests;
};

// ============================================================
// FiveHeadSharkBoss — 五头鲨鱼Boss（第1关）
// ============================================================
class FiveHeadSharkBoss : public Boss {
public:
    FiveHeadSharkBoss(int x, int y);
    bool collidesWithPlayer(int px, int py) override;
    void spawnMinions(std::vector<Shark*>& sharks) override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePatrol(int frameMs);
    void updateMelee(Player& player, int frameMs);
    void updateSummon(int frameMs);
    void updateBombardment(Player& player, int frameMs);

    int   patrolDir = 1;
    int   summonTimerMs = 5000;
    int   bombardmentTimerMs = 15000;
    int   bombardmentCastMs = 0;
    int   meleeCooldownMs = 0;
    int   meleeWindupMs = 0;
    int   meleeRecoveryMs = 0;
    std::vector<QRectF> pendingBombRects;
};

// ============================================================
// TaliMonsterBoss — 符咒怪Boss（第3关）
// ============================================================
class TaliMonsterBoss : public Boss {
public:
    TaliMonsterBoss(int x, int y);
    void takeDamage(int damage) override;
    void forceReleasePlayer() override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePhase1(Player& player, int frameMs);
    void updatePhase2(Player& player, int frameMs);
    void updateMovement(Player& player);
    void updateMouthStrike(Player& player, int frameMs);
    void updateEyeSweep(Player& player, int frameMs);
    void spawnClone();
    void updateClone(Player& player, int frameMs);
    void startCloneExplosion();
    void finishCloneExplosion(Player& player);

    bool    cloneSpawned = false;
    bool    cloneAlive = false;
    bool    phase2InvulnerabilityEnded = false;
    int     mouthTimerMs = 15000;
    int     mouthSequenceTimerMs = 0;
    int     mouthStrikeIndex = 0;
    int     eyeSweepTimerMs = 30000;
    int     eyeSweepRemainingMs = 0;
    float   eyeAngleDegrees = 0.0f;
    int     cloneHp = 0;
    QPointF clonePos;
    int     cloneExplosionTimerMs = 0;
};

// ============================================================
// SirenBoss — 海妖Boss（第5关）
// ============================================================
class SirenBoss : public Boss {
public:
    SirenBoss(int x, int y);
    void takeDamage(int damage) override;

protected:
    void updateBoss(Player& player) override;

private:
    void updatePhase1(Player& player, int frameMs);
    void updatePhase2(Player& player, int frameMs);
    void updateSoulSong(Player& player, int frameMs);
    void updatePhantom(Player& player, int frameMs);
    void updateElegy(Player& player, int frameMs);
    void updateEndlessReturn(Player& player, int frameMs);
    void updateResonancePillars();
    void applyNaturalDecay();
    void checkPhase2StaminaCheckpoints(Player& player);

    bool    phantomSpawned = false;
    bool    checkpoint75Used = false;
    bool    checkpoint50Used = false;
    bool    checkpoint25Used = false;
    int     soulSongTimerMs = 20000;
    int     soulSongCastMs = 0;
    int     elegyTimerMs = 30000;
    int     elegyCastMs = 0;
    int     endlessReturnTimerMs = 20000;
    int     poisonRemainingMs = 0;
    int     naturalDecayAccumulatorMs = 0;
    int     phantomStunMs = 0;
    QPointF phantomPos;
};