#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QElapsedTimer>
#include <QKeyEvent>
#include "GameConfig.h"

class Player : public QObject
{
    Q_OBJECT
public:
    static Player& instance();
    void update(qreal deltaTime);
    void keyPress(QKeyEvent* e);
    void keyRelease(QKeyEvent* e);
    QPointF worldPos() const { return m_worldPos; }
    QRectF collider() const { return { m_worldPos.x(), m_worldPos.y(), 50, 30 }; }
    qreal currentSpeed() const { return m_currentSpeed; }
    int durability() const { return m_durability; }
    int stamina() const { return m_stamina; }
    bool isStunned() const { return m_isStunned; }
    bool isDead() const { return m_isDead; }
    void takeDurabilityDamage(int damage);
    void applyStun(int durationMs);
    void applyRebound(const QPointF& direction);
    void applySpeedReduction(qreal reduction);
    void resetSpeedReduction();
    void saveState();
    void loadState();
    //1111
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