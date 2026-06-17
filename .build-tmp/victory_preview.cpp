#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QString>

#include "GameConfig.h"
#include "GameManager.h"
#include "Player.h"

#define private public
#include "GameWindow.h"
#undef private

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    const QString outputPath = argc > 1
        ? QString::fromLocal8Bit(argv[1])
        : QStringLiteral("build-preview/final_settlement_preview.png");
    const QString runDir = argc > 2
        ? QString::fromLocal8Bit(argv[2])
        : QStringLiteral(".build-tmp/preview-run");

    QDir().mkpath(runDir);
    QDir::setCurrent(runDir);

    Player& player = Player::instance();
    player.reset();
    player.restoreSavedProgress(
        Config::GameConfig::stageConfig(Config::GameConfig::STAGE_COUNT).targetDistance,
        112,
        84,
        120,
        110,
        Config::GameConfig::SHIP_BASE_SPEED + 2.0
    );
    player.coins = 6420;
    player.fishCaught = 68;
    player.fishTotalValue = 10840;
    player.gameSeconds = 18 * 60 + 42;

    GameWindow window;
    window.timer->stop();
    window.state = STATE_VICTORY;
    window.victoryScoreSaved = true;
    window.victoryButtonHover = -1;

    window.gm->stage = Config::GameConfig::STAGE_COUNT;
    window.gm->victory = true;
    window.gm->killCount = 43;
    window.gm->fileManager.markBossDiscovered(0, "Five Head Shark");
    window.gm->fileManager.markBossDiscovered(2, "Siren");
    for (int i = 0; i < 12; ++i) {
        window.gm->fileManager.markFishDiscovered(i, "Fish");
    }
    for (int i = 0; i < 5; ++i) {
        window.gm->fileManager.markEnemyDiscovered(i, "Enemy");
    }

    QImage image(1280, 720, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    window.render(&painter);
    painter.end();

    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    return image.save(outputPath) ? 0 : 2;
}
