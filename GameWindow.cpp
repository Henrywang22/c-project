#include "GameWindow.h"
#include "ShopDialog.h"
#include "Obstacle.h"
#include <QPainter>
#include <QFont>
#include <QKeyEvent>
#include <algorithm>

// ============================================================
// 构造/析构
// ============================================================

GameWindow::GameWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("渔途");
    setFixedSize(1280, 720);

    gm = new GameManager();

    // 加载图片
    imgSardine.load("sardine.png");
    imgTuna.load("tuna.png");
    imgEel.load("eel.png");
    imgGolden.load("golden.png");
    imgShark.load("shark.png");
    imgSwordfish.load("swordfish.png");
    imgOctopus.load("octopus.png");
    imgBoat.load("boat.png");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameWindow::gameLoop);
    timer->start(16);
}

GameWindow::~GameWindow()
{
    delete gm;
}

// ============================================================
// Qt事件：每帧绘制
// ============================================================

void GameWindow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    switch (state) {
    case STATE_INTRO:   drawIntro(p);   break;
    case STATE_MENU:    drawMenu(p);    break;
    case STATE_PLAYING:
    case STATE_PAUSED:  drawGame(p);    break;
    case STATE_DEFEAT:  drawDefeat(p);  break;
    case STATE_VICTORY: drawVictory(p); break;
    default: break;
    }
}

// ============================================================
// 游戏主循环
// ============================================================

void GameWindow::gameLoop()
{
    switch (state) {
    case STATE_PLAYING: {
        if (gm->gameOver) { state = STATE_DEFEAT;  update(); return; }
        if (gm->victory) { state = STATE_VICTORY; update(); return; }

        // 关卡通关
        if (gm->stageClear) {
            timer->stop();

            gm->fileManager.saveGame({
                gm->stage,
                gm->playerX(),
                0,  // coins（C同学Player没有）
                Player::instance().durability(),
                Player::instance().stamina(),
                0,  // fishCaught
                0,  // fishTotalValue
                gm->gameTimer / 60,
                false
                });

            openShop();
            const QList<Obstacle*>& obstacleList = ObstacleManager::instance().obstacles();
            for (Obstacle* o : obstacleList)
            ObstacleManager::instance().clear();
            for (auto s : gm->sharks) delete s;
            gm->sharks.clear();

            gm->stage++;
            gm->bossSpawned = false;
            gm->stageClear = false;
            isFishing = false;
            targetFish = nullptr;
            gm->spawnObstacles();

            timer->start(16);
            return;
        }

        // 把按键状态传给C同学的Player
        // C同学用keyPress/keyRelease接口，已在keyPressEvent里处理

        updateFishing();
        gm->update();
        break;
    }
    default: break;
    }
    update();
}

// ============================================================
// 背景海洋
// ============================================================

void GameWindow::drawSea(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(30, 100, 180));
    p.setPen(QPen(QColor(50, 130, 210), 1));
    for (int y = 80; y < 720; y += 60)
        for (int x = 0; x < 1280; x += 80)
            p.drawLine(x, y, x + 40, y);
}

// ============================================================
// 开场说明
// ============================================================

