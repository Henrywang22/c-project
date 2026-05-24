// Boss.h
#pragma once
#include "Enemy.h"
#include <vector>

class Boss : public Enemy {
public:
    enum State { PHASE1, PHASE2 };
    Boss(int x, int y) : Enemy(x, y) { hp = 500; maxHp = 500; attack = 20; dropValue = 200; }
    void update(int px, int py) override {}
    bool collidesWithPlayer(int px, int py) override { return false; }
    void spawnMinions(std::vector<Shark*>& sharks) {}

    State state = PHASE1;
    bool minionSpawned = false;
};
