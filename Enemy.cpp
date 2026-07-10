#include "Enemy.h"
#include "Player.h"
#include "GameConfig.h"
#include <cmath>
#include <cstdlib>

// ============================================================
// Enemy 基类
// ============================================================

Enemy::Enemy(int x, int y)
    : x(x), y(y), hp(100), maxHp(100),
    attack(10), speed(2.0f), dropValue(50) {
}

void Enemy::takeDamage(int damage)
{
    if (!alive) return;
    hp -= damage;
    if (hp <= 0) {
        hp = 0;
        alive = false;
    }
}

void Enemy::applyKnockback(const QPointF& origin, qreal strength)
{
    if (!alive || strength <= 0.0) return;

    QPointF direction = position() - origin;
    qreal length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
    if (length <= 0.001) {
        direction = QPointF(facingX >= 0.0f ? 1.0 : -1.0, 0.0);
        length = 1.0;
    }

    const qreal firstStep = strength * 0.30;
    m_knockbackVelocity = QPointF(
        direction.x() / length * firstStep,
        direction.y() / length * firstStep
    );
    m_knockbackFrames = 8;
    advanceKnockback();
}

void Enemy::applyStun(int durationMs)
{
    if (!alive || durationMs <= 0) return;
    m_stunFrames = qMax(m_stunFrames, qMax(1, durationMs / 16));
}

void Enemy::applySlow(int durationMs, qreal movementMultiplier)
{
    if (!alive || durationMs <= 0) return;
    m_slowFrames = qMax(m_slowFrames, qMax(1, durationMs / 16));
    m_slowMultiplier = qMin(m_slowMultiplier,
                            qBound<qreal>(0.2, movementMultiplier, 1.0));
}

bool Enemy::isStunned() const
{
    return m_stunFrames > 0;
}

bool Enemy::isSlowed() const
{
    return m_slowFrames > 0;
}

void Enemy::applyStageScaling(int stage)
{
    stage = qMax(1, stage);
    if (m_scaledStage == stage) return;

    const qreal hpScale = 1.0 + 0.15 * (stage - 1);
    const qreal attackScale = 1.0 + 0.12 * (stage - 1);
    const qreal speedScale = 1.0 + 0.025 * (stage - 1);
    const qreal rewardScale = 1.0 + 0.15 * (stage - 1);
    maxHp = qMax(1, qRound(maxHp * hpScale));
    hp = maxHp;
    attack = qMax(0, qRound(attack * attackScale));
    speed = static_cast<float>(speed * speedScale);
    dropValue = qMax(1, qRound(dropValue * rewardScale));
    m_scaledStage = stage;
}

bool Enemy::advanceKnockback()
{
    if (m_knockbackFrames > 0) {
        setPosition(position() + m_knockbackVelocity);
        m_knockbackVelocity *= 0.72;
        --m_knockbackFrames;
        return true;
    }

    if (m_stunFrames > 0) {
        --m_stunFrames;
        return true;
    }

    if (m_slowFrames > 0) {
        --m_slowFrames;
        ++m_slowPhase;
        const bool skipMovement = m_slowMultiplier <= 0.42
            ? (m_slowPhase % 3 != 0)
            : (m_slowPhase % 2 == 0);
        if (m_slowFrames == 0) m_slowMultiplier = 1.0;
        if (skipMovement) return true;
    }

    return false;
}

QRectF Enemy::collider() const
{
    return QRectF(
        x - Config::GameConfig::BOSS_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::BOSS_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::BOSS_COLLIDER_WIDTH,
        Config::GameConfig::BOSS_COLLIDER_HEIGHT
    );
}

QPointF Enemy::position() const
{
    return QPointF(x, y);
}

void Enemy::setPosition(const QPointF& pos)
{
    x = static_cast<int>(std::round(pos.x()));
    y = static_cast<int>(std::round(pos.y()));
}

// ============================================================
// Shark — 普通鲨鱼
// ============================================================

Shark::Shark(int x, int y) : Enemy(x, y)
{
    hp = 100;
    maxHp = 100;
    attack = 10;
    speed = 2.0f;
    dropValue = 30;
    posX = (float)x;
    posY = (float)y;
}