void GameWindow::drawIntro(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(20, 70, 140));
    p.setPen(QPen(QColor(50, 130, 210), 1));
    for (int y = 80; y < 720; y += 60)
        for (int x = 0; x < 1280; x += 80)
            p.drawLine(x, y, x + 40, y);

    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 42, QFont::Bold));
    p.drawText(0, 60, 1280, 100, Qt::AlignCenter, "渔  途");

    p.setFont(QFont("Microsoft YaHei", 14));
    p.setPen(QColor(180, 220, 255));
    p.drawText(0, 150, 1280, 30, Qt::AlignCenter, "—— 一场向右的海上冒险 ——");

    p.setPen(QPen(QColor(100, 160, 220), 1));
    p.drawLine(200, 195, 1080, 195);

    QStringList lines = {
        "【目标】  驾船向右航行，闯过 5 个关卡，击败每关的 Boss 鲨鱼",
        "",
        "【移动】  WASD 移动    Shift 加速（消耗体力）",
        "",
        "【捕鱼】  靠近鱼后按 F 开始捕捉，在倒计时内连续按 F 完成捕获",
        "             黄色沙丁鱼：价值低，易捕    蓝色金枪鱼：价值中，易捕",
        "             紫色深海鳗：价值高，难捕    金色金鱼：价值极高，极难捕",
        "",
        "【战斗】  空格键攻击附近鲨鱼（消耗武器耐久）",
        "             普通鲨鱼（蓝色）    Boss鲨鱼（红色，血量低于一半时进入狂暴）",
        "",
        "【障碍】  暗礁：碰撞损失耐久并反弹    漩涡：减少体力并降速",
        "",
        "【商店】  击败 Boss 后进入，按 P 随时打开    ESC 暂停",
        "",
        "【存档】  按 Q 保存并退出，下次可继续上一关",
    };

    p.setFont(QFont("Microsoft YaHei", 11));
    p.setPen(Qt::white);
    int startY = 210;
    for (const QString& line : lines) {
        if (line.isEmpty()) { startY += 6; continue; }
        p.drawText(160, startY, line);
        startY += 21;
    }

    p.setPen(QPen(QColor(100, 160, 220), 1));
    p.drawLine(200, 678, 1080, 678);

    static int blink = 0;
    blink++;
    if ((blink / 30) % 2 == 0) {
        p.setPen(QColor(255, 220, 80));
        p.setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
        p.drawText(0, 688, 1280, 30, Qt::AlignCenter, "按空格键开始游戏");
    }
}

// ============================================================
// 主菜单
// ============================================================

void GameWindow::drawMenu(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(20, 70, 140));
    p.setPen(QPen(QColor(50, 130, 210), 1));
    for (int y = 80; y < 720; y += 60)
        for (int x = 0; x < 1280; x += 80)
            p.drawLine(x, y, x + 40, y);

    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 42, QFont::Bold));
    p.drawText(0, 180, 1280, 100, Qt::AlignCenter, "渔  途");

    p.setFont(QFont("Microsoft YaHei", 20));
    p.setPen(QColor(255, 220, 80));
    p.drawText(0, 350, 1280, 50, Qt::AlignCenter, "按 N 新开游戏");

    if (gm->fileManager.hasSave()) {
        p.setPen(QColor(100, 220, 255));
        p.drawText(0, 420, 1280, 50, Qt::AlignCenter, "按 C 继续上一关");
    }
}

// ============================================================
// 游戏画面总入口
// ============================================================

void GameWindow::drawGame(QPainter& p)
{
    drawSea(p);
    drawObstacles(p);
    drawWaves(p);
    drawFish(p);
    drawBossHazards(p);
    drawSharks(p);
    drawPlayer(p);
    drawHUD(p);
    drawFishingHUD(p);

    // 天气叠加效果（用C同学的WeatherSystem）
    QColor overlay = WeatherSystem::instance().overlayColor();
    if (overlay.alpha() > 0)
        p.fillRect(0, 0, 1280, 720, overlay);

    if (state == STATE_PAUSED) drawPaused(p);
}

// ============================================================
// 鱼
// ============================================================

void GameWindow::drawFish(QPainter& p)
{
    for (auto f : gm->fish) {
        if (f->caught || f->escaped) continue;
        int screenX = f->x - gm->cameraX;
        if (screenX < -20 || screenX > 1300) continue;

        QPixmap* img = nullptr;
        switch (f->type) {
        case Fish::SARDINE:        img = &imgSardine; break;
        case Fish::TUNA:           img = &imgTuna;    break;
        case Fish::DEEPSEAEEL:     img = &imgEel;     break;
        case Fish::SWORDFISH_FISH: img = &imgGolden;  break;
        }

        if (img && !img->isNull()) {
            p.drawPixmap(screenX - 24, f->y - 12, 48, 24, *img);
        }
        else {
            p.setBrush(QColor(255, 220, 50));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - 8, f->y - 5, 16, 10);
        }

        if (!isFishing && f->isNearPlayer(gm->playerX(), gm->playerY(), 120)) {
            p.setPen(Qt::white);
            p.setFont(QFont("Microsoft YaHei", 10));
            p.drawText(screenX - 15, f->y - 14, "按F捕鱼");
        }
    }
}

