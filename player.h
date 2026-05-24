#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QVector2D>
#include "GameConfig.h"

class Weapon;

class Player : public QObject
{
    Q_OBJECT
public:
    static Player& instance();
    void update(qreal deltaTime);
    void keyPress(QKeyEvent* e);
    void keyRelease(QKeyEvent* e);

    // 坐标和碰撞
    QPointF worldPos() const { return m_worldPos; }
    QRectF collider() const { return { m_worldPos.x(), m_worldPos.y(), 50, 30 }; }

    // 速度状态
    qreal currentSpeed() const { return m_currentSpeed; }

    // 耐久和体力（只读接口）
    int durability() const { return m_durability; }
    int stamina() const { return m_stamina; }

    // 状态查询
    bool isStunned() const { return m_isStunned; }
    bool isDead() const { return m_isDead; }

    // 战斗与生存接口
    bool canTakeDamage() const; // 新增：是否可受伤(Dash期间无敌)
    void takeDurabilityDamage(int damage);
    void applyStun(int durationMs);
    void applyRebound(const QPointF& direction);
    void applySpeedReduction(qreal reduction);
    void resetSpeedReduction();

    // ==========================================
    // Boss 联调核心机制接口 (Dash & Shock)
    // ==========================================
    bool canDash() const;
    bool isDashing() const;
    void triggerDash();

    bool canShock() const;
    void triggerShock();
    bool isShockActive() const;
    QRectF shockArea() const;

    // ==========================================
    // 体力消耗与衰减体系 (塔里怪物 & 塞壬)
    // ==========================================
    bool consumeStamina(int amount);
    void restoreStaminaToFull();
    void applyTemporaryMaxStaminaPenalty(int amount);
    void clearMaxStaminaPenalty();

    // ==========================================
    // Debuff 与特殊状态体系 (塞壬)
    // ==========================================
    void applyInputReverse(int durationMs);
    bool isInputReversed() const;

    void applyNoRangedAttack(int durationMs);
    bool canUseRangedAttack() const;

    void applyPoison(int durationMs);
    bool isPoisoned() const;

    bool isMoving() const;
    bool isSpaceHeld() const;

    // Item.cpp 原有接口
    void restoreStamina(int amount);
    void restoreDurability(int amount);
    void upgradeBaseSpeed(float amount);
    void upgradeMaxDurability(int amount);
    void upgradeMaxStamina(int amount);

    // (保留兼容D模块的旧接口，建议后续D模块清理)
    Weapon* getCurrentWeapon();
    void equipWeapon(Weapon* weapon);

    // 存档接口
    void saveState();
    void loadState();

    // 游戏进度数据
    int coins = 0;
    int fishCaught = 0;
    int fishTotalValue = 0;
    int distance = 0;
    int gameSeconds = 0;
    bool visionReduced = false;
    int maxDurability = 100;
    int maxStamina = 100;

    void reset();

signals:
    void stateChanged();
    void playerDied();

private:
    Player();
    void checkBorder();
    void updateMovement(qreal deltaTime);
    void updateDebuffs(); // 新增：处理所有的状态计时器

    QPointF m_worldPos;
    qreal m_currentSpeed;
    qreal m_baseSpeed;
    int m_durability;
    int m_stamina;

    // 限制与惩罚
    int m_staminaPenalty;

    // 基础状态
    bool m_isStunned;
    bool m_isDead;
    QElapsedTimer m_stunTimer;
    int m_stunDuration;
    qreal m_speedReduction;
    bool m_reboundActive;
    QPointF m_reboundDir;

    // 按键追踪
    bool m_keyW, m_keyA, m_keyS, m_keyD, m_keyShift, m_keySpace;

    // --- Dash 属性 ---
    bool m_isDashing;
    int m_dashCooldownMs;
    int m_dashDurationMs;
    QPointF m_dashDirection;
    QElapsedTimer m_dashTimer;
    QElapsedTimer m_dashCooldown;

    // --- Shock 属性 ---
    bool m_isShockActive;
    int m_shockCharges;
    int m_shockCooldownMs;
    int m_shockDurationMs;
    QElapsedTimer m_shockTimer;
    QElapsedTimer m_shockCooldown;

    // --- Debuff 属性 ---
    bool m_isInputReversed;
    int m_inputReverseDurationMs;
    QElapsedTimer m_inputReverseTimer;

    bool m_noRangedAttack;
    int m_noRangedDurationMs;
    QElapsedTimer m_noRangedTimer;

    bool m_isPoisoned;
    int m_poisonDurationMs;
    QElapsedTimer m_poisonTimer;
    QElapsedTimer m_poisonTickTimer;

    Weapon* m_currentWeapon = nullptr;
};

#endif // PLAYER_H