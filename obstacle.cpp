#include "GameConfig.h"
#include "Obstacle.h"
#include "Player.h"
#include "WeatherSystem.h"
#include <QRandomGenerator>
#include <QVector2D>
#include <QLineF>

using namespace Config;

Obstacle::Obstacle(ObstacleType type, const QPointF& worldPos)
    : m_type(type), m_worldPos(worldPos), m_size(30) {}

void Obstacle::update(qreal deltaTime) { Q_UNUSED(deltaTime); }

QRectF Obstacle::collider() const {
    return {m_worldPos.x() - m_size/2.0f, m_worldPos.y() - m_size/2.0f,
            static_cast<qreal>(m_size), static_cast<qreal>(m_size)};
}

void Obstacle::onPlayerCollision(Player* player) { Q_UNUSED(player); }

bool Obstacle::isVisible(const QPointF& playerPos) const {
    qreal visionRange =
        GameConfig::VISION_RANGE * WeatherSystem::instance().currentVisionMultiplier();
    return QLineF(m_worldPos, playerPos).length() < visionRange;
}

Reef::Reef(const QPointF& worldPos) : Obstacle(ObstacleType::REEF, worldPos) {
    m_size = GameConfig::REEF_MIN_SIZE +
             QRandomGenerator::global()->generate() % (GameConfig::REEF_MAX_SIZE - GameConfig::REEF_MIN_SIZE + 1);
}

QRectF Reef::collider() const {
    const qreal targetW = qMax<qreal>(104.0, m_size * 5.0);
    const qreal targetH = qMax<qreal>(78.0, m_size * 4.0);
    const qreal colliderW = targetW * 0.58;
    const qreal colliderH = targetH * 0.54;
    const qreal colliderCenterY = m_worldPos.y() - m_size * 0.82;
    return QRectF(
        m_worldPos.x() - colliderW / 2.0,
        colliderCenterY - colliderH / 2.0,
        colliderW,
        colliderH
    );
}

void Reef::onPlayerCollision(Player* player) {
    if (!player) return;

    if (m_collisionCooldown.isValid() &&
        m_collisionCooldown.elapsed() < GameConfig::REEF_COLLISION_COOLDOWN_MS) {
        return;
    }

    QVector2D dirVec(player->worldPos() - m_worldPos);
    if (dirVec.isNull()) {
        dirVec = QVector2D(player->worldPos().x() < m_worldPos.x() ? -1.0f : 1.0f, 0.0f);
    }

    dirVec.normalize();
    player->takeDurabilityDamage(GameConfig::REEF_DAMAGE);
    player->applyStun(GameConfig::STUN_DURATION_MS);
    player->applyRebound(dirVec.toPointF() * GameConfig::REEF_REBOUND_FACTOR);
    m_collisionCooldown.restart();
}

Whirlpool::Whirlpool(const QPointF& worldPos)
    : Obstacle(ObstacleType::WHIRLPOOL, worldPos),
      m_speedReduction(0),
      m_timeInWhirlpool(0),
      m_touchedThisFrame(false) {}

QRectF Whirlpool::collider() const {
    const qreal targetSize = qMax<qreal>(92.0, m_size * 4.0);
    const qreal pullSize = targetSize * 0.78;
    return QRectF(
        m_worldPos.x() - pullSize / 2.0,
        m_worldPos.y() - pullSize / 2.0,
        pullSize,
        pullSize
    );
}

void Whirlpool::update(qreal deltaTime) {
    if (!m_touchedThisFrame) {
        m_timeInWhirlpool = qMax<qreal>(0.0, m_timeInWhirlpool - deltaTime);
        m_speedReduction =
            qMin(m_timeInWhirlpool * 0.5f, static_cast<qreal>(GameConfig::WHIRLPOOL_MAX_SPEED_REDUCTION));
    }

    m_touchedThisFrame = false;
}

void Whirlpool::onPlayerCollision(Player* player) {
    if (!player) return;
    m_touchedThisFrame = true;
    m_timeInWhirlpool += 0.016f;
    m_speedReduction = qMin(m_timeInWhirlpool * 0.5f, static_cast<qreal>(GameConfig::WHIRLPOOL_MAX_SPEED_REDUCTION));
    player->applySpeedReduction(m_speedReduction);
}

ObstacleManager& ObstacleManager::instance() {
    static ObstacleManager mgr;
    return mgr;
}

void ObstacleManager::generateLevel(int level, int reefCount, int whirlpoolCount) {
    clear();
    if (reefCount < 0 || whirlpoolCount < 0) {
        reefCount = qMax(0, 4 + level);
        whirlpoolCount = qMax(0, level - 1);
    }

    int obstacleCount = qMin(16, qMax(0, reefCount + whirlpoolCount));
    if (obstacleCount <= 0) {
        return;
    }

    const qreal stageStart = qMax<qreal>(
        520.0,
        GameConfig::stageStartDistance(level) + 360.0
    );
    qreal stageEnd = qMin<qreal>(
        GameConfig::stageConfig(level).targetDistance - 260.0,
        GameConfig::RIGHT_BORDER - 520.0
    );
    if (stageEnd <= stageStart) {
        stageEnd = stageStart + 600.0;
    }

    int placedReefs = 0;
    int placedWhirlpools = 0;
    int attempts = 0;
    while (m_obstacles.size() < obstacleCount && attempts < obstacleCount * 20) {
        ++attempts;
        qreal x = stageStart + QRandomGenerator::global()->generateDouble() * (stageEnd - stageStart);
        qreal y = GameConfig::TOP_BORDER + 70.0 +
                  QRandomGenerator::global()->generateDouble() *
                  (GameConfig::BOTTOM_BORDER - GameConfig::TOP_BORDER - 140.0);

        bool tooClose = false;
        for (auto* existing : m_obstacles) {
            if (QLineF(existing->worldPos(), QPointF(x, y)).length() < 230.0) {
                tooClose = true;
                break;
            }
        }
        if (tooClose) {
            continue;
        }

        const int remainingReefs = qMax(0, reefCount - placedReefs);
        const int remainingWhirlpools = qMax(0, whirlpoolCount - placedWhirlpools);
        const int remainingTotal = qMax(1, remainingReefs + remainingWhirlpools);
        ObstacleType type = ObstacleType::REEF;
        if (remainingReefs <= 0) {
            type = ObstacleType::WHIRLPOOL;
        }
        else if (remainingWhirlpools > 0 &&
                 QRandomGenerator::global()->bounded(remainingTotal) < remainingWhirlpools) {
            type = ObstacleType::WHIRLPOOL;
        }

        if (type == ObstacleType::REEF) {
            ++placedReefs;
        }
        else {
            ++placedWhirlpools;
        }

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
