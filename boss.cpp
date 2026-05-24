#include "Boss.h"
#include "Player.h"
#include <cmath>
#include <cstdlib>

Boss::Boss(int x, int y) : Enemy(x, y)
{
    hp = 500;
    maxHp = 500;
    attack = 20;
    dropValue = 200;
    speed = 1.5f;
}

void Boss::update(Player& player)
{
    if (!alive) return;

    // 追踪玩家
    float dx = (float)(player.worldPos().x() - x);
    float dy = (float)(player.worldPos().y() - y);
    float dist = sqrt(dx * dx + dy * dy);

    if (dist > 0) {
        x += (int)(speed * dx / dist);
        y += (int)(speed * dy / dist);
    }

    if (y < 60)  y = 60;
    if (y > 700) y = 700;

    // 半血进入PHASE2，加速
    if (hp <= maxHp / 2 && state == PHASE1) {
        state = PHASE2;
        speed = 3.0f;
    }
}

bool Boss::collidesWithPlayer(int px, int py)
{
    int dx = px - x;
    int dy = py - y;
    return (dx * dx + dy * dy) < (40 * 40);
}

void Boss::spawnMinions(std::vector<Shark*>& sharks)
{
    minionSpawned = true;
    for (int i = 0; i < 3; i++) {
        int sx = x + (rand() % 200) - 100;
        int sy = y + (rand() % 200) - 100;
        if (sy < 60) sy = 60;
        if (sy > 700) sy = 700;
        sharks.push_back(new Shark(sx, sy));
    }
}