void Shark::update(Player& player)
{
    if (!alive) return;
    if (advanceKnockback()) return;

    if (biteCooldown > 0) {
        --biteCooldown;
    }

    if (retreatTimer > 0) {
        --retreatTimer;
        posX += retreatVx;
        posY += retreatVy;
        if (std::fabs(retreatVx) > 0.30f) facingX = retreatVx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;

        if (posY < Config::GameConfig::TOP_BORDER) {
            posY = Config::GameConfig::TOP_BORDER;
            y = (int)posY;
            retreatVy = std::fabs(retreatVy);
        }
        if (posY > Config::GameConfig::BOTTOM_BORDER) {
            posY = Config::GameConfig::BOTTOM_BORDER;
            y = (int)posY;
            retreatVy = -std::fabs(retreatVy);
        }
        return;
    }

    float dx = (float)(player.worldPos().x() - posX);
    float dy = (float)(player.worldPos().y() - posY);
    float dist = sqrt(dx * dx + dy * dy);

    if (dist > 0) {
        const float vx = speed * dx / dist;
        const float vy = speed * dy / dist;
        posX += vx;
        posY += vy;
        if (std::fabs(vx) > 0.30f) facingX = vx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;
    }

    if (posY < Config::GameConfig::TOP_BORDER) {
        posY = Config::GameConfig::TOP_BORDER;
        y = (int)posY;
    }
    if (posY > Config::GameConfig::BOTTOM_BORDER) {
        posY = Config::GameConfig::BOTTOM_BORDER;
        y = (int)posY;
    }
}

bool Shark::collidesWithPlayer(int px, int py)
{
    QRectF playerRect(
        px - Config::GameConfig::PLAYER_COLLIDER_WIDTH / 2.0,
        py - Config::GameConfig::PLAYER_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::PLAYER_COLLIDER_WIDTH,
        Config::GameConfig::PLAYER_COLLIDER_HEIGHT
    );
    return collider().intersects(playerRect);
}

QRectF Shark::collider() const
{
    return QRectF(
        x - Config::GameConfig::SHARK_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::SHARK_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::SHARK_COLLIDER_WIDTH,
        Config::GameConfig::SHARK_COLLIDER_HEIGHT
    );
}

QRectF Shark::biteCollider() const
{
    const qreal bodyHalf = Config::GameConfig::SHARK_COLLIDER_WIDTH / 2.0;
    const qreal reach = Config::GameConfig::SHARK_BITE_REACH;
    const qreal height = Config::GameConfig::SHARK_BITE_HEIGHT;
    const bool facingRight = facingX > 0.0f;
    const qreal left = facingRight
        ? x + bodyHalf - reach * 0.25
        : x - bodyHalf - reach * 0.75;
    return QRectF(left, y - height / 2.0, reach, height);
}

QPointF Shark::position() const
{
    return QPointF(posX, posY);
}

void Shark::setPosition(const QPointF& pos)
{
    posX = static_cast<float>(pos.x());
    posY = static_cast<float>(pos.y());
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

bool Shark::canBite() const
{
    return biteCooldown <= 0;
}

void Shark::startBiteCooldown(const QPointF& playerPos)
{
    biteCooldown = Config::GameConfig::SHARK_ATTACK_COOLDOWN_FRAMES;
    retreatTimer = Config::GameConfig::SHARK_RETREAT_FRAMES;

    float dx = (float)(posX - playerPos.x());
    float dy = (float)(posY - playerPos.y());
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist <= 0.001f) {
        dx = facingX >= 0.0f ? 1.0f : -1.0f;
        dy = 0.0f;
        dist = 1.0f;
    }

    const float retreatSpeed =
        speed * static_cast<float>(Config::GameConfig::SHARK_RETREAT_SPEED_MULTIPLIER);
    retreatVx = dx / dist * retreatSpeed;
    retreatVy = dy / dist * retreatSpeed;
}

// ============================================================
// Swordfish — 剑鱼
// ============================================================

Swordfish::Swordfish(int x, int y) : Enemy(x, y)
{
    hp = 80;
    maxHp = 80;
    attack = 25;
    speed = 1.5f;
    dropValue = 50;
    posX = (float)x;
    posY = (float)y;

    // 随机初始巡逻方向
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    patrolVx = cos(angle) * speed;
    patrolVy = sin(angle) * speed;
    if (std::fabs(patrolVx) > 0.30f) facingX = patrolVx < 0.0f ? -1.0f : 1.0f;
}

