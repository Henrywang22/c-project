#pragma once
#include <QPointF>
#include <QRectF>
class Player;

// ============================================================
// Enemy — 所有敌人的基类
// ============================================================
class Enemy {
public:
    Enemy(int x, int y);
    virtual ~Enemy() {}
    virtual void update(Player& player) = 0;
    virtual bool collidesWithPlayer(int px, int py) = 0;
    virtual QRectF collider() const;
    virtual QPointF position() const;
    virtual void setPosition(const QPointF& pos);
    virtual void takeDamage(int damage);
    void applyKnockback(const QPointF& origin, qreal strength);
    void applyStageScaling(int stage);

    int x, y;
    int hp;
    int maxHp;
    int attack;
    float speed;
    float facingX = -1.0f;
    bool alive = true;
    int attackTimer = 0;
    int dropValue;

protected:
    bool advanceKnockback();

private:
    QPointF m_knockbackVelocity;
    int m_knockbackFrames = 0;
    int m_scaledStage = 0;
};

// ============================================================
// Shark — 普通鲨鱼
// 直接追踪玩家，靠近后持续造成伤害
// ============================================================
class Shark : public Enemy {
public:
    Shark(int x, int y);
    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;
    QRectF biteCollider() const;
    bool canBite() const;
    void startBiteCooldown(const QPointF& playerPos);
    QPointF position() const override;
    void setPosition(const QPointF& pos) override;

private:
    float posX, posY;
    int biteCooldown = 0;
    int retreatTimer = 0;
    float retreatVx = 0.0f;
    float retreatVy = 0.0f;
};

// ============================================================
// Swordfish — 剑鱼
// 平时巡逻，发现玩家后蓄力冲刺造成高伤害
// ============================================================
class Swordfish : public Enemy {
public:
    enum State { IDLE, WINDUP, CHARGE };

    Swordfish(int x, int y);
    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;
    QPointF position() const override;
    void setPosition(const QPointF& pos) override;
    void takeDamage(int damage) override;

    State state = IDLE;
    int chargeTimer = 0;
    int windupTimer = 0;  // 蓄力计时
    float chargeVx = 0;   // 冲刺方向X
    float chargeVy = 0;   // 冲刺方向Y
    float posX, posY;     // float位置保证冲刺精度

private:
    int patrolTimer = 0;
    float patrolVx = 1.0f;
    float patrolVy = 0.0f;
};

// ============================================================
// Octopus — 墨鱼
// 会隐身，接触玩家后遮挡视野
// ============================================================
class Octopus : public Enemy {
public:
    Octopus(int x, int y);
    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;
    QPointF position() const override;
    void setPosition(const QPointF& pos) override;

    bool isInvisible = false;
    bool isInkCharging() const { return inkWindupFrames > 0; }
    bool hasInkProjectile() const { return inkProjectileActive; }
    QPointF inkProjectilePosition() const { return inkProjectilePos; }
    QPointF inkProjectileDirection() const { return inkProjectileVelocity; }
    int inkAnimationFrame() const { return (inkProjectileAge / 5) % 4; }

private:
    int invisTimer = 0;
    int inkCooldownFrames = 120;
    int inkWindupFrames = 0;
    int inkProjectileLife = 0;
    int inkProjectileAge = 0;
    bool inkProjectileActive = false;
    QPointF inkProjectilePos;
    QPointF inkProjectileVelocity;
    float posX, posY;
};

class ElectricRay : public Enemy {
public:
    ElectricRay(int x, int y);
    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;
    QPointF position() const override;
    void setPosition(const QPointF& pos) override;

    bool isPulseCharging() const { return pulseWarningFrames > 0; }
    bool isPulseVisible() const { return pulseVisualFrames > 0; }
    int pulseAnimationFrame() const;
    qreal pulseRadius() const;

private:
    float posX, posY;
    int pulseCooldownFrames = 120;
    int pulseWarningFrames = 0;
    int pulseVisualFrames = 0;
};

class PoisonJellyfish : public Enemy {
public:
    enum State { DRIFT, WINDUP, STRIKE, RETREAT };

    PoisonJellyfish(int x, int y);
    void update(Player& player) override;
    bool collidesWithPlayer(int px, int py) override;
    QRectF collider() const override;
    QRectF stingCollider() const;
    QPointF position() const override;
    void setPosition(const QPointF& pos) override;
    bool isStingCharging() const { return state == WINDUP; }
    bool isStingActive() const { return state == STRIKE; }
    int stingAnimationFrame() const;

private:
    float posX, posY;
    float driftPhase = 0.0f;
    int poisonCooldownFrames = 0;
    int stateTimer = 0;
    bool stingHit = false;
    QPointF retreatVelocity;
    State state = DRIFT;
};
