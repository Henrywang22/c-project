#include "GameWindow.h"
#include "QPainter"
#include "QFont"
#include "QKeyEvent"

//======析构构造=====

GameWindow::GameWindow(QWidget* parent) :QWidget(parent)
{
	setWindowTitle("渔途");
	setFixedSize(1280, 720);//固定窗口大小，不允许拉伸
	
	gm = new GameManager();//创建游戏逻辑管理器

	timer = new QTimer(this);//信号槽：每当timer超时，，自动调用gameLoop
	connect(timer, &QTimer::timeout, this, &GameWindow::gameLoop);
	timer->start(16); // 16ms ≈ 60帧/秒
}

GameWindow::~GameWindow()
{
	delete gm; // 释放GameManager内存
}

// ============ Qt事件：每帧绘制 ============

void GameWindow::paintEvent(QPaintEvent*)
{
	QPainter p(this);// 根据当前游戏状态决定画哪个画面
	switch (state) 
	{
	case STATE_INTRO:   drawIntro(p);   break; // 开场说明
	case STATE_MENU:    drawMenu(p);    break; // 主菜单
	case STATE_PLAYING:
	case STATE_PAUSED:  drawGame(p);    break; // 游戏中/暂停都画游戏画面
	case STATE_DEFEAT:  drawDefeat(p);  break; // 失败
	case STATE_VICTORY: drawVictory(p); break; // 胜利
	default: break;
	}
}

// ============ 游戏主循环（每16ms执行一次）============
void GameWindow::gameLoop()
{
    switch (state) {
    case STATE_PLAYING:
        // 检查失败/胜利
        if (gm->gameOver) { state = STATE_DEFEAT;  update(); return; }
        if (gm->victory) { state = STATE_VICTORY; update(); return; }

        // 关卡通关处理
        if (gm->stageClear) {
            timer->stop(); // 暂停游戏循环

            // 保存当前进度
            gm->fileManager.saveGame({
                gm->stage,
                gm->player->distance,
                gm->player->coins,
                gm->player->durability,
                gm->player->stamina,
                gm->player->fishCaught,
                gm->player->fishTotalValue,
                gm->player->gameSeconds,
                false
                });

            openShop(); // 打开商店

            // 清理当前关卡的障碍物和鲨鱼
            for (auto o : gm->obstacles) delete o;
            gm->obstacles.clear();
            for (auto s : gm->sharks) delete s;
            gm->sharks.clear();

            // 进入下一关
            gm->stage++;
            gm->bossSpawned = false;
            gm->stageClear = false;
            isFishing = false;
            targetFish = nullptr;
            gm->spawnObstacles(); // 生成新障碍

            timer->start(16); // 恢复游戏循环
            return;
        }

        // 处理移动输入（同时按多键可以斜向移动）
        if (keyUp)    gm->player->move(0, -1);
        if (keyDown)  gm->player->move(0, 1);
        if (keyLeft)  gm->player->move(-1, 0);
        if (keyRight) gm->player->move(1, 0);

        // 加速/停止加速
        if (keyShift) gm->player->boost();
        else          gm->player->stopBoost();

        updateFishing();  // 更新捕鱼状态
        gm->update();     // 更新所有游戏逻辑
        break;

    default: break;
    }
    update(); // 触发paintEvent重新绘制
}

// ============ 背景海洋 ============
void GameWindow::drawSea(QPainter& p)
{
    // 深蓝色海洋背景
    p.fillRect(0, 0, 1280, 720, QColor(30, 100, 180));
    // 用横线模拟水面波纹
    p.setPen(QPen(QColor(50, 130, 210), 1));
    for (int y = 80; y < 720; y += 60)
        for (int x = 0; x < 1280; x += 80)
            p.drawLine(x, y, x + 40, y);
}

