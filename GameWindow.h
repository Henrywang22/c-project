#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "GameManager.h"

enum GameState {
    STATE_INTRO,    // 介绍/说明画面
    STATE_MENU,     // 主菜单
    STATE_PLAYING,  // 游戏中
    STATE_PAUSED,   // 暂停
    STATE_DEFEAT,   // 失败
    STATE_VICTORY   // 胜利
};

// GameWindow 继承 QWidget，是整个游戏的窗口
class GameWindow : public QWidget {
    Q_OBJECT  // Qt必须的宏，支持信号槽

public:
    GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

protected:
    // Qt自动调用的事件函数，不需要手动调用
    void paintEvent(QPaintEvent* event) override;   // 每帧绘制
    void keyPressEvent(QKeyEvent* event) override;  // 按键按下
    void keyReleaseEvent(QKeyEvent* event) override;// 按键松开


private slots:
    void gameLoop(); // 每16ms被timer触发一次;这是Qt槽函数
private:
    QTimer* timer; // 游戏循环计时器
    GameManager* gm;    // 游戏逻辑管理器
    GameState    state = STATE_INTRO; // 当前游戏状态

    // 按键状态
    bool keyUp = false;
    bool keyDown = false;
    bool keyLeft = false;
    bool keyRight = false;
    bool keyShift = false;

    // 捕鱼状态
    Fish* targetFish = nullptr;
    bool  isFishing = false;
    int   fishClickCount = 0;
    int   fishTimer = 0;

    // 各画面的绘制函数
    void drawIntro(QPainter& p);      // 开场说明
    void drawMenu(QPainter& p);       // 主菜单
    void drawGame(QPainter& p);       // 游戏画面总入口
    void drawSea(QPainter& p);        // 背景海洋
    void drawFish(QPainter& p);       // 鱼
    void drawObstacles(QPainter& p);  // 障碍物
    void drawSharks(QPainter& p);     // 敌人
    void drawPlayer(QPainter& p);     // 玩家船
    void drawWaves(QPainter& p);      // 海浪
    void drawHUD(QPainter& p);        // 顶部信息栏
    void drawFishingHUD(QPainter& p); // 捕鱼进度条
    void drawPaused(QPainter& p);     // 暂停画面
    void drawDefeat(QPainter& p);     // 失败画面
    void drawVictory(QPainter& p);    // 胜利画面

    void tryStartFishing(); // 尝试开始捕鱼
    void updateFishing();   // 更新捕鱼状态
    void openShop();        // 打开商店
};