// ============================================================
// 障碍物（用ObstacleManager）
// ============================================================

void GameWindow::drawObstacles(QPainter& p)
{
    const auto& obstacles = ObstacleManager::instance().obstacles();
    for (auto* o : obstacles) {
        QPointF playerPos(gm->playerX(), gm->playerY());
        if (!o->isVisible(playerPos)) continue;

        int screenX = (int)o->worldPos().x() - gm->cameraX;
        int screenY = (int)o->worldPos().y();
        int size = o->size();

        if (screenX < -size || screenX > 1280 + size) continue;

        if (o->type() == ObstacleType::REEF) {
            // 暗礁：棕色方块
            p.setBrush(QColor(120, 80, 40));
            p.setPen(QPen(QColor(80, 50, 20), 2));
            p.drawRect(screenX - size, screenY - size, size * 2, size * 2);
        }
        else {
            // 漩涡：半透明蓝圆
            p.setBrush(QColor(80, 180, 200, 160));
            p.setPen(QPen(QColor(100, 200, 220), 2));
            p.drawEllipse(screenX - size, screenY - size, size * 2, size * 2);
            // 旋转箭头提示
            p.setPen(QColor(200, 240, 255));
            p.setFont(QFont("Microsoft YaHei", 10));
            p.drawText(screenX - 8, screenY + 5, "〜");
        }
    }
}

// ============================================================
// 海浪（用C同学的WaveSystem显示提示）
// ============================================================

void GameWindow::drawWaves(QPainter& p)
{
    // 海浪警告提示
    if (WaveSystem::instance().isWarningActive()) {
        static int blink = 0; blink++;
        if ((blink / 15) % 2 == 0) {
            p.fillRect(0, 680, 1280, 40, QColor(255, 150, 0, 180));
            p.setPen(Qt::white);
            p.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
            QString dir = WaveSystem::instance().currentDirection() == WaveDirection::RIGHT
                ? "→ 顺风海浪来了！" : "← 逆风海浪来了！";
            p.drawText(0, 680, 1280, 40, Qt::AlignCenter, dir);
        }
    }
}

// ============================================================
// 敌人
// ============================================================

void GameWindow::drawBossHazards(QPainter& p)
{
    if (!gm->boss || !gm->boss->alive) return;

    for (const auto& h : gm->boss->getHazards()) {
        if (!h.active) continue;

        QColor fill(255, 80, 80, 80);
        QColor stroke(255, 100, 100, 180);

        switch (h.type) {
        case BossHazardType::BombWarning:
        case BossHazardType::ElegyWarning:
        case BossHazardType::CloneExplosionWarning:
            fill = QColor(255, 220, 60, 70);
            stroke = QColor(255, 220, 60, 190);
            break;
        case BossHazardType::BombHitbox:
        case BossHazardType::MeleeHitbox:
        case BossHazardType::MouthStrike:
        case BossHazardType::ReefHitbox:
            fill = QColor(255, 70, 70, 95);
            stroke = QColor(255, 70, 70, 210);
            break;
        case BossHazardType::EyeSector:
            fill = QColor(120, 170, 255, 55);
            stroke = QColor(120, 170, 255, 160);
            break;
        case BossHazardType::SoulSong:
            fill = QColor(190, 90, 255, 75);
            stroke = QColor(210, 140, 255, 200);
            break;
        case BossHazardType::SeaweedZone:
            fill = QColor(40, 180, 100, 70);
            stroke = QColor(40, 220, 130, 180);
            break;
        }

        p.setBrush(fill);
        p.setPen(QPen(stroke, 2));

        if (h.radius > 0.0) {
            int sx = int(h.position.x()) - gm->cameraX;
            int sy = int(h.position.y());
            int r = int(h.radius);
            p.drawEllipse(sx - r, sy - r, r * 2, r * 2);
        } else {
            QRectF rect = h.rect;
            rect.translate(-gm->cameraX, 0);
            p.drawRect(rect);
        }
    }
}

