#include "Enemy.h"
#include "Player.h"
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

    float dx = (float)(player.worldPos().x() - posX);
    float dy = (float)(player.worldPos().y() - posY);
    float dist = sqrt(dx * dx + dy * dy);

    if (dist > 0) {
        posX += speed * dx / dist;
        posY += speed * dy / dist;
        x = (int)posX;
        y = (int)posY;
    }

    if (posY < 60)  { posY = 60;  y = 60; }
    if (posY > 700) { posY = 700; y = 700; }
}

bool Shark::collidesWithPlayer(int px, int py)
{
    int dx = px - x;
    int dy = py - y;
    return (dx * dx + dy * dy) < (30 * 30);
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
        x = (int)posX;
        y = (int)posY;

        // 每200帧随机改变巡逻方向
        if (patrolTimer % 200 == 0) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            patrolVx = cos(angle) * speed;
            patrolVy = sin(angle) * speed;
        }

        // 发现玩家（距离小于200）时进入蓄力状态
        if (dist > 0.001f && dist < 200) {
            state = WINDUP;
            windupTimer = 0;
            chargeVx = dx / dist * 8.0f;
            chargeVy = dy / dist * 8.0f;
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
    int dx = px - x;
    int dy = py - y;
    return (dx * dx + dy * dy) < (25 * 25);
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
        posX += speed * dx / dist;
        posY += speed * dy / dist;
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
    int dx = px - x;
    int dy = py - y;
    return (dx * dx + dy * dy) < (30 * 30);
}
