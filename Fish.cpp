#include "Fish.h"
#include "GameConfig.h"
#include <cstdlib>
#include <cmath>

// ============================================================
// Fish 基类
// ============================================================

Fish::Fish(int x, int y, Type type)
    : x(x), y(y), type(type), maxLife(0),
      posX((float)x), posY((float)y)
{
    value = 0;
    staminaGain = 0;
    staminaCost = 10; // 默认普通捕鱼消耗10体力
    catchRequired = 3;
    catchTimeLimit = 180;
    vx = 0; vy = 0;
}

QRectF Fish::collider() const
{
    qreal width = 42.0;
    qreal height = 22.0;
    switch (type) {
    case SARDINE:
        width = 34.0;
        height = 18.0;
        break;
    case TUNA:
        width = 48.0;
        height = 24.0;
        break;
    case DEEPSEAEEL:
        width = 56.0;
        height = 18.0;
        break;
    case SWORDFISH_FISH:
        width = 42.0;
        height = 22.0;
        break;
    case ANCHOVY:
        width = 40.0;
        height = 18.0;
        break;
    case CLOWNFISH:
        width = 38.0;
        height = 24.0;
        break;
    case MACKEREL:
        width = 52.0;
        height = 24.0;
        break;
    case SEA_BREAM:
        width = 46.0;
        height = 28.0;
        break;
    case LANTERNFISH:
        width = 44.0;
        height = 24.0;
        break;
    case GROUPER:
        width = 58.0;
        height = 30.0;
        break;
    case KOI:
        width = 52.0;
        height = 26.0;
        break;
    case CRYSTAL_FISH:
        width = 58.0;
        height = 28.0;
        break;
    }
    return QRectF(x - width / 2.0, y - height / 2.0, width, height);
}

QPointF Fish::position() const
{
    return QPointF(posX, posY);
}

void Fish::setPosition(const QPointF& pos)
{
    posX = static_cast<float>(pos.x());
    posY = static_cast<float>(pos.y());
    x = static_cast<int>(std::round(posX));
    y = static_cast<int>(std::round(posY));
}

void Fish::applyStun(int durationMs)
{
    if (caught || escaped || durationMs <= 0) return;
    stunFrames = qMax(stunFrames, qMax(1, durationMs / 16));
}

void Fish::applySlow(int durationMs, qreal movementMultiplier)
{
    if (caught || escaped || durationMs <= 0) return;
    slowFrames = qMax(slowFrames, qMax(1, durationMs / 16));
    slowMultiplier = qMin(slowMultiplier, qBound<qreal>(0.12, movementMultiplier, 1.0));
}

qreal Fish::statusMovementMultiplier() const
{
    return slowFrames > 0 ? slowMultiplier : 1.0;
}

bool Fish::tickStatusEffects()
{
    if (slowFrames > 0) {
        --slowFrames;
        if (slowFrames == 0) slowMultiplier = 1.0;
    }
    if (stunFrames > 0) {
        --stunFrames;
        return true;
    }
    if (std::fabs(vx) > 0.22f)
        facingX = vx < 0.0f ? -1.0f : 1.0f;
    return false;
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
    const qreal dx = static_cast<qreal>(px) - posX;
    const qreal dy = static_cast<qreal>(py) - posY;
    const qreal effectiveRange = static_cast<qreal>(range + Config::GameConfig::FISH_INTERACTION_RADIUS);
    return (dx * dx + dy * dy) <= (effectiveRange * effectiveRange);
}