void GameWindow::drawSharks(QPainter& p)
{
    // 普通鲨鱼
    for (auto s : gm->sharks) {
        if (!s->alive) continue;
        int screenX = s->x - gm->cameraX;
        if (screenX < -50 || screenX > 1330) continue;

        if (!imgShark.isNull()) {
            p.drawPixmap(screenX - 24, s->y - 12, 48, 24, imgShark);
        }
        else {
            p.setBrush(QColor(80, 80, 200));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - 20, s->y - 12, 40, 24);
        }
        p.fillRect(screenX - 20, s->y - 22, 40, 6, QColor(60, 60, 60));
        int bw = (int)(40.0f * s->hp / s->maxHp);
        p.fillRect(screenX - 20, s->y - 22, bw, 6, QColor(220, 50, 50));
    }

    // 剑鱼
    for (auto s : gm->swordfishes) {
        if (!s->alive) continue;
        int screenX = s->x - gm->cameraX;
        if (screenX < -50 || screenX > 1330) continue;

        if (!imgSwordfish.isNull()) {
            p.drawPixmap(screenX - 28, s->y - 10, 56, 20, imgSwordfish);
        }
        else {
            p.setBrush(QColor(50, 200, 200));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - 20, s->y - 10, 40, 20);
        }
        if (s->state == Swordfish::WINDUP) {
            p.setPen(QColor(255, 200, 0));
            p.setFont(QFont("Microsoft YaHei", 10));
            p.drawText(screenX - 15, s->y - 18, "蓄力!");
        }
        p.fillRect(screenX - 20, s->y - 20, 40, 5, QColor(60, 60, 60));
        int bw2 = (int)(40.0f * s->hp / s->maxHp);
        p.fillRect(screenX - 20, s->y - 20, bw2, 5, QColor(220, 50, 50));
    }

    // 墨鱼
    for (auto o : gm->octopuses) {
        if (!o->alive || o->isInvisible) continue;
        int screenX = o->x - gm->cameraX;
        if (screenX < -50 || screenX > 1330) continue;

        if (!imgOctopus.isNull()) {
            p.drawPixmap(screenX - 20, o->y - 20, 40, 40, imgOctopus);
        }
        else {
            p.setBrush(QColor(150, 0, 150));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - 18, o->y - 18, 36, 36);
        }
        p.fillRect(screenX - 18, o->y - 26, 36, 5, QColor(60, 60, 60));
        int bw3 = (int)(36.0f * o->hp / o->maxHp);
        p.fillRect(screenX - 18, o->y - 26, bw3, 5, QColor(220, 50, 50));
    }

    // Boss
    if (gm->boss && gm->boss->alive) {
        int screenX = gm->boss->x - gm->cameraX;
        if (screenX >= -50 && screenX <= 1330) {
            if (!imgShark.isNull()) {
                p.drawPixmap(screenX - 40, gm->boss->y - 20, 80, 40, imgShark);
                if (gm->boss->state == Boss::PHASE2)
                    p.fillRect(screenX - 40, gm->boss->y - 20, 80, 40, QColor(255, 0, 0, 80));
            }
            else {
                p.setBrush(gm->boss->state == Boss::PHASE2 ? QColor(220, 0, 0) : QColor(160, 0, 0));
                p.setPen(Qt::NoPen);
                p.drawEllipse(screenX - 35, gm->boss->y - 20, 70, 40);
            }
            p.fillRect(screenX - 35, gm->boss->y - 32, 70, 8, QColor(60, 60, 60));
            int bw4 = (int)(70.0f * gm->boss->hp / gm->boss->maxHp);
            p.fillRect(screenX - 35, gm->boss->y - 32, bw4, 8, QColor(220, 50, 50));
            if (gm->boss->state == Boss::PHASE2) {
                p.setPen(QColor(255, 100, 100));
                p.setFont(QFont("Microsoft YaHei", 10));
                p.drawText(screenX - 20, gm->boss->y - 36, "狂暴！");
            }
        }
    }
}

