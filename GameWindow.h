#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "GameManager.h"

enum GameState
{
    STATE_INTRO,    // 介绍/说明画面
    STATE_MENU,     // 主菜单
    STATE_PLAYING,  // 游戏中
    STATE_PAUSED,   // 暂停
    STATE_DEFEAT,   // 失败
    STATE_VICTORY   // 胜利
};

class GameWindow : public QWidget {
    Q_OBJECT

public:
    GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void gameLoop();

private:
    QTimer* timer;
    GameManager* gm;
    GameState    state = STATE_INTRO;

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

    // 绘制函数
    void drawIntro(QPainter& p);
    void drawMenu(QPainter& p);
    void drawGame(QPainter& p);
    void drawSea(QPainter& p);
    void drawFish(QPainter& p);
    void drawObstacles(QPainter& p);
    void drawSharks(QPainter& p);
    void drawPlayer(QPainter& p);
    void drawWaves(QPainter& p);
    void drawHUD(QPainter& p);
    void drawFishingHUD(QPainter& p);
    void drawPaused(QPainter& p);
    void drawDefeat(QPainter& p);
    void drawVictory(QPainter& p);

    void tryStartFishing();
    void updateFishing();
    void openShop();

    // 图片资源
    QPixmap imgSardine;
    QPixmap imgTuna;
    QPixmap imgEel;
    QPixmap imgGolden;
    QPixmap imgShark;
    QPixmap imgSwordfish;
    QPixmap imgOctopus;
    QPixmap imgBoat;
};