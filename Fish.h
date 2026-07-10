#pragma once

#include <QPointF>
#include <QRectF>

class Fish {
public:
    enum Type {
        SARDINE,
        TUNA,
        DEEPSEAEEL,
        SWORDFISH_FISH,
        ANCHOVY,
        CLOWNFISH,
        MACKEREL,
        SEA_BREAM,
        LANTERNFISH,
        GROUPER,
        KOI,
        CRYSTAL_FISH
    };

    Fish(int x, int y, Type type);
    virtual ~Fish() {}
    virtual void update(int playerX, int playerY);
    virtual int getEconomicValue() = 0;
    virtual int getCookingValue() = 0;
    bool isNearPlayer(int px, int py, int range);
    QRectF collider() const;
    QPointF position() const;
    void setPosition(const QPointF& pos);
    void applyStun(int durationMs);
    void applySlow(int durationMs, qreal movementMultiplier);
    bool isStunned() const { return stunFrames > 0; }
    qreal statusMovementMultiplier() const;

    int x, y;
    float vx, vy;
    Type type;
    int value;
    int staminaGain;    // 捕获后恢复的体力
    int staminaCost;    // 捕鱼消耗的体力（完美捕获减半）
    int catchRequired;  // 需要按F的次数
    int catchTimeLimit; // 捕鱼时间限制（帧数）
    bool caught = false;
    bool escaped = false;
    bool lockedForCatch = false;
    int lifeTimer = 0;
    int maxLife;
    bool fleeing = false;
    int fleeCooldown = 0;
    float facingX = 1.0f;
    float cruiseSpeed = 1.0f;

protected:
    float posX, posY;
    int moveTimer = 0;
    int stunFrames = 0;
    int slowFrames = 0;
    qreal slowMultiplier = 1.0;
    void changeDirection();
    bool tickStatusEffects();
};

class CommonFish : public Fish {
public:
    CommonFish(int x, int y, Type type);
    void update(int playerX, int playerY) override;
    int getEconomicValue() override { return value; }
    int getCookingValue() override { return staminaGain; }
};

class RareFish : public Fish {
public:
    RareFish(int x, int y, Type type);
    void update(int playerX, int playerY) override;
    int getEconomicValue() override { return value; }
    int getCookingValue() override { return staminaGain; }
};

class Sardine : public CommonFish {
public:
    Sardine(int x, int y);
};

class Tuna : public CommonFish {
public:
    Tuna(int x, int y);
};

class DeepSeaEel : public RareFish {
public:
    DeepSeaEel(int x, int y);
};

class GoldenFish : public RareFish {
public:
    GoldenFish(int x, int y);
};

class Anchovy : public CommonFish {
public:
    Anchovy(int x, int y);
};

class Clownfish : public CommonFish {
public:
    Clownfish(int x, int y);
};

class Mackerel : public CommonFish {
public:
    Mackerel(int x, int y);
};

class SeaBream : public CommonFish {
public:
    SeaBream(int x, int y);
};

class Lanternfish : public RareFish {
public:
    Lanternfish(int x, int y);
};

class Grouper : public RareFish {
public:
    Grouper(int x, int y);
};

class KoiFish : public RareFish {
public:
    KoiFish(int x, int y);
};

class CrystalFish : public RareFish {
public:
    CrystalFish(int x, int y);
};