void Swordfish::update(Player& player)
{
    if (!alive) return;
    if (advanceKnockback()) return;

    float dx = (float)(player.worldPos().x() - x);
    float dy = (float)(player.worldPos().y() - y);
    float dist = sqrt(dx * dx + dy * dy);

    switch (state) {
    case IDLE:
        // 巡逻移动
        patrolTimer++;
        posX += patrolVx;
        posY += patrolVy;
        if (std::fabs(patrolVx) > 0.30f) facingX = patrolVx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;

        // 每200帧随机改变巡逻方向
        if (patrolTimer % 200 == 0) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            patrolVx = cos(angle) * speed;
            patrolVy = sin(angle) * speed;
            if (std::fabs(patrolVx) > 0.30f) facingX = patrolVx < 0.0f ? -1.0f : 1.0f;
        }

        // 发现玩家（距离小于200）时进入蓄力状态
        if (dist > 0.001f && dist < 200) {
            state = WINDUP;
            windupTimer = 0;
            chargeVx = dx / dist * 8.0f;
            chargeVy = dy / dist * 8.0f;
            if (std::fabs(chargeVx) > 0.30f) facingX = chargeVx < 0.0f ? -1.0f : 1.0f;
        }

        if (posY < 60) { posY = 60;  patrolVy = std::fabs(patrolVy); }
        if (posY > 700) { posY = 700; patrolVy = -std::fabs(patrolVy); }
        break;

    case WINDUP:
        // 蓄力60帧后冲刺
        windupTimer++;
        if (windupTimer >= 60) {
            state = CHARGE;
            chargeTimer = 0;
        }
        break;

    case CHARGE:
        // 高速冲刺
        posX += chargeVx;
        posY += chargeVy;
        ++chargeTimer;
        if (std::fabs(chargeVx) > 0.30f) facingX = chargeVx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;

        // 冲出范围后重置
        const bool reachedWorldEdge =
            posX <= 0.0f || posX >= Config::GameConfig::RIGHT_BORDER ||
            posY <= Config::GameConfig::TOP_BORDER ||
            posY >= Config::GameConfig::BOTTOM_BORDER;
        if (chargeTimer >= 72 || reachedWorldEdge) {
            posX = qBound(0.0f, posX, static_cast<float>(Config::GameConfig::RIGHT_BORDER));
            posY = qBound(static_cast<float>(Config::GameConfig::TOP_BORDER), posY,
                          static_cast<float>(Config::GameConfig::BOTTOM_BORDER));
            x = static_cast<int>(posX);
            y = static_cast<int>(posY);
            state = IDLE;
            chargeTimer = 0;
            patrolVx = chargeVx * 0.18f;
            patrolVy = chargeVy * 0.18f;
        }
        break;
    }
}

void Swordfish::takeDamage(int damage)
{
    state = IDLE;
    windupTimer = 0;
    chargeTimer = 0;
    chargeVx = 0.0f;
    chargeVy = 0.0f;
    Enemy::takeDamage(damage);
}

bool Swordfish::collidesWithPlayer(int px, int py)
{
    QRectF playerRect(
        px - Config::GameConfig::PLAYER_COLLIDER_WIDTH / 2.0,
        py - Config::GameConfig::PLAYER_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::PLAYER_COLLIDER_WIDTH,
        Config::GameConfig::PLAYER_COLLIDER_HEIGHT
    );
    return collider().intersects(playerRect);
}

QRectF Swordfish::collider() const
{
    return QRectF(
        x - Config::GameConfig::SWORDFISH_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::SWORDFISH_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::SWORDFISH_COLLIDER_WIDTH,
        Config::GameConfig::SWORDFISH_COLLIDER_HEIGHT
    );
}

QPointF Swordfish::position() const
{
    return QPointF(posX, posY);
}

