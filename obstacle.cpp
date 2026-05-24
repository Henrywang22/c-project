#include "GameConfig.h"
#include "Obstacle.h"
#include "Player.h"
#include <QRandomGenerator>
#include <QVector2D>

using namespace Config;

Obstacle::Obstacle(ObstacleType type, const QPointF& worldPos)
    : m_type(type), m_worldPos(worldPos), m_size(30) {}

void Obstacle::update(qreal deltaTime) const { Q_UNUSED(deltaTime); }

QRectF Obstacle::collider() const {
    return {m_worldPos.x() - m_size/2.0f, m_worldPos.y() - m_size/2.0f,
            static_cast<qreal>(m_size), static_cast<qreal>(m_size)};
}

void Obstacle::onPlayerCollision(Player* player) { Q_UNUSED(player); }

bool Obstacle::isVisible(const QPointF& playerPos) const {
    return QLineF(m_worldPos, playerPos).length() < GameConfig::VISION_RANGE;
}

Reef::Reef(const QPointF& worldPos) : Obstacle(ObstacleType::REEF, worldPos) {
    m_size = GameConfig::REEF_MIN_SIZE +
             QRandomGenerator::global()->generate() % (GameConfig::REEF_MAX_SIZE - GameConfig::REEF_MIN_SIZE + 1);
}

void Reef::onPlayerCollision(Player* player) {
    if (!player) return;
    player->takeDurabilityDamage(GameConfig::REEF_DAMAGE);
    player->applyStun(GameConfig::STUN_DURATION_MS);

    QVector2D dirVec(player->worldPos() - m_worldPos);
    if (!dirVec.isNull()) {
        dirVec.normalize();
        player->applyRebound(dirVec.toPointF() * GameConfig::REEF_REBOUND_FACTOR * player->currentSpeed());
    }
}

Whirlpool::Whirlpool(const QPointF& worldPos)
    : Obstacle(ObstacleType::WHIRLPOOL, worldPos), m_speedReduction(0), m_timeInWhirlpool(0) {}

void Whirlpool::update(qreal deltaTime) const { Q_UNUSED(deltaTime); }

void Whirlpool::onPlayerCollision(Player* player) {
    if (!player) return;
    m_timeInWhirlpool += 0.016f;
    m_speedReduction = qMin(m_timeInWhirlpool * 0.5f, static_cast<qreal>(GameConfig::WHIRLPOOL_MAX_SPEED_REDUCTION));
    player->applySpeedReduction(m_speedReduction);
}

ObstacleManager& ObstacleManager::instance() {
    static ObstacleManager mgr;
    return mgr;
}

void ObstacleManager::generateLevel(int level) {
    clear();
    int obstacleCount = 6 + level * 2;
    for (int i = 0; i < obstacleCount; ++i) {
        qreal x = 500.0 + QRandomGenerator::global()->generateDouble() * 9500.0;
        qreal y = 50.0 + QRandomGenerator::global()->generateDouble() * (GameConfig::WINDOW_HEIGHT - 150.0);
        ObstacleType type = (QRandomGenerator::global()->generate() % 2 == 0) ? ObstacleType::REEF : ObstacleType::WHIRLPOOL;
        m_obstacles.append(type == ObstacleType::REEF
                               ? static_cast<Obstacle*>(new Reef({x, y}))
                               : static_cast<Obstacle*>(new Whirlpool({x, y})));
    }
}

void ObstacleManager::update(qreal deltaTime) {
    // ========== 彻底修复：用传统for循环代替范围for，彻底消除detach警告 ==========
    for (int i = 0; i < m_obstacles.size(); ++i) {
        m_obstacles[i]->update(deltaTime);
    }
}

void ObstacleManager::clear() {
    qDeleteAll(m_obstacles);
    m_obstacles.clear();
}