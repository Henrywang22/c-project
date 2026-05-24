#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <QPointF>
#include <QRectF>
#include <QElapsedTimer>
#include "GameConfig.h"

enum class ObstacleType { REEF, WHIRLPOOL };

class Obstacle
{
public:
    explicit Obstacle(ObstacleType type, const QPointF& worldPos);
    virtual ~Obstacle() = default;

    // ========== 修复：把update()标记为const ==========
    virtual void update(qreal deltaTime) const;
    virtual QRectF collider() const;
    virtual void onPlayerCollision(class Player* player);
    ObstacleType type() const { return m_type; }
    QPointF worldPos() const { return m_worldPos; }
    int size() const { return m_size; }
    bool isVisible(const QPointF& playerPos) const;

protected:
    ObstacleType m_type;
    QPointF m_worldPos;
    int m_size;
};

class Reef : public Obstacle
{
public:
    explicit Reef(const QPointF& worldPos);
    void onPlayerCollision(Player* player) override;
};

class Whirlpool : public Obstacle
{
public:
    explicit Whirlpool(const QPointF& worldPos);
    // ========== 修复：把update()标记为const ==========
    void update(qreal deltaTime) const override;
    void onPlayerCollision(Player* player) override;
    qreal currentSpeedReduction() const { return m_speedReduction; }

private:
    // ========== 修复：把成员变量标记为mutable，允许在const函数里修改 ==========
    mutable qreal m_speedReduction;
    mutable qreal m_timeInWhirlpool;
};

class ObstacleManager
{
public:
    static ObstacleManager& instance();
    void generateLevel(int level);
    void update(qreal deltaTime);
    const QList<Obstacle*>& obstacles() const { return m_obstacles; }
    void clear();

private:
    ObstacleManager() = default;
    QList<Obstacle*> m_obstacles;
};

#endif // OBSTACLE_H