// ============ 开场说明画面 ============
void GameWindow::drawIntro(QPainter& p)
{
    // 深色海洋背景
    p.fillRect(0, 0, 1280, 720, QColor(20, 70, 140));
    p.setPen(QPen(QColor(50, 130, 210), 1));
    for (int y = 80; y < 720; y += 60)
        for (int x = 0; x < 1280; x += 80)
            p.drawLine(x, y, x + 40, y);

    // 标题
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 42, QFont::Bold));
    p.drawText(0, 60, 1280, 100, Qt::AlignCenter, "渔  途");

    // 副标题
    p.setFont(QFont("Microsoft YaHei", 14));
    p.setPen(QColor(180, 220, 255));
    p.drawText(0, 150, 1280, 30, Qt::AlignCenter, "—— 一场向右的海上冒险 ——");

    // 分隔线
    p.setPen(QPen(QColor(100, 160, 220), 1));
    p.drawLine(200, 195, 1080, 195);

    // 游戏说明文字列表
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
        "             普通鲨鱼（蓝色）    Boss ）",
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
        if (line.isEmpty()) { startY += 6; continue; } // 空行少跳一点
        p.drawText(160, startY, line);
        startY += 21;
    }

    // 底部分隔线
    p.setPen(QPen(QColor(100, 160, 220), 1));
    p.drawLine(200, 678, 1080, 678);

    // 闪烁提示（每30帧切换一次显示/隐藏）
    static int blink = 0;
    blink++;
    if ((blink / 30) % 2 == 0) {
        p.setPen(QColor(255, 220, 80));
        p.setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
        p.drawText(0, 688, 1280, 30, Qt::AlignCenter, "按空格键开始游戏");
    }
}
// ============ 主菜单 ============

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

    // 只有存在存档时才显示继续选项
    if (gm->fileManager.hasSave()) {
        p.setPen(QColor(100, 220, 255));
        p.drawText(0, 420, 1280, 50, Qt::AlignCenter, "按 C 继续上一关");
    }
}

// ============ 游戏画面总入口 ============

void GameWindow::drawGame(QPainter& p)
{
    drawSea(p);        // 1. 先画背景
    drawObstacles(p);  // 2. 障碍物
    drawWaves(p);      // 3. 海浪
    drawFish(p);       // 4. 鱼
    drawSharks(p);     // 5. 敌人
    drawPlayer(p);     // 6. 玩家（画在最上层）
    drawHUD(p);        // 7. 顶部信息栏
    drawFishingHUD(p); // 8. 捕鱼进度条

    // 9. 天气效果（队友实现）
    gm->weather.draw(p, gm->cameraX);

    // 如果是暂停状态，在游戏画面上叠加暂停遮罩
    if (state == STATE_PAUSED) drawPaused(p);
}

// ============ 以下函数依赖队友的类，先留空占位 ============

void GameWindow::drawFish(QPainter& p)
{
    // TODO: 等队友Fish类完成后实现
    // 遍历gm->fish，根据鱼的类型画不同颜色的椭圆
}

void GameWindow::drawObstacles(QPainter& p)
{
    // TODO: 等队友Obstacle类完成后实现
    // 暗礁画棕色方块，漩涡画半透明圆
}

void GameWindow::drawWaves(QPainter& p)
{
    // TODO: 等队友WaveSystem完成后实现
}

void GameWindow::drawSharks(QPainter& p)
{
    // TODO: 等队友NormalEnemy/Boss类完成后实现
}

void GameWindow::drawPlayer(QPainter& p)
{
    // TODO: 等队友Player类完成后实现
    // 玩家是一个白色矩形，加速时变黄色
}

void GameWindow::drawHUD(QPainter& p)
{
    // TODO: 等队友Player类完成后实现
    // 显示耐久、体力、金币、距离、时间、击杀数等
}

void GameWindow::drawFishingHUD(QPainter& p)
{
    // TODO: 等队友Fish类完成后实现
    // 显示捕鱼进度条和点击次数
}

void GameWindow::openShop()
{
    // TODO: 等队友ShopDialog类完成后实现
}

// ============ 暂停画面 ============

void GameWindow::drawPaused(QPainter& p)
{
    // 半透明黑色遮罩覆盖在游戏画面上
    p.fillRect(0, 0, 1280, 720, QColor(0, 0, 0, 120));
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 36, QFont::Bold));
    p.drawText(0, 280, 1280, 80, Qt::AlignCenter, "游戏暂停");
    p.setFont(QFont("Microsoft YaHei", 18));
    p.drawText(0, 380, 1280, 40, Qt::AlignCenter, "按 ESC 继续    按 Q 保存退出");
}

// ============ 失败画面 ============

void GameWindow::drawDefeat(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(80, 0, 0)); // 暗红色背景
    p.setPen(QColor(255, 80, 80));
    p.setFont(QFont("Microsoft YaHei", 72, QFont::Bold));
    p.drawText(0, 200, 1280, 150, Qt::AlignCenter, "DEFEAT");

    // 显示本局统计
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 18));
    p.drawText(0, 380, 1280, 40, Qt::AlignCenter,
        QString("航行距离: %1m   捕鱼: %2条   击杀: %3")
        .arg(gm->player->distance)
        .arg(gm->player->fishCaught)
        .arg(gm->killCount));

    p.setPen(QColor(255, 200, 80));
    p.setFont(QFont("Microsoft YaHei", 14));
    p.drawText(0, 500, 1280, 40, Qt::AlignCenter, "按 Space 重新开始");
}