// ============================================================
// 玩家
// ============================================================

void GameWindow::drawPlayer(QPainter& p)
{
    // 用C同学Player的worldPos()取屏幕坐标
    int screenX = gm->playerX() - gm->cameraX;
    int screenY = gm->playerY();

    if (!imgBoat.isNull()) {
        p.drawPixmap(screenX - 30, screenY - 15, 60, 30, imgBoat);
    }
    else {
        p.setBrush(QColor(240, 240, 240));
        p.setPen(QPen(QColor(100, 100, 100), 1));
        p.drawRect(screenX - 20, screenY - 10, 40, 20);
    }
}

// ============================================================
// 顶部信息栏
// ============================================================

void GameWindow::drawHUD(QPainter& p)
{
    Player& pl = Player::instance();
    p.fillRect(0, 0, 1280, 44, QColor(0, 0, 0, 170));

    p.setFont(QFont("Microsoft YaHei", 10));
    p.setPen(Qt::white);

    // 耐久条
    p.drawText(10, 28, "耐久");
    p.fillRect(50, 8, 100, 14, QColor(60, 60, 60));
    int durW = (int)(100.0f * pl.durability() / 100);
    p.fillRect(50, 8, durW, 14, QColor(80, 200, 80));

    // 体力条
    p.drawText(165, 28, "体力");
    p.fillRect(205, 8, 100, 14, QColor(60, 60, 60));
    int staW = (int)(100.0f * pl.stamina() / 100);
    p.fillRect(205, 8, staW, 14, QColor(200, 200, 50));

    // 文字信息
    p.drawText(315, 28, QString("金币:%1").arg(pl.coins));
    p.drawText(390, 28, QString("距离:%1m").arg(pl.distance));
    p.drawText(480, 28, QString("捕鱼:%1").arg(pl.fishCaught));
    p.drawText(560, 28, QString("击杀:%1").arg(gm->killCount));

    int sec = pl.gameSeconds;
    p.drawText(630, 28, QString("时间:%1:%2")
        .arg(sec / 60, 2, 10, QChar('0'))
        .arg(sec % 60, 2, 10, QChar('0')));

    // 武器信息
    if (gm->currentWeapon) {
        p.drawText(520, 28, QString("%1 耐久:%2/%3")
            .arg(QString::fromStdString(gm->currentWeapon->getName()))
            .arg(gm->currentWeapon->getCurrentDur())
            .arg(gm->currentWeapon->getMaxDur()));
    }

    // 天气状态
    switch (WeatherSystem::instance().currentWeather()) {
    case WeatherType::SUNNY:
        p.setPen(QColor(255, 220, 80));
        p.drawText(820, 28, "☀ 晴天");
        break;
    case WeatherType::FOG:
        p.setPen(QColor(200, 200, 200));
        p.drawText(820, 28, "🌫 大雾");
        break;
    case WeatherType::STORM:
        p.setPen(QColor(100, 150, 255));
        p.drawText(820, 28, "⛈ 暴风雨");
        break;
    }

    // 海浪速度
    qreal waveMulti = WaveSystem::instance().currentSpeedMultiplier();
    if (qAbs(waveMulti - 1.0) > 0.05) {
        p.setPen(waveMulti > 1.0 ? QColor(100, 255, 100) : QColor(255, 100, 100));
        p.drawText(920, 28, QString("海浪x%1").arg(waveMulti, 0, 'f', 1));
    }

    // 关卡进度条
    p.setPen(Qt::white);
    p.drawText(1100, 18, QString("关卡 %1/5").arg(gm->stage));
    p.fillRect(1100, 24, 160, 10, QColor(60, 60, 60));
    int prog = std::min(160, (int)(gm->playerX() * 160 / (gm->stage * 2000)));
    p.fillRect(1100, 24, prog, 10, QColor(100, 200, 100));
    p.setPen(QPen(Qt::white, 1));
    p.drawRect(1100, 24, 160, 10);

    p.setPen(QColor(180, 180, 180));
    p.setFont(QFont("Microsoft YaHei", 9));
    p.drawText(1100, 40, "空格:攻击 F:捕鱼 P:商店 ESC:暂停");
}

