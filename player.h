#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QVector2D>
#include "GameConfig.h"

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

    // 战斗接口
    void takeDurabilityDamage(int damage);
    void applyStun(int durationMs);
    void applyRebound(const QPointF& direction);
    void applySpeedReduction(qreal reduction);
    void resetSpeedReduction();

    // 存档接口
    void saveState();
    void loadState();

    // ==========================================
    // 游戏进度数据（GameManager和GameWindow需要）
    // ==========================================
    int coins = 0;   // 金币
    int fishCaught = 0;   // 捕鱼数量
    int fishTotalValue = 0;   // 捕鱼总价值
    int distance = 0;   // 航行距离
    int gameSeconds = 0;   // 游戏时间（秒）
    bool visionReduced = false; // 墨鱼遮挡视野

    // 最大值（供HUD显示血条用）
    int maxDurability = 100;
    int maxStamina = 100;

signals:
    void stateChanged();
    void playerDied();

private:
    Player();
    void checkBorder();
    void updateMovement(qreal deltaTime);

    QPointF m_worldPos;
    qreal m_currentSpeed;
    qreal m_baseSpeed;
    int m_durability;
    int m_stamina;
    bool m_isStunned;
    bool m_isDead;
    QElapsedTimer m_stunTimer;
    int m_stunDuration;
    qreal m_speedReduction;
    bool m_reboundActive;
    QPointF m_reboundDir;
    bool m_keyW, m_keyA, m_keyS, m_keyD, m_keyShift;
};

#endif // PLAYER_H