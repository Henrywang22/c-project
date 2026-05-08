#include "Fish.h"
#include <cstdlib>
#include <cmath>

// ============================================================
// Fish 基类
// ============================================================

Fish::Fish(int x, int y, Type type)
    : x(x), y(y), type(type), maxLife(900 + rand() % 300)
{
    value = 0;
    staminaGain = 0;
    staminaCost = 10; // 默认普通捕鱼消耗10体力
    catchRequired = 3;
    catchTimeLimit = 180;
    vx = 0; vy = 0;
}

// 随机改变游动方向，保持原有速度大小
void Fish::changeDirection()
{
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = sqrt(vx * vx + vy * vy);
    if (speed < 0.5f) speed = 1.0f;
    vx = speed * cos(angle);
    vy = speed * sin(angle);
}

// 判断鱼是否在玩家捕鱼范围内
bool Fish::isNearPlayer(int px, int py, int range)
{
    int dx = px - x;
    int dy = py - y;
    return (dx * dx + dy * dy) <= (range * range);
}

// 基类默认update（子类会覆盖）
void Fish::update(int playerX, int playerY)
{
    lifeTimer++;
    moveTimer++;
    if (lifeTimer >= maxLife) { escaped = true; return; }
    x += (int)vx;
    y += (int)vy;
    if (x < 0 || x > 5000) { escaped = true; return; }
    if (y < 60) { y = 60;  vy = abs(vy); }
    if (y > 700) { y = 700; vy = -abs(vy); }
}

// ============================================================
// CommonFish — 普通鱼（沙丁鱼、金枪鱼）
// ============================================================

CommonFish::CommonFish(int x, int y, Type type) : Fish(x, y, type)
{
    // 随机初始游动方向，速度慢
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = 0.8f + (rand() % 5) * 0.1f;
    vx = speed * cos(angle);
    vy = speed * sin(angle);
}

void CommonFish::update(int playerX, int playerY)
{
    lifeTimer++;
    moveTimer++;

    if (lifeTimer >= maxLife) { escaped = true; return; }
    if (fleeCooldown > 0) fleeCooldown--;

    // 感知玩家：距离小于阈值时向反方向逃跑
    int dx = x - playerX;
    int dy = y - playerY;
    float dist = sqrt((float)(dx * dx + dy * dy));
    if (dist < 120 && fleeCooldown <= 0 && !fleeing) {
        fleeing = true;
        fleeCooldown = 180;
        float len = dist > 0 ? dist : 1;
        vx = (dx / len) * 2.0f; // 逃跑速度加快
        vy = (dy / len) * 2.0f;
    }
    if (fleeing && fleeCooldown <= 120) fleeing = false;

    // 每120帧随机改变方向（不在逃跑时）
    if (moveTimer % 120 == 0 && !fleeing) changeDirection();

    x += (int)vx;
    y += (int)vy;

    if (x < 0 || x > 5000) { escaped = true; return; }
    if (y < 60) { y = 60;  vy = abs(vy); }
    if (y > 700) { y = 700; vy = -abs(vy); }
}

// ============================================================
// RareFish — 稀有鱼（深海鳗、金鱼）
// ============================================================

RareFish::RareFish(int x, int y, Type type) : Fish(x, y, type)
{
    // 速度更快，更难捕
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = 1.5f + (rand() % 8) * 0.1f;
    vx = speed * cos(angle);
    vy = speed * sin(angle);
}

void RareFish::update(int playerX, int playerY)
{
    lifeTimer++;
    moveTimer++;

    if (lifeTimer >= maxLife) { escaped = true; return; }
    if (fleeCooldown > 0) fleeCooldown--;

    // 感知范围更大，逃跑更快
    int dx = x - playerX;
    int dy = y - playerY;
    float dist = sqrt((float)(dx * dx + dy * dy));
    if (dist < 150 && fleeCooldown <= 0) {
        fleeing = true;
        fleeCooldown = 120;
        float len = dist > 0 ? dist : 1;
        vx = (dx / len) * 3.5f;
        vy = (dy / len) * 3.5f;
    }
    if (fleeing && fleeCooldown <= 60) fleeing = false;

    // 每80帧随机改变方向（更频繁，更难预判）
    if (moveTimer % 80 == 0 && !fleeing) changeDirection();

    x += (int)vx;
    y += (int)vy;

    if (x < 0 || x > 5000) { escaped = true; return; }
    if (y < 60) { y = 60;  vy = abs(vy); }
    if (y > 700) { y = 700; vy = -abs(vy); }
}

// ============================================================
// Sardine — 沙丁鱼（价值低，最好捕）
// ============================================================

Sardine::Sardine(int x, int y) : CommonFish(x, y, SARDINE)
{
    value = 5 + rand() % 11;  // 5~15
    staminaGain = 10;
    staminaCost = 10;  // 普通完成消耗10体力
    catchRequired = 3;
    catchTimeLimit = 180; // 3秒
}

// ============================================================
// Tuna — 金枪鱼（价值中，普通难度）
// ============================================================

Tuna::Tuna(int x, int y) : CommonFish(x, y, TUNA)
{
    value = 25 + rand() % 31; // 25~55
    staminaGain = 20;
    staminaCost = 10;
    catchRequired = 3;
    catchTimeLimit = 180; // 3秒
}

// ============================================================
// DeepSeaEel — 深海鳗（价值高，难捕）
// ============================================================

DeepSeaEel::DeepSeaEel(int x, int y) : RareFish(x, y, DEEPSEAEEL)
{
    value = 80 + rand() % 61;  // 80~140
    staminaGain = 5;
    staminaCost = 10;
    catchRequired = 8;
    catchTimeLimit = 90; // 1.5秒，很紧张
}

// ============================================================
// GoldenFish — 金鱼（价值极高，极难捕）
// ============================================================

GoldenFish::GoldenFish(int x, int y) : RareFish(x, y, SWORDFISH_FISH)
{
    value = 150 + rand() % 101; // 150~250
    staminaGain = 30;
    staminaCost = 10;
    catchRequired = 10;
    catchTimeLimit = 75; // 1.25秒，极限
}