// ============ 胜利画面 ============

void GameWindow::drawVictory(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(0, 60, 0)); // 深绿色背景
    p.setPen(QColor(100, 255, 100));
    p.setFont(QFont("Microsoft YaHei", 72, QFont::Bold));
    p.drawText(0, 180, 1280, 150, Qt::AlignCenter, "VICTORY");

    // 计算评分和评级
    int score = gm->player->distance / 10 + gm->player->coins + gm->killCount * 50;
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
        .arg(gm->player->distance)
        .arg(gm->player->fishCaught)
        .arg(gm->killCount)
        .arg(gm->player->gameSeconds / 60, 2, 10, QChar('0'))
        .arg(gm->player->gameSeconds % 60, 2, 10, QChar('0')));

    p.setPen(QColor(255, 220, 80));
    p.setFont(QFont("Microsoft YaHei", 14));
    p.drawText(0, 520, 1280, 40, Qt::AlignCenter, "按 Space 返回主菜单");
}

// ============ 捕鱼逻辑 ============

void GameWindow::tryStartFishing()
{
    if (isFishing) return; // 已经在捕鱼就不重复开始
    // 遍历所有鱼，找到靠近玩家的鱼
    for (auto f : gm->fish) {
        if (f->caught || f->escaped) continue;
        if (f->isNearPlayer(gm->player->x, gm->player->y, 120)) {
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
    fishTimer++; // 每帧计时

    // 超时：鱼逃跑
    if (fishTimer >= targetFish->catchTimeLimit) {
        targetFish->vx *= 3; // 鱼加速逃走
        targetFish->vy *= 3;
        targetFish->escaped = true;
        isFishing = false;
        targetFish = nullptr;
        return;
    }

    // 点击次数足够：捕获成功
    if (fishClickCount >= targetFish->catchRequired) {
        targetFish->caught = true;
        gm->player->coins += targetFish->value;       // 获得金币
        gm->player->stamina = std::min(gm->player->maxStamina,
            gm->player->stamina + targetFish->staminaGain); // 恢复体力
        gm->player->fishCaught++;
        gm->player->fishTotalValue += targetFish->value;
        isFishing = false;
        targetFish = nullptr;
    }
}

// ============ 键盘按下 ============

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    // 根据当前状态处理不同的按键
    if (state == STATE_INTRO) {
        if (event->key() == Qt::Key_Space) {
            state = STATE_MENU;
            update();
        }
        return;
    }

    if (state == STATE_MENU) {
        if (event->key() == Qt::Key_N) {
            gm->fileManager.deleteSave(); // 删除存档，全新开始
            state = STATE_PLAYING;
        }
        else if (event->key() == Qt::Key_C && gm->fileManager.hasSave()) {
            gm->loadSave(); // 读取存档
            state = STATE_PLAYING;
        }
        return;
    }

    if (state == STATE_DEFEAT) {
        if (event->key() == Qt::Key_Space) {
            delete gm;
            gm = new GameManager(); // 重置游戏
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
        else if (event->key() == Qt::Key_Q) {
            gm->saveAndQuit();
            close();
        }
        return;
    }

    if (state == STATE_PLAYING) {
        switch (event->key()) {
        case Qt::Key_W: case Qt::Key_Up:    keyUp = true; break;
        case Qt::Key_S: case Qt::Key_Down:  keyDown = true; break;
        case Qt::Key_A: case Qt::Key_Left:  keyLeft = true; break;
        case Qt::Key_D: case Qt::Key_Right: keyRight = true; break;
        case Qt::Key_Shift: keyShift = true; gm->player->boost(); break;
        case Qt::Key_F:
            if (!isFishing) tryStartFishing();
            else fishClickCount++; // 捕鱼中继续点击
            break;
        case Qt::Key_Space:
            gm->attackNearest(30, 150); // 攻击范围150内最近的敌人
            break;
        case Qt::Key_P:
            timer->stop();   // 暂停游戏循环
            openShop();
            timer->start(16);// 恢复游戏循环
            break;
        case Qt::Key_Escape: state = STATE_PAUSED; break;
        case Qt::Key_Q: gm->saveAndQuit(); close(); break;
        }
    }
}

// ============ 键盘松开 ============

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_W: case Qt::Key_Up:    keyUp = false; break;
    case Qt::Key_S: case Qt::Key_Down:  keyDown = false; break;
    case Qt::Key_A: case Qt::Key_Left:  keyLeft = false; break;
    case Qt::Key_D: case Qt::Key_Right: keyRight = false; break;
    case Qt::Key_Shift: keyShift = false; gm->player->stopBoost(); break;
    }
}