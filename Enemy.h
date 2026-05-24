#pragma once
#include "Player.h"

// ============================================================
// Enemy — 所有敌人的基类
// ============================================================
class Enemy {
public:
    Enemy(int x, int y);
    virtual ~Enemy() {}
    virtual void update(Player& player) = 0;
    virtual bool collidesWithPlayer(int px, int py) = 0;

    int x, y;
    int hp;
    int maxHp;
    int attack;
    float speed;
    bool alive = true;
    int attackTimer = 0;
    int dropValue;
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

    State state = IDLE;
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

    bool isInvisible = false;  // 是否处于隐身状态
    int contactTimer = 0;      // 接触玩家计时（达到30帧触发视野遮挡）

private:
    int invisTimer = 0;
    float posX, posY;
};