// ============================================================
// 捕鱼进度条
// ============================================================

void GameWindow::drawFishingHUD(QPainter& p)
{
    if (!isFishing || !targetFish) return;

    int barX = 490, barY = 55, barW = 300, barH = 22;
    p.fillRect(barX - 5, barY - 22, barW + 10, barH + 28, QColor(0, 0, 0, 180));

    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 11));
    QString fishName;
    switch (targetFish->type) {
    case Fish::SARDINE:        fishName = "沙丁鱼"; break;
    case Fish::TUNA:           fishName = "金枪鱼"; break;
    case Fish::DEEPSEAEEL:     fishName = "深海鳗"; break;
    case Fish::SWORDFISH_FISH: fishName = "金鱼";   break;
    }
    p.drawText(barX, barY - 4, QString("捕捉 %1 — 按F: %2/%3")
        .arg(fishName).arg(fishClickCount).arg(targetFish->catchRequired));

    p.fillRect(barX, barY, barW, barH, QColor(50, 50, 50));
    float ratio = 1.0f - (float)fishTimer / targetFish->catchTimeLimit;
    int fillW = (int)(barW * ratio);
    QColor barColor;
    switch (targetFish->type) {
    case Fish::SARDINE:        barColor = QColor(255, 220, 50);  break;
    case Fish::TUNA:           barColor = QColor(50, 180, 255);  break;
    case Fish::DEEPSEAEEL:     barColor = QColor(180, 50, 255);  break;
    case Fish::SWORDFISH_FISH: barColor = QColor(255, 180, 0);   break;
    }
    p.fillRect(barX, barY, fillW, barH, barColor);
    p.setPen(QPen(Qt::white, 1));
    p.drawRect(barX, barY, barW, barH);
}

// ============================================================
// 商店
// ============================================================

void GameWindow::openShop()
{
    ShopDialog dlg(this);
    dlg.exec();
}

// ============================================================
// 暂停画面
// ============================================================

void GameWindow::drawPaused(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(0, 0, 0, 120));
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 36, QFont::Bold));
    p.drawText(0, 280, 1280, 80, Qt::AlignCenter, "游戏暂停");
    p.setFont(QFont("Microsoft YaHei", 18));
    p.drawText(0, 380, 1280, 40, Qt::AlignCenter, "按 ESC 继续    按 Q 保存退出");
}

// ============================================================
// 失败画面
// ============================================================

void GameWindow::drawDefeat(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(80, 0, 0));
    p.setPen(QColor(255, 80, 80));
    p.setFont(QFont("Microsoft YaHei", 72, QFont::Bold));
    p.drawText(0, 200, 1280, 150, Qt::AlignCenter, "DEFEAT");

    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 18));
    p.drawText(0, 380, 1280, 40, Qt::AlignCenter,
        QString("航行距离: %1m   捕鱼: %2条   击杀: %3")
        .arg(Player::instance().distance)
        .arg(Player::instance().fishCaught)
        .arg(gm->killCount));

    p.setPen(QColor(255, 200, 80));
    p.setFont(QFont("Microsoft YaHei", 14));
    p.drawText(0, 500, 1280, 40, Qt::AlignCenter, "按 Space 重新开始");
}

// ============================================================
// 胜利画面
// ============================================================

void GameWindow::drawVictory(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(0, 60, 0));
    p.setPen(QColor(100, 255, 100));
    p.setFont(QFont("Microsoft YaHei", 72, QFont::Bold));
    p.drawText(0, 180, 1280, 150, Qt::AlignCenter, "VICTORY");

    int score = Player::instance().distance / 10 + Player::instance().coins + gm->killCount * 50;
    QString grade;
    if (score >= 800) grade = "S";
    else if (score >= 500) grade = "A";
    else if (score >= 300) grade = "B";
    else                   grade = "C";

    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 24));
    p.drawText(0, 360, 1280, 50, Qt::AlignCenter,
        QString("评级: %1   得分: %2").arg(grade).arg(score));

    p.setFont(QFont("Microsoft YaHei", 18));
    p.drawText(0, 420, 1280, 40, Qt::AlignCenter,
        QString("航行: %1m   捕鱼: %2条   击杀: %3   用时: %4:%5")
        .arg(Player::instance().distance)
        .arg(Player::instance().fishCaught)
        .arg(gm->killCount)
        .arg(Player::instance().gameSeconds / 60, 2, 10, QChar('0'))
        .arg(Player::instance().gameSeconds % 60, 2, 10, QChar('0')));

    p.setPen(QColor(255, 220, 80));
    p.setFont(QFont("Microsoft YaHei", 14));
    p.drawText(0, 520, 1280, 40, Qt::AlignCenter, "按 Space 返回主菜单");
}

