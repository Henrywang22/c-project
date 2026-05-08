#pragma once
#include <QRect>

class Fish {
public:
    enum Type { SARDINE, TUNA, DEEPSEAEEL, SWORDFISH_FISH };

    Fish(int x, int y, Type type);
    virtual ~Fish() {}
    virtual void update(int playerX, int playerY);
    virtual int getEconomicValue() = 0;
    virtual int getCookingValue() = 0;
    bool isNearPlayer(int px, int py, int range);

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
    int lifeTimer = 0;
    int maxLife;
    bool fleeing = false;
    int fleeCooldown = 0;

protected:
    int moveTimer = 0;
    void changeDirection();
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
};#pragma once