void Swordfish::setPosition(const QPointF& pos)
{
    posX = static_cast<float>(pos.x());
    posY = static_cast<float>(pos.y());
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

// ============================================================
// Octopus — 墨鱼
// ============================================================

Octopus::Octopus(int x, int y) : Enemy(x, y)
{
    hp = 60;
    maxHp = 60;
    attack = 0;
    speed = 1.2f;
    dropValue = 40;
    posX = (float)x;
    posY = (float)y;
    inkCooldownFrames = 90 + rand() % 91;
}

void Octopus::update(Player& player)
{
    if (!alive) return;
    if (advanceKnockback()) return;

    invisTimer++;
    if (inkCooldownFrames > 0) {
        --inkCooldownFrames;
    }

    if (inkProjectileActive) {
        inkProjectilePos += inkProjectileVelocity;
        ++inkProjectileAge;
        --inkProjectileLife;

        const QRectF projectileRect(
            inkProjectilePos.x() - 18.0,
            inkProjectilePos.y() - 13.0,
            36.0,
            26.0
        );
        if (projectileRect.intersects(player.collider())) {
            if (player.canTakeDamage()) {
                player.applyInkBlind(3200);
            }
            inkProjectileActive = false;
            inkCooldownFrames = 250;
        }
        else if (inkProjectileLife <= 0 ||
                 inkProjectilePos.x() < 0 ||
                 inkProjectilePos.x() > Config::GameConfig::RIGHT_BORDER ||
                 inkProjectilePos.y() < Config::GameConfig::TOP_BORDER ||
                 inkProjectilePos.y() > Config::GameConfig::BOTTOM_BORDER) {
            inkProjectileActive = false;
            inkCooldownFrames = 210;
        }
    }

    if (!inkProjectileActive && inkWindupFrames <= 0) {
        const int invisPhase = invisTimer % 420;
        isInvisible = invisPhase >= 300 && invisPhase < 400;
    }

    float dx = (float)(player.worldPos().x() - x);
    float dy = (float)(player.worldPos().y() - y);
    float dist = sqrt(dx * dx + dy * dy);

    if (inkWindupFrames > 0) {
        isInvisible = false;
        --inkWindupFrames;
        if (inkWindupFrames == 0 && dist > 0.001f) {
            const QPointF direction(dx / dist, dy / dist);
            inkProjectilePos = position() + direction * 28.0;
            inkProjectileVelocity = direction * 6.2;
            inkProjectileLife = 72;
            inkProjectileAge = 0;
            inkProjectileActive = true;
        }
        return;
    }

    if (!isInvisible && !inkProjectileActive &&
        inkCooldownFrames <= 0 && dist >= 105.0f && dist <= 330.0f) {
        inkWindupFrames = 42;
        return;
    }

    if (isInvisible) {
        if (dist > 190.0f && dist < 420.0f) {
            posX += speed * 0.45f * dx / dist;
            posY += speed * 0.45f * dy / dist;
            x = (int)posX;
            y = (int)posY;
        }
        return;
    }

    if (dist > 0 && dist < 300) {
        const float vx = speed * dx / dist;
        const float vy = speed * dy / dist;
        posX += vx;
        posY += vy;
        if (std::fabs(vx) > 0.30f) facingX = vx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;
    }

    if (posY < 60) { posY = 60;  y = (int)posY; }
    if (posY > 700) { posY = 700; y = (int)posY; }
}

bool Octopus::collidesWithPlayer(int px, int py)
{
    QRectF playerRect(
        px - Config::GameConfig::PLAYER_COLLIDER_WIDTH / 2.0,
        py - Config::GameConfig::PLAYER_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::PLAYER_COLLIDER_WIDTH,
        Config::GameConfig::PLAYER_COLLIDER_HEIGHT
    );
    return collider().intersects(playerRect);
}

QRectF Octopus::collider() const
{
    return QRectF(
        x - Config::GameConfig::OCTOPUS_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::OCTOPUS_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::OCTOPUS_COLLIDER_WIDTH,
        Config::GameConfig::OCTOPUS_COLLIDER_HEIGHT
    );
}

QPointF Octopus::position() const
{
    return QPointF(posX, posY);
}

void Octopus::setPosition(const QPointF& pos)
{
    posX = static_cast<float>(pos.x());
    posY = static_cast<float>(pos.y());
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

ElectricRay::ElectricRay(int x, int y) : Enemy(x, y), posX(x), posY(y)
{
    hp = maxHp = 120;
    attack = 12;
    speed = 1.35f;
    dropValue = 65;
    pulseCooldownFrames = 100 + rand() % 100;
}

void ElectricRay::update(Player& player)
{
    if (!alive) return;
    if (advanceKnockback()) return;
    if (pulseCooldownFrames > 0) --pulseCooldownFrames;
    if (pulseVisualFrames > 0) --pulseVisualFrames;

    const QPointF delta = player.worldPos() - position();
    const qreal dist = std::hypot(delta.x(), delta.y());
    if (std::fabs(delta.x()) > 56.0) {
        facingX = delta.x() < 0.0 ? -1.0f : 1.0f;
    }

    if (pulseWarningFrames > 0) {
        --pulseWarningFrames;
        if (pulseWarningFrames == 0) {
            pulseVisualFrames = 14;
            if (dist <= pulseRadius() && player.canTakeDamage()) {
                player.takeDurabilityDamage(attack);
                player.applyStun(650);
            }
            pulseCooldownFrames = 260;
        }
        return;
    }

    if (pulseCooldownFrames <= 0 && dist <= 210.0) {
        pulseWarningFrames = 48;
        return;
    }

    if (dist > 0.001 && dist < 520.0) {
        qreal approach = 0.0;
        if (dist > 205.0) {
            approach = 1.0;
        } else if (dist < 165.0) {
            approach = -0.45;
        }

        if (std::fabs(approach) > 0.001) {
            const qreal vx = delta.x() / dist * speed * approach;
            const qreal vy = delta.y() / dist * speed * approach;
            posX += static_cast<float>(vx);
            posY += static_cast<float>(vy);
        }
    }
    posX = qBound(0.0f, posX, static_cast<float>(Config::GameConfig::RIGHT_BORDER));
    posY = qBound(static_cast<float>(Config::GameConfig::TOP_BORDER), posY,
                  static_cast<float>(Config::GameConfig::BOTTOM_BORDER));
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

bool ElectricRay::collidesWithPlayer(int px, int py)
{
    const QRectF playerRect(
        px - Config::GameConfig::PLAYER_COLLIDER_WIDTH / 2.0,
        py - Config::GameConfig::PLAYER_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::PLAYER_COLLIDER_WIDTH,
        Config::GameConfig::PLAYER_COLLIDER_HEIGHT
    );
    return collider().intersects(playerRect);
}

QRectF ElectricRay::collider() const
{
    return QRectF(
        x - Config::GameConfig::ELECTRIC_RAY_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::ELECTRIC_RAY_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::ELECTRIC_RAY_COLLIDER_WIDTH,
        Config::GameConfig::ELECTRIC_RAY_COLLIDER_HEIGHT
    );
}

int ElectricRay::pulseAnimationFrame() const
{
    if (pulseVisualFrames > 0) {
        return 2;
    }
    if (pulseWarningFrames > 0) {
        return pulseWarningFrames > 22 ? 0 : 1;
    }
    return 0;
}

qreal ElectricRay::pulseRadius() const
{
    return Config::GameConfig::ELECTRIC_RAY_PULSE_RADIUS;
}

QPointF ElectricRay::position() const
{
    return QPointF(posX, posY);
}

void ElectricRay::setPosition(const QPointF& pos)
{
    posX = static_cast<float>(pos.x());
    posY = static_cast<float>(pos.y());
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

PoisonJellyfish::PoisonJellyfish(int x, int y)
    : Enemy(x, y), posX(x), posY(y)
{
    hp = maxHp = 75;
    attack = 5;
    speed = 1.0f;
    dropValue = 55;
    driftPhase = static_cast<float>((rand() % 628) / 100.0);
}

void PoisonJellyfish::update(Player& player)
{
    if (!alive) return;
    if (advanceKnockback()) return;
    if (poisonCooldownFrames > 0) --poisonCooldownFrames;

    driftPhase += 0.035f;
    const QPointF delta = player.worldPos() - position();
    const qreal dist = std::hypot(delta.x(), delta.y());

    if (state == WINDUP) {
        if (--stateTimer <= 0) {
            state = STRIKE;
            stateTimer = 26;
            stingHit = false;
        }
    }
    else if (state == STRIKE) {
        const int frame = stingAnimationFrame();
        if (!stingHit && frame >= 2 && stingCollider().intersects(player.collider())) {
            if (player.canTakeDamage()) {
                player.takeDurabilityDamage(attack);
                player.applyPoison(4200);
                player.applyRebound(QPointF(facingX * 1.35, 0.0));
            }
            stingHit = true;
        }

        if (--stateTimer <= 0) {
            state = RETREAT;
            stateTimer = Config::GameConfig::SHARK_RETREAT_FRAMES;
            const qreal safeDist = qMax<qreal>(1.0, dist);
            retreatVelocity = QPointF(
                -delta.x() / safeDist * speed * Config::GameConfig::SHARK_RETREAT_SPEED_MULTIPLIER,
                -delta.y() / safeDist * speed * Config::GameConfig::SHARK_RETREAT_SPEED_MULTIPLIER
            );
            poisonCooldownFrames = 190;
        }
    }
    else if (state == RETREAT) {
        posX += static_cast<float>(retreatVelocity.x());
        posY += static_cast<float>(retreatVelocity.y());
        if (std::fabs(retreatVelocity.x()) > 0.30) {
            facingX = retreatVelocity.x() < 0.0 ? -1.0f : 1.0f;
        }
        if (--stateTimer <= 0) {
            state = DRIFT;
        }
    }
    else {
        if (poisonCooldownFrames <= 0 && dist <= 154.0) {
            state = WINDUP;
            stateTimer = 30;
            if (std::fabs(delta.x()) > 36.0) {
                facingX = delta.x() < 0.0 ? -1.0f : 1.0f;
            }
        }
        else if (dist > 0.001 && dist < 420.0) {
            const qreal approach = dist < 112.0 ? -0.32 : 0.78;
            const qreal vx = delta.x() / dist * speed * approach;
            const qreal vy = delta.y() / dist * speed * approach;
            posX += static_cast<float>(vx);
            posY += static_cast<float>(vy);
            if (std::fabs(vx) > 0.30) {
                facingX = vx < 0.0 ? -1.0f : 1.0f;
            }
        }
        posY += std::sin(driftPhase) * 0.34f;
    }

    posX = qBound(0.0f, posX, static_cast<float>(Config::GameConfig::RIGHT_BORDER));
    posY = qBound(static_cast<float>(Config::GameConfig::TOP_BORDER), posY,
                  static_cast<float>(Config::GameConfig::BOTTOM_BORDER));
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

bool PoisonJellyfish::collidesWithPlayer(int px, int py)
{
    const QRectF playerRect(
        px - Config::GameConfig::PLAYER_COLLIDER_WIDTH / 2.0,
        py - Config::GameConfig::PLAYER_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::PLAYER_COLLIDER_WIDTH,
        Config::GameConfig::PLAYER_COLLIDER_HEIGHT
    );
    return collider().intersects(playerRect);
}

QRectF PoisonJellyfish::collider() const
{
    return QRectF(
        x - Config::GameConfig::JELLYFISH_COLLIDER_WIDTH / 2.0,
        y - Config::GameConfig::JELLYFISH_COLLIDER_HEIGHT / 2.0,
        Config::GameConfig::JELLYFISH_COLLIDER_WIDTH,
        Config::GameConfig::JELLYFISH_COLLIDER_HEIGHT
    );
}

QRectF PoisonJellyfish::stingCollider() const
{
    const qreal bodyHalf = Config::GameConfig::JELLYFISH_COLLIDER_WIDTH / 2.0;
    const qreal reach = Config::GameConfig::JELLYFISH_STING_REACH;
    const qreal left = facingX < 0.0f
        ? x - bodyHalf - reach
        : x + bodyHalf;
    return QRectF(
        left,
        y - Config::GameConfig::JELLYFISH_STING_HEIGHT / 2.0,
        reach,
        Config::GameConfig::JELLYFISH_STING_HEIGHT
    );
}

int PoisonJellyfish::stingAnimationFrame() const
{
    if (state == WINDUP) {
        return stateTimer > 14 ? 0 : 1;
    }
    if (state == STRIKE) {
        return qBound(1, (26 - stateTimer) / 7 + 1, 3);
    }
    return 0;
}

QPointF PoisonJellyfish::position() const
{
    return QPointF(posX, posY);
}

void PoisonJellyfish::setPosition(const QPointF& pos)
{
    posX = static_cast<float>(pos.x());
    posY = static_cast<float>(pos.y());
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}
