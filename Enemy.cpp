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

    if (biteCooldown > 0) {
        --biteCooldown;
    }

    if (retreatTimer > 0) {
        --retreatTimer;
        posX += retreatVx;
        posY += retreatVy;
        if (std::fabs(retreatVx) > 0.01f) facingX = retreatVx < 0.0f ? -1.0f : 1.0f;
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
        if (std::fabs(vx) > 0.01f) facingX = vx < 0.0f ? -1.0f : 1.0f;
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
    if (std::fabs(patrolVx) > 0.01f) facingX = patrolVx < 0.0f ? -1.0f : 1.0f;
}

void Swordfish::update(Player& player)
{
    if (!alive) return;

    float dx = (float)(player.worldPos().x() - x);
    float dy = (float)(player.worldPos().y() - y);
    float dist = sqrt(dx * dx + dy * dy);

    switch (state) {
    case IDLE:
        // 巡逻移动
        patrolTimer++;
        posX += patrolVx;
        posY += patrolVy;
        if (std::fabs(patrolVx) > 0.01f) facingX = patrolVx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;

        // 每200帧随机改变巡逻方向
        if (patrolTimer % 200 == 0) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            patrolVx = cos(angle) * speed;
            patrolVy = sin(angle) * speed;
            if (std::fabs(patrolVx) > 0.01f) facingX = patrolVx < 0.0f ? -1.0f : 1.0f;
        }

        // 发现玩家（距离小于200）时进入蓄力状态
        if (dist > 0.001f && dist < 200) {
            state = WINDUP;
            windupTimer = 0;
            chargeVx = dx / dist * 8.0f;
            chargeVy = dy / dist * 8.0f;
            if (std::fabs(chargeVx) > 0.01f) facingX = chargeVx < 0.0f ? -1.0f : 1.0f;
        }

        if (posY < 60) { posY = 60;  patrolVy = abs(patrolVy); }
        if (posY > 700) { posY = 700; patrolVy = -abs(patrolVy); }
        break;

    case WINDUP:
        // 蓄力60帧后冲刺
        windupTimer++;
        if (windupTimer >= 60) state = CHARGE;
        break;

    case CHARGE:
        // 高速冲刺
        posX += chargeVx;
        posY += chargeVy;
        if (std::fabs(chargeVx) > 0.01f) facingX = chargeVx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;

        // 冲出范围后重置
        if (x < -100 || x > 6000 || y < 0 || y > 800) {
            state = IDLE;
            posX = (float)(player.worldPos().x() + 300 + rand() % 200);
            posY = (float)(80 + rand() % 580);
            x = (int)posX;
            y = (int)posY;
        }
        break;
    }
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
}

void Octopus::update(Player& player)
{
    if (!alive) return;

    invisTimer++;

    // 每300帧切换隐身状态
    if (invisTimer % 300 == 0) {
        isInvisible = !isInvisible;
    }

    // 隐身时不移动
    if (isInvisible) return;

    // 可见时缓慢追踪玩家
    float dx = (float)(player.worldPos().x() - x);
    float dy = (float)(player.worldPos().y() - y);
    float dist = sqrt(dx * dx + dy * dy);

    if (dist > 0 && dist < 300) {
        const float vx = speed * dx / dist;
        const float vy = speed * dy / dist;
        posX += vx;
        posY += vy;
        if (std::fabs(vx) > 0.01f) facingX = vx < 0.0f ? -1.0f : 1.0f;
        x = (int)posX;
        y = (int)posY;
    }

    if (posY < 60) { posY = 60;  y = (int)posY; }
    if (posY > 700) { posY = 700; y = (int)posY; }

    // 离开玩家后重置接触计时
    if (!collidesWithPlayer((int)player.worldPos().x(), (int)player.worldPos().y())) {
        contactTimer = 0;
    }
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