// ============================================================
// 捕鱼逻辑
// ============================================================

void GameWindow::tryStartFishing()
{
    if (isFishing) return;
    int px = gm->playerX();
    int py = gm->playerY();
    for (auto f : gm->fish) {
        if (f->caught || f->escaped) continue;
        if (f->isNearPlayer(px, py, 120)) {
            targetFish = f;
            isFishing = true;
            fishClickCount = 0;
            fishTimer = 0;
            return;
        }
    }
}

void GameWindow::updateFishing()
{
    if (!isFishing || !targetFish) return;
    fishTimer++;

    if (fishTimer >= targetFish->catchTimeLimit) {
        targetFish->vx *= 3;
        targetFish->vy *= 3;
        targetFish->escaped = true;
        isFishing = false;
        targetFish = nullptr;
        return;
    }

    if (fishClickCount >= targetFish->catchRequired) {
        targetFish->caught = true;
        Player& pl = Player::instance();
        pl.coins += targetFish->value;
        pl.fishCaught++;
        pl.fishTotalValue += targetFish->value;

        // 完美捕获：时间剩余超过一半，体力消耗减半
        int cost = (fishTimer < targetFish->catchTimeLimit / 2)
            ? targetFish->staminaCost / 2
            : targetFish->staminaCost;
        // C同学没有直接设置stamina的接口，用applySpeedReduction模拟体力消耗影响
        isFishing = false;
        targetFish = nullptr;
    }
}

// ============================================================
// 键盘按下
// ============================================================

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    // 同时传给C同学的Player处理移动
    Player::instance().keyPress(event);

    if (state == STATE_INTRO) {
        if (event->key() == Qt::Key_Space) { state = STATE_MENU; update(); }
        return;
    }

    if (state == STATE_MENU) {
        if (event->key() == Qt::Key_N) {
            gm->fileManager.deleteSave();
            state = STATE_PLAYING;
        }
        else if (event->key() == Qt::Key_C && gm->fileManager.hasSave()) {
            gm->loadSave();
            state = STATE_PLAYING;
        }
        return;
    }

    if (state == STATE_DEFEAT) {
        if (event->key() == Qt::Key_Space) {
            delete gm;
            gm = new GameManager();
            state = STATE_MENU;
            update();
        }
        return;
    }

    if (state == STATE_VICTORY) {
        if (event->key() == Qt::Key_Space) {
            delete gm;
            gm = new GameManager();
            state = STATE_MENU;
            update();
        }
        return;
    }

    if (state == STATE_PAUSED) {
        if (event->key() == Qt::Key_Escape) state = STATE_PLAYING;
        else if (event->key() == Qt::Key_Q) { gm->saveAndQuit(); close(); }
        return;
    }

    if (state == STATE_PLAYING) {
        switch (event->key()) {
        case Qt::Key_F:
            if (!isFishing) tryStartFishing();
            else fishClickCount++;
            break;
        case Qt::Key_Space:
            gm->attackNearest(30, 150);
            break;
        case Qt::Key_P:
            timer->stop();
            openShop();
            timer->start(16);
            break;
        case Qt::Key_Escape: state = STATE_PAUSED; break;
        case Qt::Key_Q: gm->saveAndQuit(); close(); break;
        default: break;
        }
    }
}

// ============================================================
// 键盘松开
// ============================================================

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    // 传给C同学的Player处理
    Player::instance().keyRelease(event);
}