// 基类默认update（子类会覆盖）
void Fish::update(int playerX, int playerY)
{
    Q_UNUSED(playerX);
    Q_UNUSED(playerY);
    if (tickStatusEffects()) return;
    moveTimer++;
    const float speedScale = (lockedForCatch
        ? static_cast<float>(Config::GameConfig::FISH_LOCKED_SPEED_MULTIPLIER)
        : 1.0f) * static_cast<float>(statusMovementMultiplier());
    posX += vx * speedScale;
    posY += vy * speedScale;
    x = (int)std::round(posX);
    y = (int)std::round(posY);
    if (x < 0) {
        x = 0;
        posX = 0;
        vx = std::fabs(vx);
    }
    if (x > Config::GameConfig::RIGHT_BORDER) {
        x = Config::GameConfig::RIGHT_BORDER;
        posX = static_cast<float>(x);
        vx = -std::fabs(vx);
    }
    if (y < 60) { y = 60;  posY = 60;  vy = std::fabs(vy); }
    if (y > 700) { y = 700; posY = 700; vy = -std::fabs(vy); }
}

// ============================================================
// CommonFish — 普通鱼（沙丁鱼、金枪鱼）
// ============================================================

CommonFish::CommonFish(int x, int y, Type type) : Fish(x, y, type)
{
    // 随机初始游动方向，速度慢
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = 0.8f + (rand() % 5) * 0.1f;
    cruiseSpeed = speed;
    vx = speed * cos(angle);
    vy = speed * sin(angle);
}

void CommonFish::update(int playerX, int playerY)
{
    if (tickStatusEffects()) return;
    moveTimer++;

    if (fleeCooldown > 0) fleeCooldown--;

    // 感知玩家：距离小于阈值时向反方向逃跑
    const qreal dx = posX - static_cast<qreal>(playerX);
    const qreal dy = posY - static_cast<qreal>(playerY);
    const float dist = static_cast<float>(std::sqrt(dx * dx + dy * dy));
    if (dist < 120 && fleeCooldown <= 0 && !fleeing) {
        fleeing = true;
        fleeCooldown = 180;
        float len = dist > 0 ? dist : 1;
        vx = (dx / len) * 2.0f; // 逃跑速度加快
        vy = (dy / len) * 2.0f;
    }
    if (fleeing && fleeCooldown <= 120) {
        fleeing = false;
        const float speed = std::hypot(vx, vy);
        if (speed > 0.001f) {
            vx = vx / speed * cruiseSpeed;
            vy = vy / speed * cruiseSpeed;
        }
        else {
            changeDirection();
        }
    }

    // 每120帧随机改变方向（不在逃跑时）
    if (moveTimer % 120 == 0 && !fleeing) changeDirection();

    const float speedScale = (lockedForCatch
        ? static_cast<float>(Config::GameConfig::FISH_LOCKED_SPEED_MULTIPLIER)
        : 1.0f) * static_cast<float>(statusMovementMultiplier());
    posX += vx * speedScale;
    posY += vy * speedScale;
    x = (int)std::round(posX);
    y = (int)std::round(posY);

    if (x < 0) {
        x = 0;
        posX = 0;
        vx = std::fabs(vx);
    }
    if (x > Config::GameConfig::RIGHT_BORDER) {
        x = Config::GameConfig::RIGHT_BORDER;
        posX = static_cast<float>(x);
        vx = -std::fabs(vx);
    }
    if (y < 60) { y = 60;  posY = 60;  vy = std::fabs(vy); }
    if (y > 700) { y = 700; posY = 700; vy = -std::fabs(vy); }
}

// ============================================================
// RareFish — 稀有鱼（深海鳗、金鱼）
// ============================================================

RareFish::RareFish(int x, int y, Type type) : Fish(x, y, type)
{
    // 速度更快，更难捕
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = 1.5f + (rand() % 8) * 0.1f;
    cruiseSpeed = speed;
    vx = speed * cos(angle);
    vy = speed * sin(angle);
}

void RareFish::update(int playerX, int playerY)
{
    if (tickStatusEffects()) return;
    moveTimer++;

    if (fleeCooldown > 0) fleeCooldown--;

    // 感知范围更大，逃跑更快
    const qreal dx = posX - static_cast<qreal>(playerX);
    const qreal dy = posY - static_cast<qreal>(playerY);
    const float dist = static_cast<float>(std::sqrt(dx * dx + dy * dy));
    if (dist < 150 && fleeCooldown <= 0) {
        fleeing = true;
        fleeCooldown = 120;
        float len = dist > 0 ? dist : 1;
        vx = (dx / len) * 3.5f;
        vy = (dy / len) * 3.5f;
    }
    if (fleeing && fleeCooldown <= 60) {
        fleeing = false;
        const float speed = std::hypot(vx, vy);
        if (speed > 0.001f) {
            vx = vx / speed * cruiseSpeed;
            vy = vy / speed * cruiseSpeed;
        }
        else {
            changeDirection();
        }
    }

    // 每80帧随机改变方向（更频繁，更难预判）
    if (moveTimer % 80 == 0 && !fleeing) changeDirection();

    const float speedScale = (lockedForCatch
        ? static_cast<float>(Config::GameConfig::FISH_LOCKED_SPEED_MULTIPLIER)
        : 1.0f) * static_cast<float>(statusMovementMultiplier());
    posX += vx * speedScale;
    posY += vy * speedScale;
    x = (int)std::round(posX);
    y = (int)std::round(posY);

    if (x < 0) {
        x = 0;
        posX = 0;
        vx = std::fabs(vx);
    }
    if (x > Config::GameConfig::RIGHT_BORDER) {
        x = Config::GameConfig::RIGHT_BORDER;
        posX = static_cast<float>(x);
        vx = -std::fabs(vx);
    }
    if (y < 60) { y = 60;  posY = 60;  vy = std::fabs(vy); }
    if (y > 700) { y = 700; posY = 700; vy = -std::fabs(vy); }
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
    catchTimeLimit = 120;
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
    catchTimeLimit = 75;
}

Anchovy::Anchovy(int x, int y) : CommonFish(x, y, ANCHOVY)
{
    value = 8 + rand() % 11;
    staminaGain = 10;
    staminaCost = 8;
    catchRequired = 3;
    catchTimeLimit = 190;
}

Clownfish::Clownfish(int x, int y) : CommonFish(x, y, CLOWNFISH)
{
    value = 12 + rand() % 13;
    staminaGain = 12;
    staminaCost = 8;
    catchRequired = 4;
    catchTimeLimit = 180;
}

Mackerel::Mackerel(int x, int y) : CommonFish(x, y, MACKEREL)
{
    value = 35 + rand() % 31;
    staminaGain = 18;
    staminaCost = 10;
    catchRequired = 4;
    catchTimeLimit = 165;
}

SeaBream::SeaBream(int x, int y) : CommonFish(x, y, SEA_BREAM)
{
    value = 45 + rand() % 36;
    staminaGain = 18;
    staminaCost = 10;
    catchRequired = 5;
    catchTimeLimit = 155;
}

Lanternfish::Lanternfish(int x, int y) : RareFish(x, y, LANTERNFISH)
{
    value = 95 + rand() % 66;
    staminaGain = 8;
    staminaCost = 12;
    catchRequired = 7;
    catchTimeLimit = 105;
}

Grouper::Grouper(int x, int y) : RareFish(x, y, GROUPER)
{
    value = 110 + rand() % 81;
    staminaGain = 15;
    staminaCost = 12;
    catchRequired = 7;
    catchTimeLimit = 120;
}

KoiFish::KoiFish(int x, int y) : RareFish(x, y, KOI)
{
    value = 180 + rand() % 101;
    staminaGain = 25;
    staminaCost = 14;
    catchRequired = 9;
    catchTimeLimit = 90;
}

CrystalFish::CrystalFish(int x, int y) : RareFish(x, y, CRYSTAL_FISH)
{
    value = 240 + rand() % 141;
    staminaGain = 30;
    staminaCost = 15;
    catchRequired = 11;
    catchTimeLimit = 82;
}
