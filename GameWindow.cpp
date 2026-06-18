#include "GameWindow.h"
#include "Shopdialog.h"
#include "BackpackDialog.h"
#include "EncyclopediaDialog.h"
#include "GameUiDialog.h"
#include "Obstacle.h"
#include "InventorySystem.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QKeyEvent>
#include <QDateTime>
#include <QLineF>
#include <QVector2D>
#include <QPolygon>
#include <QLinearGradient>
#include <QRandomGenerator>
#include <QStringList>
#include <algorithm>
#include <cmath>

namespace {

QString coinDisplayText(const Player& player)
{
    return player.testModeInfiniteCoins
        ? QStringLiteral("\u221e")
        : QString::number(player.coins);
}

QString bossDisplayName(BossKind kind)
{
    switch (kind) {
    case BossKind::FiveHeadShark:
        return QStringLiteral("\u593a\u547d\u4e94\u5934\u9ca8");
    case BossKind::TaliMonster:
        return QStringLiteral("\u5854\u91cc\u6d77\u602a");
    case BossKind::Siren:
        return QStringLiteral("\u585e\u58ec\u5973\u5996");
    }
    return QStringLiteral("Boss");
}

QString stageName(int stage)
{
    return QString::fromUtf8(Config::GameConfig::stageText(stage).name);
}

QString stageBrief(int stage)
{
    return QString::fromUtf8(Config::GameConfig::stageText(stage).brief);
}

QString stageClearSummary(int stage)
{
    return QString::fromUtf8(Config::GameConfig::stageText(stage).clearSummary);
}

QString formatGameTime(int seconds)
{
    seconds = qMax(0, seconds);
    const int minutes = seconds / 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}

QString victoryGrade(int score)
{
    if (score >= 8500) return QStringLiteral("S");
    if (score >= 7200) return QStringLiteral("A");
    if (score >= 5800) return QStringLiteral("B");
    if (score >= 4300) return QStringLiteral("C");
    return QStringLiteral("D");
}

QString victoryGradeTitle(const QString& grade)
{
    if (grade == QStringLiteral("S")) return QStringLiteral("传奇渔夫");
    if (grade == QStringLiteral("A")) return QStringLiteral("远海猎手");
    if (grade == QStringLiteral("B")) return QStringLiteral("老练船长");
    if (grade == QStringLiteral("C")) return QStringLiteral("合格归航");
    return QStringLiteral("勉强归航");
}

QString victoryComment(const QString& grade)
{
    if (grade == QStringLiteral("S")) return QStringLiteral("风暴和深海都记住了你的船名。");
    if (grade == QStringLiteral("A")) return QStringLiteral("这趟远航收获丰厚，船队会传颂你的航线。");
    if (grade == QStringLiteral("B")) return QStringLiteral("稳稳归港，战利品和图鉴都有扎实进展。");
    if (grade == QStringLiteral("C")) return QStringLiteral("成功完成远航，但仍有不少收益可以挖掘。");
    return QStringLiteral("船还在，航线也还在，下次会更漂亮。");
}

int stageLengthMeters(int stage)
{
    const int start = Config::GameConfig::stageStartDistance(stage);
    const int end = Config::GameConfig::stageConfig(stage).targetDistance;
    return qMax(1, end - start);
}

QString weatherLabel(const Config::GameConfig::StageConfig& cfg)
{
    if (cfg.stormWeight > 0 && cfg.stormWeight >= cfg.sunnyWeight && cfg.stormWeight >= cfg.fogWeight) {
        return QStringLiteral("暴风雨高发");
    }
    if (cfg.fogWeight > 0 && cfg.stormWeight > 0) {
        return QStringLiteral("天气多变");
    }
    if (cfg.fogWeight > 0) {
        return QStringLiteral("薄雾海面");
    }
    if (cfg.stormWeight > 0) {
        return QStringLiteral("偶有雷雨");
    }
    return QStringLiteral("晴朗海面");
}

QString waveLabel(const Config::GameConfig::StageConfig& cfg)
{
    if (cfg.waveChancePerFrame >= 2000) {
        return cfg.waveLeftWeight > cfg.waveRightWeight
            ? QStringLiteral("轻微逆浪")
            : QStringLiteral("顺向小浪");
    }
    if (cfg.waveLeftWeight > cfg.waveRightWeight) {
        return QStringLiteral("逆浪较多");
    }
    if (cfg.waveRightWeight > cfg.waveLeftWeight) {
        return QStringLiteral("顺浪频繁");
    }
    return QStringLiteral("浪流交替");
}

QString stageStartObjective(int stage)
{
    const auto& cfg = Config::GameConfig::stageConfig(stage);
    const QString goal = QStringLiteral("目标：航行 %1m").arg(stageLengthMeters(stage));
    return cfg.hasBoss ? goal + QStringLiteral(" 并击败 Boss") : goal;
}

QString stageStartSeaLine(int stage)
{
    const auto& cfg = Config::GameConfig::stageConfig(stage);
    return QStringLiteral("%1  ·  %2  ·  %3")
        .arg(weatherLabel(cfg),
             waveLabel(cfg),
             cfg.hasBoss ? QStringLiteral("Boss 出没") : QStringLiteral("无 Boss"));
}

QString nextStageLine(int stage)
{
    if (stage >= Config::GameConfig::STAGE_COUNT) {
        return QStringLiteral("最终海域已清理，准备进入通关结算。");
    }

    const int nextStage = stage + 1;
    const auto& cfg = Config::GameConfig::stageConfig(nextStage);
    QStringList notes;
    if (cfg.swordfishCap > 0) notes << QStringLiteral("剑鱼");
    if (cfg.octopusCap > 0) notes << QStringLiteral("章鱼");
    if (cfg.hasBoss) notes << QStringLiteral("Boss");
    if (cfg.whirlpoolCount > 0) notes << QStringLiteral("漩涡");
    if (cfg.stormWeight > 0 || cfg.fogWeight > 0) notes << QStringLiteral("复杂天气");

    const QString danger = notes.isEmpty()
        ? QStringLiteral("海况仍较平稳")
        : QStringLiteral("将出现%1").arg(notes.join(QStringLiteral("、")));
    return QStringLiteral("下一关：%1。%2，建议补给后再出航。")
        .arg(stageName(nextStage), danger);
}

QString fishDisplayName(Fish::Type type)
{
    switch (type) {
    case Fish::SARDINE:        return QStringLiteral("\u6c99\u4e01\u9c7c");
    case Fish::TUNA:           return QStringLiteral("\u91d1\u67aa\u9c7c");
    case Fish::DEEPSEAEEL:     return QStringLiteral("\u6df1\u6d77\u9cd7");
    case Fish::SWORDFISH_FISH: return QStringLiteral("\u91d1\u9c7c");
    case Fish::ANCHOVY:        return QStringLiteral("\u94f6\u9cca\u9c7c");
    case Fish::CLOWNFISH:      return QStringLiteral("\u5c0f\u4e11\u9c7c");
    case Fish::MACKEREL:       return QStringLiteral("\u84dd\u9cb5");
    case Fish::SEA_BREAM:      return QStringLiteral("\u771f\u9cb7");
    case Fish::LANTERNFISH:    return QStringLiteral("\u706f\u7b3c\u9c7c");
    case Fish::GROUPER:        return QStringLiteral("\u77f3\u6591\u9c7c");
    case Fish::KOI:            return QStringLiteral("\u9526\u9ca4");
    case Fish::CRYSTAL_FISH:   return QStringLiteral("\u6676\u9cde\u9c7c");
    }
    return QStringLiteral("\u672a\u77e5\u9c7c");
}

const char* fishDiscoveryName(Fish::Type type)
{
    switch (type) {
    case Fish::SARDINE:        return u8"\u6c99\u4e01\u9c7c";
    case Fish::TUNA:           return u8"\u91d1\u67aa\u9c7c";
    case Fish::DEEPSEAEEL:     return u8"\u6df1\u6d77\u9cd7";
    case Fish::SWORDFISH_FISH: return u8"\u91d1\u9c7c";
    case Fish::ANCHOVY:        return u8"\u94f6\u9cca\u9c7c";
    case Fish::CLOWNFISH:      return u8"\u5c0f\u4e11\u9c7c";
    case Fish::MACKEREL:       return u8"\u84dd\u9cb5";
    case Fish::SEA_BREAM:      return u8"\u771f\u9cb7";
    case Fish::LANTERNFISH:    return u8"\u706f\u7b3c\u9c7c";
    case Fish::GROUPER:        return u8"\u77f3\u6591\u9c7c";
    case Fish::KOI:            return u8"\u9526\u9ca4";
    case Fish::CRYSTAL_FISH:   return u8"\u6676\u9cde\u9c7c";
    }
    return "Unknown Fish";
}

int fishDiscoveryId(Fish::Type type)
{
    switch (type) {
    case Fish::SARDINE:        return 0;
    case Fish::TUNA:           return 1;
    case Fish::DEEPSEAEEL:     return 2;
    case Fish::SWORDFISH_FISH: return 3;
    case Fish::ANCHOVY:        return 4;
    case Fish::CLOWNFISH:      return 5;
    case Fish::MACKEREL:       return 6;
    case Fish::SEA_BREAM:      return 7;
    case Fish::LANTERNFISH:    return 8;
    case Fish::GROUPER:        return 9;
    case Fish::KOI:            return 10;
    case Fish::CRYSTAL_FISH:   return 11;
    }
    return 0;
}

QColor fishAccentColor(Fish::Type type)
{
    switch (type) {
    case Fish::SARDINE:
    case Fish::ANCHOVY:
        return QColor(255, 220, 68);
    case Fish::TUNA:
    case Fish::MACKEREL:
        return QColor(72, 190, 255);
    case Fish::CLOWNFISH:
    case Fish::SEA_BREAM:
        return QColor(255, 158, 82);
    case Fish::DEEPSEAEEL:
    case Fish::LANTERNFISH:
        return QColor(188, 86, 255);
    case Fish::GROUPER:
        return QColor(176, 122, 58);
    case Fish::SWORDFISH_FISH:
    case Fish::KOI:
        return QColor(255, 176, 42);
    case Fish::CRYSTAL_FISH:
        return QColor(80, 240, 255);
    }
    return QColor(255, 220, 68);
}

QSizeF fishDrawSize(Fish::Type type)
{
    switch (type) {
    case Fish::SARDINE:        return QSizeF(68, 36);
    case Fish::TUNA:           return QSizeF(78, 40);
    case Fish::DEEPSEAEEL:     return QSizeF(78, 34);
    case Fish::SWORDFISH_FISH: return QSizeF(72, 38);
    case Fish::ANCHOVY:        return QSizeF(76, 34);
    case Fish::CLOWNFISH:      return QSizeF(64, 42);
    case Fish::MACKEREL:       return QSizeF(88, 42);
    case Fish::SEA_BREAM:      return QSizeF(72, 44);
    case Fish::LANTERNFISH:    return QSizeF(68, 40);
    case Fish::GROUPER:        return QSizeF(86, 48);
    case Fish::KOI:            return QSizeF(84, 44);
    case Fish::CRYSTAL_FISH:   return QSizeF(90, 46);
    }
    return QSizeF(68, 36);
}

QFont promptFont(int pixelSize, bool bold)
{
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPixelSize(pixelSize);
    font.setWeight(bold ? QFont::Bold : QFont::Normal);
    return font;
}

void drawPromptText(QPainter& p, const QRect& rect, const QString& text, int pixelSize,
                    const QColor& color, bool bold = false,
                    Qt::Alignment flags = Qt::AlignCenter,
                    const QColor& shadow = QColor(45, 22, 10, 140))
{
    QFont font = promptFont(pixelSize, bold);
    while (pixelSize > 16) {
        QFontMetrics metrics(font);
        if (metrics.horizontalAdvance(text) <= rect.width() && metrics.height() <= rect.height()) {
            break;
        }
        font.setPixelSize(--pixelSize);
    }

    p.setFont(font);
    p.setPen(shadow);
    p.drawText(rect.translated(2, 2), flags, text);
    p.setPen(color);
    p.drawText(rect, flags, text);
}

void drawPromptBackground(QPainter& p, const QPixmap& pixmap)
{
    if (!pixmap.isNull()) {
        p.drawPixmap(QRect(0, 0, 1280, 720), pixmap, pixmap.rect());
        return;
    }
    p.fillRect(0, 0, 1280, 720, QColor(18, 85, 145));
}

QRect promptPopupRect()
{
    return QRect(150, 38, 980, 655);
}

QPointF clampedProjectileEnd(const QPointF& origin, const QPointF& target, qreal range)
{
    const qreal dx = target.x() - origin.x();
    const qreal dy = target.y() - origin.y();
    const qreal length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.001) {
        return QPointF(origin.x() + range, origin.y());
    }
    if (length <= range) {
        return target;
    }

    return QPointF(origin.x() + dx / length * range,
                   origin.y() + dy / length * range);
}

QPointF firstLineRectIntersection(const QLineF& line, const QRectF& rect, bool& hit)
{
    hit = false;
    if (rect.isEmpty()) {
        return line.p2();
    }
    if (rect.contains(line.p1())) {
        hit = true;
        return line.p1();
    }
    if (rect.contains(line.p2())) {
        hit = true;
        return line.p2();
    }

    const QLineF edges[] = {
        QLineF(rect.topLeft(), rect.topRight()),
        QLineF(rect.topRight(), rect.bottomRight()),
        QLineF(rect.bottomRight(), rect.bottomLeft()),
        QLineF(rect.bottomLeft(), rect.topLeft())
    };

    QPointF best = line.p2();
    qreal bestDistance = 1e18;
    QPointF intersection;
    for (const auto& edge : edges) {
        if (line.intersects(edge, &intersection) == QLineF::BoundedIntersection) {
            const qreal dx = intersection.x() - line.p1().x();
            const qreal dy = intersection.y() - line.p1().y();
            const qreal distance = dx * dx + dy * dy;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = intersection;
                hit = true;
            }
        }
    }

    return best;
}

void drawPixmapCentered(QPainter& p, const QPixmap& pixmap, const QPointF& center,
                        const QSizeF& size, qreal opacity = 1.0, qreal rotation = 0.0)
{
    if (pixmap.isNull() || size.isEmpty()) return;

    p.save();
    p.setOpacity(qBound<qreal>(0.0, opacity, 1.0));
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.translate(center);
    if (std::abs(rotation) > 0.001) {
        p.rotate(rotation);
    }
    const QRectF target(-size.width() / 2.0, -size.height() / 2.0, size.width(), size.height());
    p.drawPixmap(target, pixmap, QRectF(pixmap.rect()));
    p.restore();
}
}

// ============================================================
// 构造/析构
// ============================================================

GameWindow::GameWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("渔途");
    setFixedSize(1280, 720);
    setMouseTracking(true);

    gm = new GameManager();

    // 加载图片
    imgSeaBackground.load(":/FishingVoyage/backgrounds/sea.png");
    imgSardine.load(":/FishingVoyage/fish/sardine_swim.png");
    imgTuna.load(":/FishingVoyage/fish/tuna_swim.png");
    imgEel.load(":/FishingVoyage/fish/eel_swim.png");
    imgGolden.load(":/FishingVoyage/fish/golden_swim.png");
    imgAnchovy.load(":/FishingVoyage/fish/anchovy_swim.png");
    imgClownfish.load(":/FishingVoyage/fish/clownfish_swim.png");
    imgMackerel.load(":/FishingVoyage/fish/mackerel_swim.png");
    imgSeaBream.load(":/FishingVoyage/fish/sea_bream_swim.png");
    imgLanternfish.load(":/FishingVoyage/fish/lanternfish_swim.png");
    imgGrouper.load(":/FishingVoyage/fish/grouper_swim.png");
    imgKoi.load(":/FishingVoyage/fish/koi_swim.png");
    imgCrystalFish.load(":/FishingVoyage/fish/crystal_fish_swim.png");
    imgShark.load(":/FishingVoyage/enemies/shark.png");
    imgSwordfish.load(":/FishingVoyage/enemies/swordfish.png");
    imgOctopus.load(":/FishingVoyage/enemies/octopus.png");
    imgElectricRay.load(":/FishingVoyage/enemies/electric_ray.png");
    imgPoisonJellyfish.load(":/FishingVoyage/enemies/poison_jellyfish.png");
    imgBoat.load(":/FishingVoyage/player/rod_right.png");
    imgObstacleReef.load(":/FishingVoyage/obstacles/reef.png");
    imgWaveOverlay.load(":/FishingVoyage/water/wave_current_overlay.png");
    imgObstacleWhirlpool.load(":/FishingVoyage/obstacles/whirlpool_sheet.png");
    imgStormLightning.load(":/FishingVoyage/weather/storm_lightning_sheet.png");
    imgHarpoonProjectile.load(":/FishingVoyage/projectiles/harpoon.png");
    imgOctopusInk.load(":/FishingVoyage/projectiles/octopus_ink.png");
    imgWoodNoticeBoard.load(":/FishingVoyage/ui/common/wood_notice_board.png");
    imgWoodNoticeButton.load(":/FishingVoyage/ui/common/wood_notice_button.png");
    imgNoticeIconInfo.load(":/FishingVoyage/ui/common/notice_icon_info.png");
    imgFinalVictoryBoard.load(":/FishingVoyage/ui/final/final_victory_board.png");
    imgRainCluster.load(":/FishingVoyage/effects/rain_cluster.png");
    imgRainStreaks.load(":/FishingVoyage/effects/rain_streaks.png");
    imgFogEdgeOverlay.load(":/FishingVoyage/effects/fog_edge_overlay.png");
    imgLightningWarningRing.load(":/FishingVoyage/effects/lightning_warning_ring.png");
    imgBossWarningRing.load(":/FishingVoyage/effects/boss_warning_ring.png");
    imgBossWarningRect.load(":/FishingVoyage/effects/boss_warning_rect.png");
    imgShockwaveRing.load(":/FishingVoyage/effects/shockwave_ring.png");
    imgElectricDischarge.load(":/FishingVoyage/effects/electric_discharge.png");
    imgJellyfishSting.load(":/FishingVoyage/effects/jellyfish_sting.png");
    imgWeaponRangeRing.load(":/FishingVoyage/effects/weapon_range_ring.png");
    imgHitSpark.load(":/FishingVoyage/effects/hit_spark.png");
    imgMuzzleFlash.load(":/FishingVoyage/effects/muzzle_flash.png");
    imgFiveHeadIdle.load(":/FishingVoyage/boss/five_head_shark/idle.png");
    imgFiveHeadBite.load(":/FishingVoyage/boss/five_head_shark/bite.png");
    imgFiveHeadCast.load(":/FishingVoyage/boss/five_head_shark/cast.png");
    imgFiveHeadBombardment.load(":/FishingVoyage/boss/five_head_shark/bombardment.png");
    imgFiveHeadSummonWater.load(":/FishingVoyage/boss/five_head_shark/summon_water.png");
    imgFiveHeadHit.load(":/FishingVoyage/boss/five_head_shark/hit.png");
    imgFiveHeadDeath.load(":/FishingVoyage/boss/five_head_shark/death.png");
    imgSirenIdle.load(":/FishingVoyage/boss/siren/idle.png");
    imgSirenPhaseTransition.load(":/FishingVoyage/boss/siren/phase_transition.png");
    imgSirenSoulSongWindup.load(":/FishingVoyage/boss/siren/soul_song_windup.png");
    imgSirenSoulSong.load(":/FishingVoyage/boss/siren/soul_song.png");
    imgSirenSoulSongWarningBeam.load(":/FishingVoyage/boss/siren/soul_song_warning_beam.png");
    imgSirenSoulSongBeam.load(":/FishingVoyage/boss/siren/soul_song_beam.png");
    imgSirenElegyWindup.load(":/FishingVoyage/boss/siren/elegy_windup.png");
    imgSirenElegyWave.load(":/FishingVoyage/boss/siren/elegy_wave.png");
    imgSirenElegyPull.load(":/FishingVoyage/boss/siren/elegy_pull.png");
    imgSirenSeaweed.load(":/FishingVoyage/boss/siren/seaweed_zone.png");
    imgSirenReef.load(":/FishingVoyage/boss/siren/reef_emerge.png");
    imgSirenPhantomIdle.load(":/FishingVoyage/boss/siren/phantom_idle.png");
    imgSirenPhantomMove.load(":/FishingVoyage/boss/siren/phantom_move.png");
    imgSirenPhantomStun.load(":/FishingVoyage/boss/siren/phantom_stun.png");
    imgSirenImmunity.load(":/FishingVoyage/boss/siren/immunity.png");
    imgSirenResonancePillar.load(":/FishingVoyage/boss/siren/resonance_pillar.png");
    imgSirenFocusMeter.load(":/FishingVoyage/boss/siren/focus_meter.png");
    imgSirenDeath.load(":/FishingVoyage/boss/siren/death.png");
    imgBossEncounterWarning.load(":/FishingVoyage/boss/common/encounter_warning.png");
    imgStageDecor[0].load(":/FishingVoyage/decor/stage1_islet.png");
    imgStageDecor[1].load(":/FishingVoyage/decor/stage2_lighthouse.png");
    imgStageDecor[2].load(":/FishingVoyage/decor/stage3_coral.png");
    imgStageDecor[3].load(":/FishingVoyage/decor/stage4_shipwreck.png");
    imgStageDecor[4].load(":/FishingVoyage/decor/stage5_ruins.png");
    imgStageDecor[5].load(":/FishingVoyage/decor/stage6_shoal.png");
    imgTerrainProps[0].load(":/FishingVoyage/decor/props/rocks.png");
    imgTerrainProps[1].load(":/FishingVoyage/decor/props/shoal.png");
    imgTerrainProps[2].load(":/FishingVoyage/decor/props/palm_islet.png");
    imgTerrainProps[3].load(":/FishingVoyage/decor/props/coral_outcrop.png");
    imgTerrainProps[4].load(":/FishingVoyage/decor/props/shipwreck.png");
    imgTerrainProps[5].load(":/FishingVoyage/decor/props/buoy.png");
    imgTerrainProps[6].load(":/FishingVoyage/decor/props/broken_dock.png");
    imgTerrainProps[7].load(":/FishingVoyage/decor/props/stone_arch.png");
    imgTerrainProps[8].load(":/FishingVoyage/decor/props/reef_patch.png");
    imgTerrainProps[9].load(":/FishingVoyage/decor/props/driftwood.png");
    imgTerrainProps[10].load(":/FishingVoyage/decor/props/lighthouse_rock.png");
    imgTerrainProps[11].load(":/FishingVoyage/decor/props/stone_marker.png");
    const char* weaponKeys[5] = { "rod", "net", "harpoon", "pistol", "shotgun" };
    const char* dirKeys[4] = { "up", "down", "left", "right" };
    for (int wi = 0; wi < 5; ++wi) {
        for (int di = 0; di < 4; ++di) {
            imgPlayerMove[wi][di].load(QString(":/FishingVoyage/player/%1_%2.png").arg(weaponKeys[wi], dirKeys[di]));
            imgPlayerBoost[wi][di].load(QString(":/FishingVoyage/player/%1_%2_boost.png").arg(weaponKeys[wi], dirKeys[di]));
        }
    }
    imgMenuBackground.load(":/FishingVoyage/ui/menu/background.png");
    imgMenuPanel.load(":/FishingVoyage/ui/menu/panel.png");
    imgMenuTitlePlaque.load(":/FishingVoyage/ui/menu/title_plaque.png");
    imgMenuRecordPanel.load(":/FishingVoyage/ui/menu/record_panel.png");
    imgMenuButtonNormal.load(":/FishingVoyage/ui/menu/button_normal.png");
    imgMenuButtonHover.load(":/FishingVoyage/ui/menu/button_hover.png");
    imgMenuButtonDisabled.load(":/FishingVoyage/ui/menu/button_disabled.png");
    imgStageStartPrompt.load(":/FishingVoyage/ui/prompts/stage_start.png");
    imgStageClearPrompt.load(":/FishingVoyage/ui/prompts/stage_clear.png");
    imgHudTopStatusBar.load(":/FishingVoyage/ui/hud/top_status_bar.png");
    imgHudHealthFill.load(":/FishingVoyage/ui/hud/bar_health_fill.png");
    imgHudStaminaFill.load(":/FishingVoyage/ui/hud/bar_stamina_fill.png");
    imgHudEquipmentPanel.load(":/FishingVoyage/ui/hud/panel_equipment.png");
    imgHudHotbar.load(":/FishingVoyage/ui/hud/hotbar.png");
    imgHudMinimapPanel.load(":/FishingVoyage/ui/hud/panel_minimap.png");
    imgHudLogPanel.load(":/FishingVoyage/ui/hud/panel_log.png");
    imgHudSlotNormal.load(":/FishingVoyage/ui/hud/slot_normal.png");
    imgHudSlotSelected.load(":/FishingVoyage/ui/hud/slot_selected.png");
    imgHudIconHeart.load(":/FishingVoyage/ui/hud/icon_heart.png");
    imgHudIconLightning.load(":/FishingVoyage/ui/hud/icon_lightning.png");
    imgHudIconCoin.load(":/FishingVoyage/ui/hud/icon_coin.png");
    imgHudIconFish.load(":/FishingVoyage/ui/hud/icon_fish.png");
    imgHudIconSun.load(":/FishingVoyage/ui/hud/icon_sun.png");
    imgHudIconCompass.load(":/FishingVoyage/ui/hud/icon_compass.png");
    imgFishingQtePanel.load(":/FishingVoyage/ui/fishing/qte_panel.png");
    imgIconWeaponRod.load(":/FishingVoyage/ui/icons/weapon_rod.png");
    imgIconWeaponNet.load(":/FishingVoyage/ui/icons/weapon_net.png");
    imgIconWeaponHarpoon.load(":/FishingVoyage/ui/icons/weapon_harpoon.png");
    imgIconWeaponPistol.load(":/FishingVoyage/ui/icons/weapon_pistol.png");
    imgIconWeaponShotgun.load(":/FishingVoyage/ui/icons/weapon_shotgun.png");
    imgIconItemFood.load(":/FishingVoyage/ui/icons/item_food.png");
    imgIconItemRepairT1.load(":/FishingVoyage/ui/icons/item_repair_t1.png");
    imgIconItemRepairT2.load(":/FishingVoyage/ui/icons/item_repair_t2.png");
    imgIconItemRepairT3.load(":/FishingVoyage/ui/icons/item_repair_t3.png");
    imgIconItemEmergencyRepair.load(":/FishingVoyage/ui/icons/item_emergency_repair.png");

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
    case STATE_STAGE_START: drawStageStartPrompt(p); break;
    case STATE_STAGE_CLEAR: drawStageClearPrompt(p); break;
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
        updateAttackProjectiles();
        updateHitFeedbacks();
        updateFloatingNotice();
        if (gm->gameOver) { state = STATE_DEFEAT;  update(); return; }
        if (gm->victory) {
            saveVictoryHighScore();
            state = STATE_VICTORY;
            update();
            return;
        }

        // 关卡通关
        if (gm->stageClear) {
            resetFishingState(true);

            Player::instance().clearInputState();
            promptButtonHover = false;
            setCursor(Qt::ArrowCursor);
            state = STATE_STAGE_CLEAR;
            update();
            return;
        }

        updateFishing();
        gm->update();
        if (gm->boss && gm->boss->alive && !bossEncounterShown) {
            bossEncounterShown = true;
            bossEncounterRemainingMs = 1800;
            encounterBossKind = gm->boss->kind;
        }
        if (bossEncounterRemainingMs > 0) {
            bossEncounterRemainingMs = qMax(0, bossEncounterRemainingMs - 16);
        }
        applyTestModeBenefits();
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
    if (!imgSeaBackground.isNull()) {
        const int tileW = qMax(1, imgSeaBackground.width());
        const int offset = -static_cast<int>(gm->cameraX * 0.35) % tileW;
        int startX = offset > 0 ? offset - tileW : offset;
        for (int x = startX; x < 1280; x += tileW) {
            p.drawPixmap(QRect(x, 0, tileW, 720), imgSeaBackground);
        }
    }
    else {
        p.fillRect(0, 0, 1280, 720, QColor(30, 100, 180));
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int sparkleOffset = (gm->cameraX / 6 + static_cast<int>(now / 95)) % 160;
    for (int y = 78; y < 690; y += 62) {
        for (int x = -160 + ((y / 62) % 3) * 36 - sparkleOffset; x < 1380; x += 160) {
            const int pulse = static_cast<int>((now / 180 + x / 40 + y / 31) % 4);
            const QColor shine(230, 252, 255, 28 + pulse * 7);
            p.fillRect(x, y, 2, 2, shine);
            p.fillRect(x + 3, y + 1, 1, 1, shine.lighter(115));
        }
    }
    p.restore();
}

// ============================================================
// 开场说明
// ============================================================

void GameWindow::drawStageDecorations(QPainter& p)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    for (int i = 0; i < Config::GameConfig::STAGE_DECOR_COUNT; ++i) {
        const auto& decor = Config::GameConfig::STAGE_DECORS[i];
        if (decor.stage != gm->stage) continue;

        const QPixmap& pixmap = imgStageDecor[qBound(0, decor.imageIndex, 5)];
        if (pixmap.isNull()) continue;

        const int stageStart = Config::GameConfig::stageStartDistance(decor.stage);
        const int stageEnd = Config::GameConfig::stageConfig(decor.stage).targetDistance;
        const int stageLength = qMax(1, stageEnd - stageStart);
        const int worldX = stageStart + qRound(stageLength * decor.stageRatio);
        const int screenX = worldX - gm->cameraX;

        const int targetW = qMax(48, qRound(pixmap.width() * decor.scale));
        const int targetH = qMax(32, qRound(pixmap.height() * decor.scale));
        QRect target(screenX - targetW / 2, decor.y - targetH / 2, targetW, targetH);

        if (target.right() < -80 || target.left() > 1360) continue;

        QRect shadow(
            target.left() + target.width() / 9,
            target.bottom() - target.height() / 4,
            target.width() * 7 / 9,
            qMax(10, target.height() / 5)
        );
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 46, 86, 58));
        p.drawEllipse(shadow);

        if (decor.mirror) {
            p.save();
            p.translate(target.center().x(), 0);
            p.scale(-1, 1);
            QRect mirrored(-target.width() / 2, target.top(), target.width(), target.height());
            p.drawPixmap(mirrored, pixmap, pixmap.rect());
            p.restore();
        }
        else {
            p.drawPixmap(target, pixmap, pixmap.rect());
        }
    }

    for (int i = 0; i < Config::GameConfig::TERRAIN_PROP_COUNT; ++i) {
        const auto& prop = Config::GameConfig::TERRAIN_PROPS[i];
        if (prop.stage != gm->stage || prop.imageIndex < 0 || prop.imageIndex >= 12) continue;

        const QPixmap& pixmap = imgTerrainProps[prop.imageIndex];
        if (pixmap.isNull()) continue;

        const int stageStart = Config::GameConfig::stageStartDistance(prop.stage);
        const int stageEnd = Config::GameConfig::stageConfig(prop.stage).targetDistance;
        const int stageLength = qMax(1, stageEnd - stageStart);
        const int worldX = stageStart + qRound(stageLength * prop.stageRatio);
        const int screenX = worldX - gm->cameraX;
        const int targetW = qMax(36, qRound(pixmap.width() * prop.scale));
        const int targetH = qMax(28, qRound(pixmap.height() * prop.scale));
        const QRect target(screenX - targetW / 2, prop.y - targetH / 2, targetW, targetH);

        if (target.right() < -80 || target.left() > 1360) continue;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 40, 72, 44));
        p.drawEllipse(QRect(
            target.left() + target.width() / 8,
            target.bottom() - qMax(10, target.height() / 5),
            target.width() * 3 / 4,
            qMax(8, target.height() / 6)
        ));

        if (prop.mirror) {
            p.save();
            p.translate(target.center().x(), 0);
            p.scale(-1, 1);
            p.drawPixmap(QRect(-target.width() / 2, target.top(), target.width(), target.height()),
                         pixmap, pixmap.rect());
            p.restore();
        }
        else {
            p.drawPixmap(target, pixmap, pixmap.rect());
        }
    }

    p.restore();
}

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
        QStringLiteral("【目标】  驾船向右航行，闯过 %1 个关卡，在关键海域击败 Boss").arg(Config::GameConfig::STAGE_COUNT),
        "",
        "【移动】  WASD 移动    Shift 加速（消耗体力）    空格键 闪避冲刺（短暂无敌）",
        "",
        "【捕鱼】  装备鱼竿/渔网/鱼叉后，鼠标左键点击鱼即可开始捕捉",
        "             倒计时内连续点击鼠标左键完成捕获，时间过半完成则体力消耗减半",
        "             黄色沙丁鱼：价值低，易捕      蓝色金枪鱼：价值中，易捕",
        "             紫色深海鳗：价值高，难捕      金色金鱼：价值极高，极难捕",
        "",
        "【战斗】  装备武器后，鼠标左键点击敌人进行攻击（命中才扣耐久，有冷却）",
        "          优先攻击敌人；未命中时，若武器支持捕鱼则自动尝试捕鱼",
        "          E键 震荡波：满充能时释放，伤害范围内小怪并眩晕Boss，释放后自动回充",
        "",
        "【障碍】  暗礁：碰撞损失耐久并反弹      漩涡：减少体力并降速",
        "",
        "【背包/商店】  按 B 打开船舱背包      H 打开航海图鉴      ESC 暂停",
        "             每关 Boss 击败后会自动进入码头商店整备",
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
        p.drawText(0, 688, 1280, 30, Qt::AlignCenter, "按任意键开始游戏");
    }
}

// ============================================================
// 主菜单
// ============================================================

void GameWindow::drawMenu(QPainter& p)
{
    if (!imgMenuBackground.isNull()) {
        p.drawPixmap(rect(), imgMenuBackground);

        p.save();

        QRect panelRect(360, 100, 560, 390);
        QRect titleRect(330, 16, 620, 180);
        QRect recordRect(405, 500, 470, 150);

        if (!imgMenuPanel.isNull()) {
            p.drawPixmap(panelRect, imgMenuPanel);
        }
        if (!imgMenuTitlePlaque.isNull()) {
            p.drawPixmap(titleRect, imgMenuTitlePlaque);
        }
        if (!imgMenuRecordPanel.isNull()) {
            p.drawPixmap(recordRect, imgMenuRecordPanel);
        }

        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setPen(QColor(83, 43, 14));
        p.setFont(QFont("Microsoft YaHei", 48, QFont::Bold));
        p.drawText(titleRect.adjusted(4, 16, 4, -44), Qt::AlignCenter, "渔 途");
        p.setPen(QColor(255, 190, 58));
        p.drawText(titleRect.adjusted(0, 12, 0, -48), Qt::AlignCenter, "渔 途");
        p.setPen(QColor(245, 217, 150));
        p.setFont(QFont("Microsoft YaHei", 19, QFont::Bold));
        p.drawText(titleRect.adjusted(0, 100, 0, -16), Qt::AlignCenter, "FISHING VOYAGE");

        const QString labels[6] = {
            "开始航程", "继续游戏", "航海图鉴", "操作说明", "游戏设置", "退出游戏"
        };
        const bool hasSave = gm->fileManager.hasSave();
        for (int i = 0; i < 6; ++i) {
            const bool disabled = (i == 1 && !hasSave);
            const bool hovered = (i == menuHoverIndex && !disabled);
            const QPixmap& buttonImage = disabled
                ? imgMenuButtonDisabled
                : (hovered ? imgMenuButtonHover : imgMenuButtonNormal);

            if (!buttonImage.isNull()) {
                p.drawPixmap(menuButtonRect(i), buttonImage);
            }

            QRect textRect = menuButtonRect(i);

            p.setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
            p.setPen(disabled ? QColor(70, 62, 54) : QColor(82, 43, 16));
            p.drawText(textRect.translated(2, 2), Qt::AlignCenter, labels[i]);
            p.setPen(disabled ? QColor(114, 104, 91) : QColor(35, 20, 10));
            p.drawText(textRect, Qt::AlignCenter, labels[i]);

            if (i == 0) {
                QRect anchorRect = menuButtonRect(i).adjusted(38, 9, -318, -9);
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setPen(QPen(QColor(122, 67, 20), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                QPoint center = anchorRect.center();
                p.drawEllipse(QRect(center.x() - 6, anchorRect.top(), 12, 12));
                p.drawLine(center.x(), anchorRect.top() + 12, center.x(), anchorRect.bottom() - 8);
                p.drawLine(anchorRect.left() + 7, anchorRect.top() + 20, anchorRect.right() - 7, anchorRect.top() + 20);
                p.drawArc(anchorRect.adjusted(4, 12, -4, -2), 200 * 16, 140 * 16);
                p.drawLine(anchorRect.left() + 5, anchorRect.bottom() - 12, anchorRect.left() + 13, anchorRect.bottom() - 17);
                p.drawLine(anchorRect.right() - 5, anchorRect.bottom() - 12, anchorRect.right() - 13, anchorRect.bottom() - 17);
                p.setRenderHint(QPainter::Antialiasing, false);
            }
        }

        p.setPen(QColor(79, 43, 18));
        p.setFont(QFont("Microsoft YaHei", 17, QFont::Bold));
        p.drawText(recordRect.adjusted(0, 8, 0, -70), Qt::AlignCenter, "当前航海记录");
        p.setFont(QFont("Microsoft YaHei", 13, QFont::Bold));
        p.drawText(recordRect.adjusted(24, 42, -24, -40), Qt::AlignCenter,
            QString("第 %1 关  |  金币 %2  |  鱼获 %3")
            .arg(gm->stage)
            .arg(coinDisplayText(Player::instance()))
            .arg(Player::instance().fishCaught));
        Weapon* currentWeapon = InventorySystem::instance().currentWeapon();
        QString weaponName = currentWeapon
            ? QString::fromStdString(currentWeapon->getName())
            : QString("无");
        p.drawText(recordRect.adjusted(24, 70, -24, -12), Qt::AlignCenter,
            QString("当前装备：%1").arg(weaponName));

        p.restore();
        return;
    }

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

void GameWindow::drawStageStartPrompt(QPainter& p)
{
    drawGame(p);
    p.fillRect(0, 0, 1280, 720, QColor(0, 10, 24, 135));
    const QRect popup = promptPopupRect();
    p.drawPixmap(popup, imgStageStartPrompt, imgStageStartPrompt.rect());
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const int stage = gm ? qBound(1, gm->stage, Config::GameConfig::STAGE_COUNT) : 1;
    const QColor gold(255, 224, 126);
    const QColor brown(68, 36, 17);
    const QColor dark(42, 22, 10);
    const QColor teal(224, 252, 231);

    drawPromptText(p, QRect(432, 82, 416, 64), QStringLiteral("第 %1 关").arg(stage),
                   31, gold, true, Qt::AlignCenter, QColor(54, 25, 7, 210));
    drawPromptText(p, QRect(260, 178, 760, 76), stageName(stage),
                   46, brown, true, Qt::AlignCenter, QColor(247, 218, 150, 135));
    drawPromptText(p, QRect(285, 255, 710, 38), stageBrief(stage),
                   22, dark, false, Qt::AlignCenter, QColor(246, 219, 154, 85));
    drawPromptText(p, QRect(300, 352, 680, 58), stageStartObjective(stage),
                   34, brown, true, Qt::AlignCenter, QColor(247, 219, 150, 125));
    drawPromptText(p, QRect(325, 448, 630, 36), stageStartSeaLine(stage),
                   22, dark, true, Qt::AlignCenter, QColor(246, 219, 154, 70));

    drawPromptText(p, stagePromptButtonRect(), QStringLiteral("开始航行"),
                   29, teal, true, Qt::AlignCenter, QColor(18, 67, 68, 230));
}

void GameWindow::drawStageClearPrompt(QPainter& p)
{
    drawGame(p);
    p.fillRect(0, 0, 1280, 720, QColor(0, 10, 24, 135));
    const QRect popup = promptPopupRect();
    p.drawPixmap(popup, imgStageClearPrompt, imgStageClearPrompt.rect());
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const int stage = gm ? qBound(1, gm->stage, Config::GameConfig::STAGE_COUNT) : 1;
    const QColor gold(255, 224, 126);
    const QColor cream(255, 243, 210);
    const QColor brown(68, 36, 17);
    const QColor dark(42, 22, 10);
    const QColor teal(224, 252, 231);

    drawPromptText(p, QRect(432, 82, 416, 64), QStringLiteral("第 %1 关完成").arg(stage),
                   29, gold, true, Qt::AlignCenter, QColor(55, 25, 7, 220));
    drawPromptText(p, QRect(260, 178, 760, 70), QStringLiteral("%1  已通过").arg(stageName(stage)),
                   39, brown, true, Qt::AlignCenter, QColor(247, 218, 150, 130));
    drawPromptText(p, QRect(280, 252, 720, 38),
                   QStringLiteral("已完成 %1m 航程，%2").arg(stageLengthMeters(stage)).arg(stageClearSummary(stage)),
                   21, dark, false, Qt::AlignCenter, QColor(246, 219, 154, 75));

    drawPromptText(p, QRect(295, 360, 210, 38),
                   QStringLiteral("累计鱼获 %1条").arg(Player::instance().fishCaught),
                   22, cream, true, Qt::AlignCenter, QColor(42, 17, 8, 230));
    drawPromptText(p, QRect(535, 360, 210, 38),
                   QStringLiteral("航程 %1m").arg(stageLengthMeters(stage)),
                   22, cream, true, Qt::AlignCenter, QColor(42, 17, 8, 230));
    drawPromptText(p, QRect(775, 360, 210, 38), QStringLiteral("回港补给"),
                   22, cream, true, Qt::AlignCenter, QColor(42, 17, 8, 230));

    drawPromptText(p, QRect(280, 470, 720, 44), nextStageLine(stage),
                   20, dark, false, Qt::AlignCenter, QColor(246, 219, 154, 70));

    drawPromptText(p, stagePromptButtonRect(), QStringLiteral("回到港口"),
                   29, teal, true, Qt::AlignCenter, QColor(18, 67, 68, 230));
}

void GameWindow::drawGame(QPainter& p)
{
    drawSea(p);
    drawStageDecorations(p);
    drawObstacles(p);
    drawWaves(p);
    drawFish(p);
    drawBossHazards(p);
    drawSharks(p);
    drawShockWaveEffect(p);
    drawPlayer(p);
    drawAttackProjectiles(p);
    drawHitFeedbacks(p);
    // 天气叠加效果
    QColor overlay = WeatherSystem::instance().overlayColor();
    if (overlay.alpha() > 0)
        p.fillRect(0, 0, 1280, 720, overlay);

    if (Player::instance().visionReduced && !imgOctopusInk.isNull()) {
        const int frameWidth = imgOctopusInk.width() / 4;
        const QRect source(frameWidth * 3, 0, frameWidth, imgOctopusInk.height());
        p.save();
        p.setOpacity(0.82);
        p.drawPixmap(QRect(330, 175, 620, 370), imgOctopusInk, source);
        p.setOpacity(0.38);
        p.drawPixmap(QRect(455, 250, 370, 220), imgOctopusInk, source);
        p.restore();
    }

    drawWeatherEffects(p);
    drawHUD(p);
    drawWaveNotice(p);
    drawFishingHUD(p);
    drawTestModeOverlay(p);
    drawFloatingNotice(p);
    drawBossEncounterNotice(p);

    if (state == STATE_PAUSED) drawPaused(p);
}

// ============================================================
// 鱼
// ============================================================

void GameWindow::drawFish(QPainter& p)
{
    for (auto f : gm->fish) {
        if (!f || f->caught || f->escaped) continue;
        int screenX = f->x - gm->cameraX;
        if (screenX < -20 || screenX > 1300) continue;

        const bool locked = isFishing && f == targetFish;
        if (locked) {
            const int pulse = 4 + static_cast<int>((QDateTime::currentMSecsSinceEpoch() / 120) % 4);
            drawPixmapCentered(p, imgWeaponRangeRing, QPointF(screenX, f->y),
                               QSizeF(110 + pulse * 2, 70 + pulse * 2), 0.62);
        }

        QPixmap* img = nullptr;
        switch (f->type) {
        case Fish::SARDINE:        img = &imgSardine; break;
        case Fish::TUNA:           img = &imgTuna;    break;
        case Fish::DEEPSEAEEL:     img = &imgEel;     break;
        case Fish::SWORDFISH_FISH: img = &imgGolden;  break;
        case Fish::ANCHOVY:        img = &imgAnchovy; break;
        case Fish::CLOWNFISH:      img = &imgClownfish; break;
        case Fish::MACKEREL:       img = &imgMackerel; break;
        case Fish::SEA_BREAM:      img = &imgSeaBream; break;
        case Fish::LANTERNFISH:    img = &imgLanternfish; break;
        case Fish::GROUPER:        img = &imgGrouper; break;
        case Fish::KOI:            img = &imgKoi; break;
        case Fish::CRYSTAL_FISH:   img = &imgCrystalFish; break;
        }

        if (img && !img->isNull()) {
            const int frameCount = 4;
            const int frameSize = img->width() / frameCount;
            int frame = static_cast<int>((QDateTime::currentMSecsSinceEpoch() / 140) % frameCount);
            if (f->type == Fish::DEEPSEAEEL) {
                static const int smoothEelFrames[] = {0, 1, 2, 3, 2, 1};
                const int sequenceIndex = static_cast<int>(
                    (QDateTime::currentMSecsSinceEpoch() / 210) % 6);
                frame = smoothEelFrames[sequenceIndex];
            }
            const QRect source(frame * frameSize, 0, frameSize, img->height());
            const QSizeF drawSize = fishDrawSize(f->type);
            const QRect target(
                qRound(screenX - drawSize.width() / 2.0),
                qRound(f->y - drawSize.height() / 2.0),
                qRound(drawSize.width()),
                qRound(drawSize.height())
            );
            if (f->facingX < 0.0f) {
                p.save();
                p.translate(target.left() + target.width(), target.top());
                p.scale(-1, 1);
                p.drawPixmap(QRect(0, 0, target.width(), target.height()), *img, source);
                p.restore();
            }
            else {
                p.drawPixmap(target, *img, source);
            }
        }
        else {
            p.setBrush(QColor(255, 220, 50));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - 8, f->y - 5, 16, 10);
        }
    }
}

// ============================================================
// 障碍物
// ============================================================

void GameWindow::drawObstacles(QPainter& p)
{
    for (Fish* f : gm->fish) {
        if (!f || f->caught || f->escaped || !f->isStunned()) continue;
        const int screenX = f->x - gm->cameraX;
        if (screenX < -80 || screenX > 1360) continue;
        const qreal pulse = 0.82 + 0.12 * std::sin(
            QDateTime::currentMSecsSinceEpoch() * 0.012);
        if (!imgShockwaveRing.isNull()) {
            drawPixmapCentered(p, imgShockwaveRing,
                               QPointF(screenX, f->y - 3),
                               QSizeF(72 * pulse, 42 * pulse), 0.72);
        }
        p.save();
        p.setPen(QColor(190, 250, 255));
        p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        p.drawText(QRectF(screenX - 42, f->y - 39, 84, 18),
                   Qt::AlignCenter, QStringLiteral("眩晕"));
        p.restore();
    }

    const auto& obstacles = ObstacleManager::instance().obstacles();
    for (auto* o : obstacles) {
        if (!o) continue;
        QPointF playerPos(gm->playerX(), gm->playerY());
        if (!o->isVisible(playerPos)) continue;

        int screenX = (int)o->worldPos().x() - gm->cameraX;
        int screenY = (int)o->worldPos().y();
        int size = o->size();

        if (screenX < -size || screenX > 1280 + size) continue;

        if (o->type() == ObstacleType::REEF) {
            if (!imgObstacleReef.isNull()) {
                const int targetW = qMax(104, size * 5);
                const int targetH = qMax(78, size * 4);
                QRect target(screenX - targetW / 2, screenY - targetH / 2 - size / 3, targetW, targetH);
                p.drawPixmap(target, imgObstacleReef, imgObstacleReef.rect());
            }
            else {
                p.setBrush(QColor(120, 80, 40));
                p.setPen(QPen(QColor(80, 50, 20), 2));
                p.drawRect(screenX - size, screenY - size, size * 2, size * 2);
            }
        }
        else {
            if (!imgObstacleWhirlpool.isNull()) {
                const int frameCount = 8;
                const int frameWidth = imgObstacleWhirlpool.width() / frameCount;
                const int frame = static_cast<int>(((QDateTime::currentMSecsSinceEpoch() / 85) + screenX / 19) % frameCount);
                const int targetSize = qMax(92, size * 4);
                QRect target(screenX - targetSize / 2, screenY - targetSize / 2, targetSize, targetSize);
                QRect source(frame * frameWidth, 0, frameWidth, imgObstacleWhirlpool.height());
                p.drawPixmap(target, imgObstacleWhirlpool, source);
            }
            else {
                p.setBrush(QColor(80, 180, 200, 160));
                p.setPen(QPen(QColor(100, 200, 220), 2));
                p.drawEllipse(screenX - size, screenY - size, size * 2, size * 2);
                p.setPen(QColor(200, 240, 255));
                p.setFont(QFont("Microsoft YaHei", 10));
                p.drawText(screenX - 8, screenY + 5, "〜");
            }
        }
    }
}

// ============================================================
// 海浪提示
// ============================================================

void GameWindow::drawWaves(QPainter& p)
{
    WaveSystem& wave = WaveSystem::instance();
    if (!wave.isWarningActive() && !wave.isWaveActive()) return;

    const bool warning = wave.isWarningActive();
    const bool rightward = wave.currentDirection() == WaveDirection::RIGHT;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qreal progress = warning ? wave.warningProgress() : wave.activeProgress();

    {
        QColor deep = rightward ? QColor(14, 112, 132, 118) : QColor(17, 74, 132, 126);
        QColor mid = rightward ? QColor(44, 184, 190, 74) : QColor(54, 122, 184, 82);
        QColor foam = rightward ? QColor(218, 255, 245, 190) : QColor(210, 232, 255, 180);
        QColor accent = rightward ? QColor(87, 226, 210, 214) : QColor(248, 186, 92, 220);
        if (WeatherSystem::instance().currentWeather() == WeatherType::STORM) {
            deep = deep.darker(135);
            mid = mid.darker(125);
            foam = foam.darker(108);
        }

        p.save();
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRect waterRect(0, 52, 1280, 640);
        const int revealW = warning
            ? qBound(140, static_cast<int>(waterRect.width() * (0.18 + progress * 0.48)), waterRect.width())
            : waterRect.width();
        const QRect clipRect(
            rightward ? waterRect.left() : waterRect.right() - revealW + 1,
            waterRect.top(),
            revealW,
            waterRect.height()
        );

        const int edgeW = qMin(revealW, warning ? 320 : 420);
        const QRect edgeRect(
            rightward ? waterRect.left() : waterRect.right() - edgeW + 1,
            waterRect.top(),
            edgeW,
            waterRect.height()
        );
        QLinearGradient edgeGradient(
            rightward ? edgeRect.left() : edgeRect.right(), 0,
            rightward ? edgeRect.right() : edgeRect.left(), 0
        );
        edgeGradient.setColorAt(0.0, deep);
        edgeGradient.setColorAt(0.58, mid);
        edgeGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
        p.fillRect(edgeRect, edgeGradient);

        if (!imgWaveOverlay.isNull()) {
            p.save();
            p.setClipRect(clipRect);
            const qreal baseOpacity = warning
                ? (0.22 + progress * 0.24)
                : (0.48 + (1.0 - progress) * 0.08);
            p.setOpacity(baseOpacity);

            if (!rightward) {
                p.translate(1280, 0);
                p.scale(-1, 1);
            }

            const int drift = static_cast<int>((now / (warning ? 78 : 42)) % 1280);
            const int startX = -1280 + drift;
            for (int x = startX; x < 2560; x += 1280) {
                p.drawPixmap(QRect(x, 0, 1280, 720), imgWaveOverlay);
            }

            if (wave.isWaveActive()) {
                p.setOpacity(baseOpacity * 0.42);
                const int slowDrift = static_cast<int>((now / 95) % 1280);
                for (int x = -1280 + slowDrift; x < 2560; x += 1280) {
                    p.drawPixmap(QRect(x + 220, -70, 1280, 720), imgWaveOverlay);
                }
            }
            p.restore();
        }

        const int frontX = rightward ? clipRect.right() : clipRect.left();
        const int foamPhase = static_cast<int>((now / 72) % 36);
        p.setPen(Qt::NoPen);
        for (int y = waterRect.top() + 14; y < waterRect.bottom(); y += 30) {
            const int jitter = (foamPhase + y / 5) % 42;
            const int x = frontX + (rightward ? -jitter : jitter);
            p.fillRect(x - 12, y, 18, 3, QColor(foam.red(), foam.green(), foam.blue(), warning ? 92 : 118));
            p.fillRect(x + (rightward ? -34 : 16), y + 7, 24, 2, QColor(foam.red(), foam.green(), foam.blue(), warning ? 58 : 76));
        }

        p.restore();
        return;
    }

    QColor deep = rightward ? QColor(24, 116, 138, 118) : QColor(20, 64, 114, 126);
    QColor mid = rightward ? QColor(58, 178, 190, 96) : QColor(42, 103, 160, 102);
    QColor foam = rightward ? QColor(214, 252, 240, 185) : QColor(205, 228, 255, 176);
    QColor accent = rightward ? QColor(86, 220, 210, 210) : QColor(238, 166, 72, 220);
    if (WeatherSystem::instance().currentWeather() == WeatherType::STORM) {
        deep = deep.darker(135);
        mid = mid.darker(125);
        foam = foam.darker(108);
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);

    const int edgeW = warning ? 64 : 48;
    const QRect edgeRect(rightward ? 0 : 1280 - edgeW, 58, edgeW, 628);
    QLinearGradient edgeGradient(
        rightward ? edgeRect.left() : edgeRect.right(), 0,
        rightward ? edgeRect.right() : edgeRect.left(), 0
    );
    edgeGradient.setColorAt(0.0, deep);
    edgeGradient.setColorAt(0.55, mid);
    edgeGradient.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(edgeRect, edgeGradient);

    const int foamShift = static_cast<int>((now / 70) % 24);
    for (int y = edgeRect.top() + 10; y < edgeRect.bottom(); y += 22) {
        const int row = ((y / 22) % 3) * 5;
        if (rightward) {
            const int x = edgeRect.left() + 5 + (foamShift + row) % 18;
            p.fillRect(x, y, 30, 3, foam);
            p.fillRect(x + 12, y + 4, 24, 3, foam.lighter(106));
            p.fillRect(x + 30, y + 8, 18, 3, foam.darker(108));
        }
        else {
            const int x = edgeRect.right() - 35 - (foamShift + row) % 18;
            p.fillRect(x, y, 30, 3, foam);
            p.fillRect(x - 6, y + 4, 24, 3, foam.lighter(106));
            p.fillRect(x - 18, y + 8, 18, 3, foam.darker(108));
        }
    }

    auto drawPixelArrow = [&](int cx, int cy, bool pointsRight, const QColor& color) {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        const int dir = pointsRight ? 1 : -1;
        p.fillRect(cx - dir * 12, cy - 8, 14, 4, color);
        p.fillRect(cx - dir * 6, cy - 4, 14, 4, color);
        p.fillRect(cx, cy, 14, 4, color);
        p.fillRect(cx - dir * 6, cy + 4, 14, 4, color);
        p.fillRect(cx - dir * 12, cy + 8, 14, 4, color);
    };

    const int arrowPhase = static_cast<int>((now / 120) % 42);
    for (int i = 0; i < 7; ++i) {
        const int y = 128 + i * 70 + ((i % 2) * 10);
        const int cx = rightward
            ? edgeRect.left() + 18 + (arrowPhase + i * 5) % 36
            : edgeRect.right() - 18 - (arrowPhase + i * 5) % 36;
        drawPixelArrow(cx, y, rightward, accent);
    }

    if (wave.isWaveActive()) {
        const int streamOffset = static_cast<int>((now / 38) % 140);
        p.setPen(Qt::NoPen);
        for (int y = 92; y < 670; y += 34) {
            const int rowOffset = (y / 34) % 2 ? 70 : 0;
            for (int x = -160; x < 1440; x += 140) {
                const int sx = rightward
                    ? x + streamOffset + rowOffset
                    : 1280 - (x + streamOffset + rowOffset);
                p.fillRect(sx, y, 48, 2, QColor(foam.red(), foam.green(), foam.blue(), 64));
                p.fillRect(sx + (rightward ? 34 : -34), y + 5, 28, 2, QColor(foam.red(), foam.green(), foam.blue(), 42));
            }
        }
    }

    const QRect panel(28, 138, 232, 66);
    if (!imgWoodNoticeButton.isNull()) {
        p.drawPixmap(panel.adjusted(-16, -16, 16, 14), imgWoodNoticeButton, imgWoodNoticeButton.rect());
    } else {
        p.fillRect(panel, QColor(76, 40, 18, warning ? 230 : 205));
    }

    const QString title = warning
        ? QStringLiteral("海浪预警")
        : (rightward ? QStringLiteral("顺浪中") : QStringLiteral("逆浪中"));
    const QString detail = warning
        ? QStringLiteral("%1  %2s").arg(rightward ? QStringLiteral("顺浪") : QStringLiteral("逆浪"))
              .arg(QString::number(wave.warningRemainingMs() / 1000.0, 'f', 1))
        : (rightward ? QStringLiteral("航速提升") : QStringLiteral("航速降低"));

    p.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    p.setPen(QColor(255, 237, 184));
    p.drawText(panel.adjusted(14, 7, -14, -36), Qt::AlignLeft | Qt::AlignVCenter, title);
    p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    p.setPen(QColor(215, 238, 232));
    p.drawText(panel.adjusted(14, 30, -14, -14), Qt::AlignLeft | Qt::AlignVCenter, detail);

    const QRect bar(panel.left() + 14, panel.bottom() - 12, panel.width() - 28, 5);
    p.fillRect(bar, QColor(8, 20, 28, 190));
    const qreal remaining = warning ? (1.0 - progress) : (1.0 - wave.activeProgress());
    const int fillW = qBound(0, static_cast<int>(bar.width() * remaining), bar.width());
    p.fillRect(QRect(bar.left(), bar.top(), fillW, bar.height()), accent);
    p.fillRect(bar.left(), bar.top(), bar.width(), 1, QColor(255, 255, 230, 60));

    p.restore();
}

void GameWindow::drawWaveNotice(QPainter& p)
{
    WaveSystem& wave = WaveSystem::instance();
    if (!wave.isWarningActive() && !wave.isWaveActive()) return;

    const bool warning = wave.isWarningActive();
    const bool rightward = wave.currentDirection() == WaveDirection::RIGHT;
    const qreal progress = warning ? wave.warningProgress() : wave.activeProgress();
    const QColor accent = rightward ? QColor(87, 226, 210, 214) : QColor(248, 186, 92, 220);
    const QRect panel(28, 138, 232, 66);

    p.save();
    if (!imgWoodNoticeButton.isNull()) {
        p.drawPixmap(panel.adjusted(-16, -16, 16, 14), imgWoodNoticeButton, imgWoodNoticeButton.rect());
    } else {
        p.fillRect(panel, QColor(76, 40, 18, warning ? 230 : 205));
    }

    const QString title = warning
        ? QStringLiteral("\u6d77\u6d6a\u9884\u8b66")
        : (rightward ? QStringLiteral("\u987a\u6d6a\u4e2d") : QStringLiteral("\u9006\u6d6a\u4e2d"));
    const QString detail = warning
        ? QStringLiteral("%1  %2s").arg(rightward ? QStringLiteral("\u987a\u6d6a") : QStringLiteral("\u9006\u6d6a"))
              .arg(QString::number(wave.warningRemainingMs() / 1000.0, 'f', 1))
        : (rightward ? QStringLiteral("\u822a\u901f\u63d0\u5347") : QStringLiteral("\u822a\u901f\u964d\u4f4e"));

    p.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    p.setPen(QColor(255, 237, 184));
    p.drawText(panel.adjusted(14, 7, -14, -36), Qt::AlignLeft | Qt::AlignVCenter, title);
    p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    p.setPen(QColor(215, 238, 232));
    p.drawText(panel.adjusted(14, 30, -14, -14), Qt::AlignLeft | Qt::AlignVCenter, detail);

    const QRect bar(panel.left() + 14, panel.bottom() - 12, panel.width() - 28, 5);
    p.fillRect(bar, QColor(8, 20, 28, 190));
    const qreal remaining = warning ? (1.0 - progress) : (1.0 - wave.activeProgress());
    const int fillW = qBound(0, static_cast<int>(bar.width() * remaining), bar.width());
    p.fillRect(QRect(bar.left(), bar.top(), fillW, bar.height()), accent);
    p.fillRect(bar.left(), bar.top(), bar.width(), 1, QColor(255, 255, 230, 60));
    p.restore();
}

void GameWindow::drawWeatherEffects(QPainter& p)
{
    WeatherSystem& weather = WeatherSystem::instance();
    const qreal stormIntensity = weather.stormIntensity();
    const qreal fogIntensity = weather.fogIntensity();
    if (stormIntensity <= 0.01 && fogIntensity <= 0.01 && !weather.isTransitioning()) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    p.save();
    p.setClipRect(0, 0, 1280, 720);

    if (stormIntensity > 0.01 && !imgRainStreaks.isNull()) {
        const int frameCount = 4;
        const int frameWidth = imgRainStreaks.width() / frameCount;
        const int frame = static_cast<int>((now / 92) % frameCount);
        const QRect source(frame * frameWidth, 0, frameWidth, imgRainStreaks.height());
        const int rainLevel = qMax(1, weather.rainLevel());
        p.save();
        p.setOpacity((0.12 + rainLevel * 0.075) * qBound<qreal>(0.2, stormIntensity, 1.0));
        p.drawPixmap(QRect(0, 0, 1280, 720), imgRainStreaks, source);
        p.restore();
    }

    if (stormIntensity > 0.01 && !imgRainCluster.isNull()) {
        const int frameCount = 4;
        const int frameWidth = imgRainCluster.width() / frameCount;
        const qint64 animationTick = now / 105;
        p.setOpacity(0.34 + stormIntensity * 0.38);

        const int rainLevel = qMax(1, weather.rainLevel());
        const int baseSites = rainLevel == 1 ? 38 : (rainLevel == 2 ? 60 : 86);
        const int siteCount = qMax(10, qRound(baseSites * qBound<qreal>(0.22, stormIntensity, 1.0)));
        const int eventCycle = rainLevel == 1 ? 8 : (rainLevel == 2 ? 7 : 6);
        p.setOpacity((0.30 + rainLevel * 0.08) + stormIntensity * 0.28);

        const int worldSpacing = rainLevel == 1 ? 34 : (rainLevel == 2 ? 22 : 16);
        const int firstWorldCell = gm ? (gm->cameraX - 80) / worldSpacing : -3;
        for (int i = 0; i < siteCount; ++i) {
            const int worldCell = firstWorldCell + i;
            const int eventFrame = static_cast<int>(
                (animationTick + qAbs(worldCell * 3)) % eventCycle);
            if (eventFrame >= frameCount) continue;

            const int worldX = worldCell * worldSpacing +
                qAbs((worldCell * 47 + 19) % qMax(1, worldSpacing));
            const int x = worldX - (gm ? gm->cameraX : 0);
            if (x < -48 || x > 1328) continue;
            const int y = 92 + qAbs((worldCell * 113 + 47) % 590);
            const int width = 26 + qAbs(worldCell % 5) * 4 + rainLevel * 2;
            const int height = qMax(16, width * 10 / 27);
            const QRect source(eventFrame * frameWidth, 0, frameWidth, imgRainCluster.height());
            const QRect target(x - width / 2, y - height / 2, width, height);
            p.drawPixmap(target, imgRainCluster, source);
        }
        p.setOpacity(1.0);
    }

    if (fogIntensity > 0.01 && !imgFogEdgeOverlay.isNull()) {
        p.save();
        p.setOpacity((0.20 + 0.58 * fogIntensity) *
                     qBound<qreal>(0.0, fogIntensity, 1.0));
        p.drawPixmap(QRect(0, 0, 1280, 720), imgFogEdgeOverlay);
        p.restore();
    }

    if (stormIntensity > 0.85 && gm && (gm->lightningWarningActive || gm->lightningStrikeActive)) {
        const int sx = static_cast<int>(gm->lightningTarget.x()) - gm->cameraX;
        const int sy = static_cast<int>(gm->lightningTarget.y());
        const int radius = gm->lightningStrikeActive ? 90 : 82;
        if (gm->lightningWarningActive) {
            const int pulse = 8 + static_cast<int>((now / 80) % 10);
            const qreal size = (radius + pulse) * 2.45;
            drawPixmapCentered(p, imgLightningWarningRing, QPointF(sx, sy), QSizeF(size, size), 0.78);
        }
        else {
            const qreal size = radius * 2.55;
            drawPixmapCentered(p, imgLightningWarningRing, QPointF(sx, sy), QSizeF(size, size), 0.9);
        }
    }

    const int lightningPulse = static_cast<int>((now / 120) % 34);
    if (stormIntensity > 0.85 && !imgStormLightning.isNull() &&
        (lightningPulse < 4 || weather.shouldTriggerLightning())) {
        const int frameCount = 4;
        const int frameWidth = imgStormLightning.width() / frameCount;
        const int frame = qBound(0, lightningPulse, frameCount - 1);
        p.drawPixmap(QRect(0, 0, 1280, 720), imgStormLightning, QRect(frame * frameWidth, 0, frameWidth, imgStormLightning.height()));
    }

    if (weather.isTransitioning()) {
        QString prompt;
        switch (weather.incomingWeather()) {
        case WeatherType::SUNNY: prompt = QStringLiteral("云层正在散开"); break;
        case WeatherType::FOG: prompt = QStringLiteral("海雾正在靠近"); break;
        case WeatherType::STORM: prompt = QStringLiteral("风雨正在逼近"); break;
        }

        const qreal progress = weather.transitionProgress();
        const qreal fade = qMin<qreal>(1.0, qMin(progress * 5.0, (1.0 - progress) * 5.0));
        p.setOpacity(0.42 + 0.48 * fade);
        const QRect noticeRect(480, 132, 320, 62);
        if (!imgWoodNoticeButton.isNull()) {
            p.drawPixmap(noticeRect, imgWoodNoticeButton, imgWoodNoticeButton.rect());
        }
        p.setOpacity(1.0);
        p.setFont(promptFont(17, QFont::Bold));
        p.setPen(QColor(255, 235, 174));
        p.drawText(noticeRect.adjusted(24, 0, -24, -4), Qt::AlignCenter, prompt);
    }

    p.restore();
}

// ============================================================
// 敌人与Boss危害区
// ============================================================

void GameWindow::drawBossHazards(QPainter& p)
{
    if (!gm->boss || !gm->boss->alive) return;

    auto drawEffectFrame = [&](const QPixmap& sheet, const QRectF& target,
                               qreal progress, qreal opacity = 1.0) {
        if (sheet.isNull() || sheet.height() <= 0) return;
        const int frameWidth = sheet.height();
        const int frameCount = qMax(1, sheet.width() / frameWidth);
        const int frame = qBound(0, qFloor(progress * frameCount), frameCount - 1);
        p.save();
        p.setOpacity(opacity);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(target, sheet,
                     QRectF(frame * frameWidth, 0, frameWidth, sheet.height()));
        p.restore();
    };
    auto drawDirectionalEffectFrame = [&](const QPixmap& sheet,
                                          const QPointF& startWorld,
                                          const QPointF& endWorld,
                                          qreal thickness,
                                          qreal progress,
                                          qreal opacity = 1.0) {
        if (sheet.isNull() || sheet.height() <= 0) return;
        QPointF start(startWorld.x() - gm->cameraX, startWorld.y());
        QPointF end(endWorld.x() - gm->cameraX, endWorld.y());
        QPointF delta = end - start;
        qreal length = std::hypot(delta.x(), delta.y());
        if (length < 24.0) return;
        const int frameWidth = sheet.height();
        const int frameCount = qMax(1, sheet.width() / frameWidth);
        const int frame = qBound(0, qFloor(progress * frameCount), frameCount - 1);
        const qreal angle = std::atan2(delta.y(), delta.x()) * 180.0 / 3.14159265358979323846;
        p.save();
        p.setOpacity(opacity);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.translate(start);
        p.rotate(angle);
        p.drawPixmap(QRectF(0, -thickness / 2.0, length, thickness),
                     sheet,
                     QRectF(frame * frameWidth, 0, frameWidth, sheet.height()));
        p.restore();
    };
    auto drawBacklashArcs = [&](const QPointF& center, qreal radius,
                                qreal progress, qreal fade) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        for (int i = 0; i < 9; ++i) {
            const qreal phase = progress * 4.2 + i * 0.74;
            const qreal angle = phase + i * 0.37;
            const qreal inner = radius * (0.18 + 0.04 * (i % 3));
            const qreal outer = radius * (0.74 + 0.10 * std::sin(phase));
            const QPointF a(center.x() + std::cos(angle) * inner,
                            center.y() + std::sin(angle) * inner);
            const QPointF b(center.x() + std::cos(angle + 0.12) * outer,
                            center.y() + std::sin(angle + 0.12) * outer);
            p.setPen(QPen(QColor(150, 246, 255, qRound(190 * fade)),
                          3.0 - (i % 3) * 0.45,
                          Qt::SolidLine,
                          Qt::RoundCap));
            p.drawLine(QLineF(a, b));
        }
        p.restore();
    };

    for (const auto& h : gm->boss->getHazards()) {
        if (!h.active) continue;
        const qreal progress = h.durationMs > 0
            ? qBound<qreal>(0.0, h.elapsedMs / h.durationMs, 0.999)
            : 0.0;

        if (h.type == BossHazardType::BombWarning ||
            h.type == BossHazardType::BombHitbox) {
            const int sx = qRound(h.position.x()) - gm->cameraX;
            const int sy = qRound(h.position.y());
            const qreal pulse = h.type == BossHazardType::BombWarning
                ? 1.0 + 0.10 * ((QDateTime::currentMSecsSinceEpoch() / 95) % 3)
                : 1.32;
            const qreal diameter = qMax(h.rect.width(), h.rect.height()) * pulse;
            if (h.type == BossHazardType::BombWarning) {
                const qreal warnProgress = qBound<qreal>(0.0, progress * 0.55, 0.55);
                drawEffectFrame(imgFiveHeadBombardment,
                                 QRectF(sx - diameter / 2.0, sy - diameter / 2.0,
                                        diameter, diameter),
                                 warnProgress, 0.58);
            }
            else {
                const qreal effectSize = diameter;
                drawEffectFrame(imgFiveHeadBombardment,
                                QRectF(sx - effectSize / 2.0, sy - effectSize / 2.0,
                                       effectSize, effectSize),
                                progress, 0.96);
            }
            continue;
        }

        if (h.type == BossHazardType::SummonMarker) {
            QRectF rect = h.rect;
            rect.translate(-gm->cameraX, 0);
            const qreal loopProgress =
                (QDateTime::currentMSecsSinceEpoch() % 760) / 760.0;
            drawEffectFrame(imgFiveHeadSummonWater,
                            rect.adjusted(-18, -18, 18, 18),
                            loopProgress, 0.86);
            continue;
        }

        if (h.type == BossHazardType::SeaweedZone) {
            const QPointF center(h.position.x() - gm->cameraX, h.position.y());
            const qreal size = h.radius * 2.0;
            const qreal loopProgress =
                (QDateTime::currentMSecsSinceEpoch() % 880) / 880.0;
            // The supplied art has transparent padding.  Compensate per axis
            // so the visible seaweed edge, not the 512px canvas, matches the
            // gameplay radius.
            const qreal visualWidth = size * 1.30;
            const qreal visualHeight = size * 1.16;
            drawEffectFrame(imgSirenSeaweed,
                            QRectF(center.x() - visualWidth / 2.0,
                                   center.y() - visualHeight / 2.0,
                                   visualWidth, visualHeight),
                            loopProgress, 0.72);
            continue;
        }

        if (h.type == BossHazardType::ReefHitbox) {
            QRectF rect = h.rect;
            rect.translate(-gm->cameraX, 0);
            const qreal size = qMax(rect.width(), rect.height()) * 1.1;
            drawEffectFrame(imgSirenReef,
                            QRectF(rect.center().x() - size / 2.0,
                                   rect.center().y() - size / 2.0, size, size),
                            qMin<qreal>(0.999, h.elapsedMs / 900.0), 0.94);
            continue;
        }

        if (h.type == BossHazardType::ResonancePillar) {
            const QPointF center(h.position.x() - gm->cameraX, h.position.y());
            if (!imgSirenResonancePillar.isNull()) {
                const int frameWidth = imgSirenResonancePillar.height();
                const int frameCount = qMax(1, imgSirenResonancePillar.width() / frameWidth);
                const int frame = qBound(0, h.visualStage, frameCount - 1);
                p.save();
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(4, 57, 70, 46));
                p.drawEllipse(center + QPointF(0, 68), 78, 14);
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QColor(117, 227, 232, 46), 1.4));
                p.drawEllipse(center + QPointF(0, 68), 92, 18);
                const QRectF pillarTarget(center.x() - 92, center.y() - 92, 184, 184);
                p.setOpacity(0.94);
                p.drawPixmap(pillarTarget,
                             imgSirenResonancePillar,
                             QRectF(frame * frameWidth, 0, frameWidth,
                                     imgSirenResonancePillar.height()));
                p.setCompositionMode(QPainter::CompositionMode_Screen);
                p.setOpacity(0.07);
                p.drawPixmap(pillarTarget,
                             imgSirenResonancePillar,
                             QRectF(frame * frameWidth, 0, frameWidth,
                                    imgSirenResonancePillar.height()));
                p.restore();
            }
            continue;
        }

        if (h.type == BossHazardType::ResonanceBacklash) {
            const QPointF center(h.position.x() - gm->cameraX, h.position.y());
            const qreal fade = 1.0 - progress;
            if (!imgHitSpark.isNull()) {
                const qreal sparkSize = 156.0 + progress * 68.0;
                drawPixmapCentered(p, imgHitSpark, center,
                                   QSizeF(sparkSize, sparkSize * 0.82),
                                   0.90 * fade);
            }
            drawBacklashArcs(center, h.radius, progress, fade);
            p.save();
            p.setFont(QFont("Microsoft YaHei", 13, QFont::Bold));
            p.setPen(QColor(170, 245, 255, qRound(255 * fade)));
            p.drawText(QRectF(center.x() - 120.0,
                              center.y() - 178.0 - progress * 24.0,
                              240.0, 28.0),
                       Qt::AlignCenter,
                       QStringLiteral("共鸣反噬  -%1").arg(h.damage));
            p.restore();
            continue;
        }

        if (h.type == BossHazardType::SoulSong) {
            const QPointF endWorld = !h.target.isNull() ? h.target : h.rect.center();
            const bool charging = h.damage <= 0;
            const qreal stagger = charging ? h.visualStage * 0.045 : 0.0;
            const qreal growth = charging
                ? qBound<qreal>(0.0,
                    (progress - stagger) / qMax<qreal>(0.1, 1.0 - stagger),
                    1.0)
                : 1.0;
            const qreal easedGrowth =
                growth * growth * (3.0 - 2.0 * growth);
            const qreal currentHalfWidth = charging
                ? 4.0 + (h.radius - 4.0) * easedGrowth
                : h.radius;
            const qreal coreThickness =
                qBound<qreal>(2.2, currentHalfWidth * 0.42, 20.0);
            const qreal textureThickness =
                qBound<qreal>(16.0, currentHalfWidth * 1.55, 68.0);
            const QPointF startScreen(h.position.x() - gm->cameraX, h.position.y());
            const QPointF endScreen(endWorld.x() - gm->cameraX, endWorld.y());
            p.save();
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(QPen(charging
                              ? QColor(110, 236, 255,
                                       qRound(42 + easedGrowth * 64))
                              : QColor(145, 255, 255, 132),
                          coreThickness,
                          Qt::SolidLine,
                          Qt::RoundCap));
            p.drawLine(QLineF(startScreen, endScreen));
            p.restore();
            if (charging) {
                if (!imgSirenSoulSong.isNull()) {
                    drawDirectionalEffectFrame(imgSirenSoulSong,
                                               h.position,
                                               endWorld,
                                               textureThickness,
                                               progress,
                                               0.10 + easedGrowth * 0.30);
                }
                continue;
            }
            if (!imgSirenSoulSong.isNull()) {
                drawDirectionalEffectFrame(imgSirenSoulSong,
                                           h.position,
                                           endWorld,
                                           textureThickness,
                                           progress,
                                           0.62);
            }
            continue;
        }

        if (h.type == BossHazardType::ElegyWarning) {
            const QPointF center(h.position.x() - gm->cameraX, h.position.y());
            const qreal baseSize = h.radius * 2.0;
            if (h.visualStage == 0) {
                const qreal pulse = 0.82 + progress * 0.24;
                drawEffectFrame(imgSirenElegyPull,
                                QRectF(center.x() - baseSize * pulse / 2.0,
                                       center.y() - baseSize * pulse / 2.0,
                                 baseSize * pulse, baseSize * pulse),
                                 qMin<qreal>(0.55, progress * 0.55),
                                 0.48 + progress * 0.18);
            }
            else if (h.visualStage >= 4) {
                const qreal fade = 1.0 - progress;
                drawEffectFrame(imgSirenElegyWave,
                                QRectF(center.x() - baseSize / 2.0,
                                       center.y() - baseSize / 2.0,
                                       baseSize, baseSize),
                                progress,
                                0.62 * fade);
                drawEffectFrame(imgSirenElegyPull,
                                QRectF(center.x() - baseSize * 0.46,
                                       center.y() - baseSize * 0.46,
                                       baseSize * 0.92, baseSize * 0.92),
                                progress,
                                0.42 * fade);
            }
            else {
                const qreal waveSize = baseSize * (0.68 + progress * 0.32);
                const qreal secondarySize = baseSize * (0.86 + progress * 0.14);
                drawEffectFrame(imgSirenElegyPull,
                                QRectF(center.x() - baseSize * 0.46,
                                       center.y() - baseSize * 0.46,
                                       baseSize * 0.92, baseSize * 0.92),
                                progress,
                                0.38);
                drawEffectFrame(imgSirenElegyWave,
                                QRectF(center.x() - waveSize / 2.0,
                                       center.y() - waveSize / 2.0,
                                       waveSize, waveSize),
                                progress,
                                0.82);
                if (h.visualStage >= 2) {
                    drawEffectFrame(imgSirenElegyWave,
                                    QRectF(center.x() - secondarySize / 2.0,
                                           center.y() - secondarySize / 2.0,
                                           secondarySize, secondarySize),
                                    qMin<qreal>(0.999, progress + 0.18),
                                    0.44);
                }
            }
            continue;
        }

        if (h.radius > 0.0) {
            continue;
        }
        else {
            QRectF rect = h.rect;
            rect.translate(-gm->cameraX, 0);
            if ((h.type == BossHazardType::MouthStrike ||
                      h.type == BossHazardType::MeleeHitbox) &&
                     h.damage > 0 && !imgHitSpark.isNull()) {
                drawPixmapCentered(p, imgHitSpark, rect.center(),
                                   QSizeF(qMin<qreal>(96.0, rect.width()),
                                          qMin<qreal>(72.0, rect.height())), 0.9);
            }
        }
    }
}

void GameWindow::drawSharks(QPainter& p)
{
    auto drawAnimatedEnemy = [&](const QPixmap& pixmap, const QRect& target, int frameOffset = 0, bool mirror = false) {
        if (pixmap.isNull()) return false;

        const int frameCount = 4;
        const int frameWidth = pixmap.width() / frameCount;
        const int frame = static_cast<int>(((QDateTime::currentMSecsSinceEpoch() / 140) + frameOffset) % frameCount);
        const QRect source(frame * frameWidth, 0, frameWidth, pixmap.height());
        if (mirror) {
            p.save();
            p.translate(target.right() + 1, target.top());
            p.scale(-1, 1);
            p.drawPixmap(QRect(0, 0, target.width(), target.height()), pixmap, source);
            p.restore();
        }
        else {
            p.drawPixmap(target, pixmap, source);
        }
        return true;
    };
    auto drawBossSheet = [&](const QPixmap& sheet, const QRectF& target,
                             qreal progress, bool loop, qreal opacity = 1.0,
                             bool mirror = false,
                             const QColor& tint = QColor()) {
        if (sheet.isNull() || sheet.height() <= 0) return false;
        const int frameWidth = sheet.height();
        const int frameCount = qMax(1, sheet.width() / frameWidth);
        const qreal normalized = loop
            ? (QDateTime::currentMSecsSinceEpoch() % (frameCount * 120)) /
              static_cast<qreal>(frameCount * 120)
            : qBound<qreal>(0.0, progress, 0.999);
        const int frame = qBound(0, qFloor(normalized * frameCount), frameCount - 1);
        QPixmap framePixmap = sheet.copy(frame * frameWidth, 0,
                                         frameWidth, sheet.height());
        if (tint.isValid() && tint.alpha() > 0) {
            QPainter tintPainter(&framePixmap);
            tintPainter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            tintPainter.fillRect(framePixmap.rect(), tint);
        }
        p.save();
        p.setOpacity(opacity);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (mirror) {
            p.translate(target.right(), target.top());
            p.scale(-1.0, 1.0);
            p.drawPixmap(QRectF(0, 0, target.width(), target.height()), framePixmap,
                         QRectF(framePixmap.rect()));
        }
        else {
            p.drawPixmap(target, framePixmap, QRectF(framePixmap.rect()));
        }
        p.restore();
        return true;
    };
    auto drawInkFrame = [&](const QPointF& worldPos, int frame, const QSize& size, bool mirror, qreal opacity) {
        if (imgOctopusInk.isNull()) return;
        const int frameCount = 4;
        const int frameWidth = imgOctopusInk.width() / frameCount;
        const QRect source(qBound(0, frame, frameCount - 1) * frameWidth, 0,
                           frameWidth, imgOctopusInk.height());
        const QRect target(
            static_cast<int>(worldPos.x()) - gm->cameraX - size.width() / 2,
            static_cast<int>(worldPos.y()) - size.height() / 2,
            size.width(),
            size.height()
        );
        p.save();
        p.setOpacity(opacity);
        if (mirror) {
            p.translate(target.right() + 1, target.top());
            p.scale(-1, 1);
            p.drawPixmap(QRect(0, 0, target.width(), target.height()), imgOctopusInk, source);
        }
        else {
            p.drawPixmap(target, imgOctopusInk, source);
        }
        p.restore();
    };
    auto drawStunBadge = [&](const QPointF& worldPos, QSizeF size) {
        const QPointF screen(worldPos.x() - gm->cameraX, worldPos.y());
        if (!imgShockwaveRing.isNull()) {
            const qreal pulse = 1.0 + 0.06 * ((QDateTime::currentMSecsSinceEpoch() / 110) % 3);
            drawPixmapCentered(p, imgShockwaveRing, QPointF(screen.x(), screen.y() - size.height() * 0.34),
                               QSizeF(size.width() * 0.78 * pulse, size.height() * 0.42 * pulse),
                               0.62);
        }
        p.save();
        p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        p.setPen(QColor(255, 238, 154));
        p.drawText(QRectF(screen.x() - 34, screen.y() - size.height() * 0.65, 68, 18),
                   Qt::AlignCenter, QStringLiteral("眩晕"));
        p.restore();
    };

    // 普通鲨鱼
    for (auto s : gm->sharks) {
        if (!s || !s->alive) continue;
        int screenX = s->x - gm->cameraX;
        if (screenX < -50 || screenX > 1330) continue;

        const int sharkW = Config::GameConfig::SHARK_VISUAL_WIDTH;
        const int sharkH = Config::GameConfig::SHARK_VISUAL_HEIGHT;
        if (!drawAnimatedEnemy(imgShark, QRect(screenX - sharkW / 2, s->y - sharkH / 2, sharkW, sharkH), s->x / 23, s->facingX > 0.0f)) {
            p.setBrush(QColor(80, 80, 200));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - sharkW / 2, s->y - sharkH / 2, sharkW, sharkH);
        }
        p.fillRect(screenX - 20, s->y - sharkH / 2 - 12, 40, 6, QColor(60, 60, 60));
        int bw = qBound(0, (int)(40.0f * s->hp / qMax(1, s->maxHp)), 40);
        p.fillRect(screenX - 20, s->y - sharkH / 2 - 12, bw, 6, QColor(220, 50, 50));
        if (s->isStunned()) drawStunBadge(s->position(), QSizeF(sharkW, sharkH));
    }

    // 剑鱼
    for (auto s : gm->swordfishes) {
        if (!s || !s->alive) continue;
        int screenX = s->x - gm->cameraX;
        if (screenX < -50 || screenX > 1330) continue;

        const int swordfishW = Config::GameConfig::SWORDFISH_VISUAL_WIDTH;
        const int swordfishH = Config::GameConfig::SWORDFISH_VISUAL_HEIGHT;
        if (!drawAnimatedEnemy(imgSwordfish, QRect(screenX - swordfishW / 2, s->y - swordfishH / 2, swordfishW, swordfishH), s->x / 23, s->facingX > 0.0f)) {
            p.setBrush(QColor(50, 200, 200));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - swordfishW / 2, s->y - swordfishH / 2, swordfishW, swordfishH);
        }
        if (s->state == Swordfish::WINDUP) {
            p.setPen(QColor(255, 200, 0));
            p.setFont(QFont("Microsoft YaHei", 10));
            p.drawText(screenX - 15, s->y - 18, "蓄力!");
        }
        p.fillRect(screenX - 20, s->y - swordfishH / 2 - 10, 40, 5, QColor(60, 60, 60));
        int bw2 = qBound(0, (int)(40.0f * s->hp / qMax(1, s->maxHp)), 40);
        p.fillRect(screenX - 20, s->y - swordfishH / 2 - 10, bw2, 5, QColor(220, 50, 50));
        if (s->isStunned()) drawStunBadge(s->position(), QSizeF(swordfishW, swordfishH));
    }

    // 墨鱼
    for (auto o : gm->octopuses) {
        if (!o || !o->alive) continue;
        if (o->hasInkProjectile()) {
            const QPointF velocity = o->inkProjectileDirection();
            drawInkFrame(o->inkProjectilePosition(), o->inkAnimationFrame(), QSize(56, 34),
                         velocity.x() < 0.0, 0.96);
        }
        int screenX = o->x - gm->cameraX;
        if (screenX < -50 || screenX > 1330) continue;

        const int octopusW = Config::GameConfig::OCTOPUS_VISUAL_WIDTH;
        const int octopusH = Config::GameConfig::OCTOPUS_VISUAL_HEIGHT;
        p.save();
        if (o->isInvisible) p.setOpacity(0.18);
        if (!drawAnimatedEnemy(imgOctopus, QRect(screenX - octopusW / 2, o->y - octopusH / 2, octopusW, octopusH), o->x / 23, o->facingX > 0.0f)) {
            p.setBrush(QColor(150, 0, 150));
            p.setPen(Qt::NoPen);
            p.drawEllipse(screenX - octopusW / 2, o->y - octopusH / 2, octopusW, octopusH);
        }
        p.restore();
        if (o->isInkCharging()) {
            const qreal pulse = 0.72 + 0.18 * ((QDateTime::currentMSecsSinceEpoch() / 90) % 2);
            drawInkFrame(QPointF(o->x + o->facingX * 24.0f, o->y - 2.0),
                         0, QSize(38, 30), o->facingX < 0.0f, pulse);
        }
        p.fillRect(screenX - 18, o->y - octopusH / 2 - 10, 36, 5, QColor(60, 60, 60));
        int bw3 = qBound(0, (int)(36.0f * o->hp / qMax(1, o->maxHp)), 36);
        p.fillRect(screenX - 18, o->y - octopusH / 2 - 10, bw3, 5, QColor(220, 50, 50));
        if (o->isStunned()) drawStunBadge(o->position(), QSizeF(octopusW, octopusH));
    }

    for (Enemy* enemy : gm->specialEnemies) {
        if (!enemy || !enemy->alive) continue;
        const int screenX = enemy->x - gm->cameraX;
        if (screenX < -90 || screenX > 1370) continue;

        if (ElectricRay* ray = dynamic_cast<ElectricRay*>(enemy)) {
            if (ray->isPulseCharging() || ray->isPulseVisible()) {
                const qreal pulseOpacity = ray->isPulseVisible()
                    ? 0.92
                    : 0.48 + 0.18 * ((QDateTime::currentMSecsSinceEpoch() / 90) % 2);
                const qreal diameter = ray->pulseRadius() * 2.0;
                const int frameCount = 4;
                const int frameWidth = imgElectricDischarge.width() / frameCount;
                const int frame = qBound(0, ray->pulseAnimationFrame(), frameCount - 1);
                if (!imgElectricDischarge.isNull() && frameWidth > 0) {
                    QRectF source(frame * frameWidth, 0, frameWidth, imgElectricDischarge.height());
                    if (frame >= 2) {
                        const qreal trim = frameWidth * 0.08;
                        source.adjust(trim, trim, -trim, -trim);
                    }
                    p.save();
                    p.setOpacity(pulseOpacity);
                    p.drawPixmap(
                        QRectF(screenX - diameter / 2.0, ray->y - diameter / 2.0,
                               diameter, diameter),
                        imgElectricDischarge,
                        source
                    );
                    p.restore();
                }
            }
            const int visualW = Config::GameConfig::ELECTRIC_RAY_VISUAL_WIDTH;
            const int visualH = Config::GameConfig::ELECTRIC_RAY_VISUAL_HEIGHT;
            drawAnimatedEnemy(
                imgElectricRay,
                QRect(screenX - visualW / 2, ray->y - visualH / 2, visualW, visualH),
                ray->x / 17,
                ray->facingX > 0.0f
            );
        }
        else if (PoisonJellyfish* jelly = dynamic_cast<PoisonJellyfish*>(enemy)) {
            if (jelly->isStingCharging() || jelly->isStingActive()) {
                const int frameCount = 4;
                const int frameWidth = imgJellyfishSting.width() / frameCount;
                const int frame = qBound(0, jelly->stingAnimationFrame(), frameCount - 1);
                const qreal bodyHalf = Config::GameConfig::JELLYFISH_COLLIDER_WIDTH / 2.0;
                const qreal reach = Config::GameConfig::JELLYFISH_STING_REACH;
                const qreal centerX = screenX + jelly->facingX * (bodyHalf + reach / 2.0);
                if (!imgJellyfishSting.isNull() && frameWidth > 0) {
                    p.save();
                    p.setOpacity(jelly->isStingActive() ? 0.96 : 0.72);
                    p.translate(centerX, jelly->y);
                    if (jelly->facingX > 0.0f) p.scale(-1.0, 1.0);
                    p.drawPixmap(
                        QRectF(-reach / 2.0, -Config::GameConfig::JELLYFISH_STING_HEIGHT / 2.0,
                               reach, Config::GameConfig::JELLYFISH_STING_HEIGHT),
                        imgJellyfishSting,
                        QRectF(frame * frameWidth, 0, frameWidth, imgJellyfishSting.height())
                    );
                    p.restore();
                }
            }
            const int visualW = Config::GameConfig::JELLYFISH_VISUAL_WIDTH;
            const int visualH = Config::GameConfig::JELLYFISH_VISUAL_HEIGHT;
            drawAnimatedEnemy(
                imgPoisonJellyfish,
                QRect(screenX - visualW / 2, jelly->y - visualH / 2, visualW, visualH),
                jelly->x / 19,
                jelly->facingX > 0.0f
            );
        }

        const int barWidth = 42;
        const int barY = dynamic_cast<ElectricRay*>(enemy)
            ? enemy->y - Config::GameConfig::ELECTRIC_RAY_VISUAL_HEIGHT / 2 - 10
            : enemy->y - Config::GameConfig::JELLYFISH_VISUAL_HEIGHT / 2 - 10;
        p.fillRect(screenX - barWidth / 2, barY, barWidth, 5,
                   QColor(50, 42, 52, 210));
        const int healthWidth = enemy->maxHp > 0
            ? qBound(0, barWidth * enemy->hp / enemy->maxHp, barWidth)
            : 0;
        p.fillRect(screenX - barWidth / 2, barY, healthWidth, 5,
                   QColor(196, 55, 76));
        if (enemy->isStunned()) {
            const QSizeF stunSize = dynamic_cast<ElectricRay*>(enemy)
                ? QSizeF(Config::GameConfig::ELECTRIC_RAY_VISUAL_WIDTH,
                         Config::GameConfig::ELECTRIC_RAY_VISUAL_HEIGHT)
                : QSizeF(Config::GameConfig::JELLYFISH_VISUAL_WIDTH,
                         Config::GameConfig::JELLYFISH_VISUAL_HEIGHT);
            drawStunBadge(enemy->position(), stunSize);
        }
    }

    // Boss
    if (gm->boss && gm->boss->alive) {
        QPointF secondaryPos;
        int secondaryHp = 0;
        int secondaryMaxHp = 0;
        if (gm->boss->getSecondaryTarget(secondaryPos, secondaryHp, secondaryMaxHp)) {
            const int cloneX = qRound(secondaryPos.x()) - gm->cameraX;
            const int cloneY = qRound(secondaryPos.y());
            if (cloneX >= -60 && cloneX <= 1340) {
                p.setBrush(QColor(120, 40, 180));
                p.setPen(QPen(QColor(230, 180, 255), 2));
                const int cloneW = Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH;
                const int cloneH = Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT;
                p.drawEllipse(cloneX - cloneW / 2, cloneY - cloneH / 2, cloneW, cloneH);
                p.fillRect(cloneX - cloneW / 2, cloneY - cloneH / 2 - 12,
                           cloneW, 6, QColor(60, 60, 60));
                const int cloneBar = secondaryMaxHp > 0
                    ? cloneW * secondaryHp / secondaryMaxHp
                    : 0;
                p.fillRect(cloneX - cloneW / 2, cloneY - cloneH / 2 - 12,
                           cloneBar, 6, QColor(220, 50, 50));
            }
        }

        QPointF companionPos;
        bool companionStunned = false;
        if (gm->boss->getCompanionVisual(companionPos, companionStunned)) {
            const int companionX = qRound(companionPos.x()) - gm->cameraX;
            const int companionY = qRound(companionPos.y());
            const QPixmap& companionSheet = imgSirenPhantomMove.isNull()
                ? imgSirenPhantomStun
                : imgSirenPhantomMove;
            const bool mirrorCompanion = companionPos.x() < gm->playerX();
            const QColor companionTint = companionStunned
                ? QColor(150, 240, 255, 118)
                : QColor();
            drawBossSheet(companionSheet,
                           QRectF(companionX - 68, companionY - 68, 136, 136),
                           companionStunned
                               ? (QDateTime::currentMSecsSinceEpoch() % 800) / 800.0
                               : 0.0,
                           !companionStunned, 0.96,
                           mirrorCompanion,
                           companionTint);
        }

        const int screenX = gm->boss->x - gm->cameraX;
        const int screenY = gm->boss->y;
        if (screenX >= -280 && screenX <= 1560) {
            const bool isPhase2 = gm->boss->state == Boss::PHASE2;
            const BossVisualAction action = gm->boss->visualAction();
            const qreal actionProgress = gm->boss->visualActionProgress();
            const QPixmap* bodySheet = nullptr;
            QSizeF bodySize;
            bool mirrorBoss = false;

            if (gm->boss->kind == BossKind::FiveHeadShark) {
                bodySize = QSizeF(380, 318);
                mirrorBoss = gm->boss->facingX > 0.0f;
                switch (action) {
                case BossVisualAction::Bite: bodySheet = &imgFiveHeadBite; break;
                case BossVisualAction::Cast: bodySheet = &imgFiveHeadCast; break;
                case BossVisualAction::Hit: bodySheet = &imgFiveHeadHit; break;
                case BossVisualAction::Death: bodySheet = &imgFiveHeadDeath; break;
                default: bodySheet = &imgFiveHeadIdle; break;
                }
                if (action == BossVisualAction::Summon) {
                    drawBossSheet(imgFiveHeadSummonWater,
                                  QRectF(screenX - 175, screenY - 175, 350, 350),
                                  actionProgress, false, 0.82, mirrorBoss);
                }
            }
            else if (gm->boss->kind == BossKind::Siren) {
                bodySize = QSizeF(340, 382);
                mirrorBoss = gm->boss->facingX > 0.0f;
                switch (action) {
                case BossVisualAction::Hit: bodySheet = &imgSirenIdle; break;
                case BossVisualAction::PhaseTransition: bodySheet = &imgSirenPhaseTransition; break;
                case BossVisualAction::SoulSongWindup: bodySheet = &imgSirenSoulSongWindup; break;
                case BossVisualAction::SoulSong: bodySheet = &imgSirenSoulSongWindup; break;
                case BossVisualAction::ElegyWindup: bodySheet = &imgSirenElegyWindup; break;
                case BossVisualAction::Elegy: bodySheet = &imgSirenElegyWindup; break;
                case BossVisualAction::Death: bodySheet = &imgSirenDeath; break;
                default: bodySheet = &imgSirenIdle; break;
                }
            }

            if (bodySize.isEmpty()) bodySize = QSizeF(110, 80);
            QRectF bodyRect(screenX - bodySize.width() / 2.0,
                            screenY - bodySize.height() / 2.0,
                             bodySize.width(), bodySize.height());
            const bool sharkEnraged = isPhase2 &&
                gm->boss->kind == BossKind::FiveHeadShark;
            QColor bodyTint;
            if (sharkEnraged) {
                bodyTint = QColor(220, 28, 28, 132);
            }
            else if (gm->boss->kind == BossKind::Siren &&
                     action == BossVisualAction::Hit) {
                bodyTint = QColor(170, 246, 255, 136);
            }
            qreal bodyOpacity = 1.0;
            if (action == BossVisualAction::Death) {
                const qreal t = qBound<qreal>(0.0, actionProgress, 1.0);
                const qreal sink = 20.0 * t * t;
                const qreal swell = 1.0 + 0.045 * (1.0 - std::abs(t * 2.0 - 1.0));
                const QPointF center = bodyRect.center() + QPointF(0.0, sink);
                bodyRect = QRectF(center.x() - bodySize.width() * swell / 2.0,
                                  center.y() - bodySize.height() * swell / 2.0,
                                  bodySize.width() * swell,
                                  bodySize.height() * swell);
                bodyOpacity = t > 0.78
                    ? qBound<qreal>(0.18, 1.0 - (t - 0.78) / 0.22, 1.0)
                    : 1.0;
            }
            if (gm->boss->kind == BossKind::Siren &&
                isPhase2 &&
                action != BossVisualAction::Death &&
                !imgSirenImmunity.isNull()) {
                drawBossSheet(imgSirenImmunity,
                              bodyRect.adjusted(-46, -44, 46, 44),
                              actionProgress,
                              true,
                              0.56,
                              mirrorBoss);
            }
            const bool loopBody = action == BossVisualAction::Idle ||
                                  action == BossVisualAction::Summon;
            bool bossDrawn = false;
            if (action == BossVisualAction::Death &&
                gm->boss->kind == BossKind::FiveHeadShark &&
                bodySheet) {
                const qreal intro = qBound<qreal>(0.0, actionProgress / 0.16, 1.0);
                if (intro < 1.0 && !imgFiveHeadHit.isNull()) {
                    drawBossSheet(imgFiveHeadHit, bodyRect,
                                  0.72, false, (1.0 - intro) * 0.86,
                                  mirrorBoss);
                }
                bossDrawn = drawBossSheet(*bodySheet, bodyRect,
                                           actionProgress, false,
                                           bodyOpacity * qMax<qreal>(0.18, intro),
                                           mirrorBoss, bodyTint);
            }
            else if (bodySheet) {
                bossDrawn = drawBossSheet(*bodySheet, bodyRect,
                                            actionProgress, loopBody, bodyOpacity,
                                           mirrorBoss, bodyTint);
            }
            if (!bossDrawn) {
                drawAnimatedEnemy(imgShark, bodyRect.toRect(), screenX / 23);
            }

            if (!gm->boss->isDying() && gm->boss->isStunnedByShock() && !imgShockwaveRing.isNull()) {
                const qreal pulse = 1.0 + 0.08 * ((QDateTime::currentMSecsSinceEpoch() / 100) % 3);
                drawPixmapCentered(p, imgShockwaveRing,
                                   QPointF(screenX, screenY),
                                   QSizeF(bodyRect.width() * pulse,
                                          bodyRect.height() * 0.82 * pulse),
                                   0.72);
                p.save();
                p.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
                p.setPen(QColor(255, 238, 154));
                p.drawText(QRect(screenX - 48, qRound(bodyRect.top()) - 68, 96, 22),
                           Qt::AlignCenter, QStringLiteral("眩晕"));
                p.restore();
            }

            if (!gm->boss->isDying()) {
            const int healthBarWidth =
                gm->boss->kind == BossKind::FiveHeadShark ? 190 : 150;
            const int healthBarY = qRound(bodyRect.top()) - 14;
            p.fillRect(screenX - healthBarWidth / 2, healthBarY,
                       healthBarWidth, 9, QColor(50, 35, 32, 225));
            const int healthWidth = gm->boss->maxHp > 0
                ? qBound(0, healthBarWidth * gm->boss->hp / gm->boss->maxHp,
                         healthBarWidth)
                : 0;
            p.fillRect(screenX - healthBarWidth / 2, healthBarY,
                       healthWidth, 9, QColor(210, 48, 58));

            const QString phaseText = isPhase2
                ? QStringLiteral("二阶段")
                : QStringLiteral("一阶段");
            p.setPen(QColor(255, 236, 180));
            p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            p.drawText(QRect(screenX - 90, healthBarY - 22, 180, 18),
                       Qt::AlignCenter,
                       QStringLiteral("%1  %2")
                           .arg(bossDisplayName(gm->boss->kind), phaseText));
            if (isPhase2 && gm->boss->kind == BossKind::FiveHeadShark) {
                p.setPen(QColor(255, 100, 100));
                p.setFont(QFont("Microsoft YaHei", 10));
                p.drawText(QRect(screenX - 60, healthBarY + 10, 120, 18),
                           Qt::AlignCenter, QStringLiteral("狂暴"));
            }
            }
        }
    }
}

// ============================================================
// 玩家
// ============================================================

void GameWindow::drawPlayer(QPainter& p)
{
    int screenX = gm->playerX() - gm->cameraX;
    int screenY = gm->playerY();
    Player& player = Player::instance();

    Weapon* currentWeapon = InventorySystem::instance().currentWeapon();
    if (currentWeapon && !currentWeapon->isBroken()) {
        int displayRange = currentWeapon->getRange();
        if (currentWeapon->canFish() && !currentWeapon->canAttack()) {
            displayRange += Config::GameConfig::FISH_INTERACTION_RADIUS;
        }
        drawPixmapCentered(p, imgWeaponRangeRing, QPointF(screenX, screenY),
                           QSizeF(displayRange * 2.0, displayRange * 2.0), 0.34);
    }

    int weaponIndex = 0;
    if (currentWeapon) {
        const std::string type = currentWeapon->getTypeCode();
        if (type == "Net") weaponIndex = 1;
        else if (type == "Harpoon") weaponIndex = 2;
        else if (type == "Pistol") weaponIndex = 3;
        else if (type == "Shotgun") weaponIndex = 4;
    }
    const int dirIndex = qBound(0, player.facingDirection(), 3);
    const bool useBoostSprite = player.isDashing() || player.isBoosting();
    const QPixmap& sprite = useBoostSprite && !imgPlayerBoost[weaponIndex][dirIndex].isNull()
        ? imgPlayerBoost[weaponIndex][dirIndex]
        : imgPlayerMove[weaponIndex][dirIndex];

    if (!sprite.isNull()) {
        const QRect source(0, 0, sprite.width(), sprite.height());
        QSize maxSize = (dirIndex == 0 || dirIndex == 1) ? QSize(72, 96) : QSize(100, 82);
        QSize targetSize = source.size();
        targetSize.scale(maxSize, Qt::KeepAspectRatio);
        const QRect target(screenX - targetSize.width() / 2,
                           screenY - targetSize.height() / 2,
                           targetSize.width(),
                           targetSize.height());
        p.drawPixmap(target, sprite, source);
    }
    else if (!imgBoat.isNull()) {
        p.drawPixmap(screenX - 30, screenY - 15, 60, 30, imgBoat);
    }
    else {
        p.setBrush(QColor(240, 240, 240));
        p.setPen(QPen(QColor(100, 100, 100), 1));
        p.drawRect(screenX - 20, screenY - 10, 40, 20);
    }

    // 绘制Dash残影/特效
    if (Player::instance().isDashing()) {
        drawPixmapCentered(p, imgShockwaveRing, QPointF(screenX, screenY + 4),
                            QSizeF(120, 58), 0.46);
    }

    if (player.isStunned()) {
        drawPixmapCentered(p, imgShockwaveRing, QPointF(screenX, screenY - 30),
                           QSizeF(92, 36), 0.66);
        p.save();
        p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        p.setPen(QColor(255, 238, 154));
        p.drawText(QRect(screenX - 36, screenY - 54, 72, 18), Qt::AlignCenter,
                   QStringLiteral("眩晕"));
        p.restore();
    }

    if (player.isDamageFlashing() && !imgHitSpark.isNull()) {
        const qreal t = player.damageFlashRatio();
        drawPixmapCentered(p, imgHitSpark, QPointF(screenX, screenY - 2),
                           QSizeF(86 + 24 * t, 64 + 16 * t), 0.34 + 0.46 * t);
    }

}

void GameWindow::drawShockWaveEffect(QPainter& p)
{
    Player& player = Player::instance();
    if (!player.isShockActive() || imgShockwaveRing.isNull()) {
        return;
    }

    QRectF area = player.shockArea();
    area.translate(-gm->cameraX, 0);
    const qreal progress = player.shockEffectProgress();
    const qreal scale = 0.28 + progress * 0.90;
    const qreal opacity = 0.82 * (1.0 - progress);
    drawPixmapCentered(p, imgShockwaveRing, area.center(),
                        QSizeF(area.width() * scale, area.height() * scale),
                        opacity);
    const qreal innerScale = 0.18 + progress * 0.52;
    drawPixmapCentered(p, imgShockwaveRing, area.center(),
                       QSizeF(area.width() * innerScale, area.height() * innerScale),
                       qMax<qreal>(0.0, opacity * 0.58));
}

void GameWindow::drawAttackProjectiles(QPainter& p)
{
    if (attackProjectiles.isEmpty()) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    for (const auto& shot : attackProjectiles) {
        const int travelMs = qMax(1, shot.travelMs);
        const qreal progress = qBound(0.0, static_cast<qreal>(shot.ageMs) / travelMs, 1.0);
        const qreal trailProgress = qMax<qreal>(0.0, progress - 0.22);

        const QPointF currentWorld(
            shot.startWorld.x() + (shot.endWorld.x() - shot.startWorld.x()) * progress,
            shot.startWorld.y() + (shot.endWorld.y() - shot.startWorld.y()) * progress
        );
        const QPointF trailWorld(
            shot.startWorld.x() + (shot.endWorld.x() - shot.startWorld.x()) * trailProgress,
            shot.startWorld.y() + (shot.endWorld.y() - shot.startWorld.y()) * trailProgress
        );

        const QPointF currentScreen(currentWorld.x() - gm->cameraX, currentWorld.y());
        const QPointF trailScreen(trailWorld.x() - gm->cameraX, trailWorld.y());
        const QPointF muzzleScreen(shot.startWorld.x() - gm->cameraX, shot.startWorld.y());

        if (shot.kind == AttackProjectileKind::Harpoon) {
            const qreal dx = shot.endWorld.x() - shot.startWorld.x();
            const qreal dy = shot.endWorld.y() - shot.startWorld.y();
            const qreal angle = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
            const qreal fade = shot.ageMs <= travelMs
                ? 1.0
                : qBound(0.0, 1.0 - static_cast<qreal>(shot.ageMs - travelMs) /
                    qMax(1, shot.lifetimeMs - travelMs), 1.0);

            if (!imgHarpoonProjectile.isNull()) {
                p.save();
                p.setOpacity(fade);
                p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                p.translate(currentScreen);
                p.rotate(angle);

                const qreal targetW = 108.0;
                const qreal targetH = targetW * imgHarpoonProjectile.height() /
                    qMax(1, imgHarpoonProjectile.width());
                const QRectF target(-targetW, -targetH / 2.0, targetW, targetH);
                p.drawPixmap(target, imgHarpoonProjectile, QRectF(imgHarpoonProjectile.rect()));
                p.restore();
            }
            continue;
        }

        const qreal dx = shot.endWorld.x() - shot.startWorld.x();
        const qreal dy = shot.endWorld.y() - shot.startWorld.y();
        const qreal angle = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
        const qreal fade = qBound(0.0, 1.0 - static_cast<qreal>(shot.ageMs) / shot.lifetimeMs, 1.0);
        const QSizeF bulletSize(42 + shot.radius * 4, 18 + shot.radius * 2);
        drawPixmapCentered(p, imgMuzzleFlash, currentScreen, bulletSize, 0.54 + fade * 0.34, angle);

        if (shot.ageMs < 55) {
            const qreal flashFade = qBound(0.0, 1.0 - static_cast<qreal>(shot.ageMs) / 55.0, 1.0);
            drawPixmapCentered(p, imgMuzzleFlash, muzzleScreen,
                               QSizeF(62 + shot.radius * 5, 28 + shot.radius * 3),
                               0.72 * flashFade, angle);
        }
    }

    p.restore();
}

void GameWindow::updateAttackProjectiles()
{
    for (auto& shot : attackProjectiles) {
        shot.ageMs += 16;
    }

    attackProjectiles.erase(
        std::remove_if(
            attackProjectiles.begin(),
            attackProjectiles.end(),
            [](const AttackProjectileEffect& shot) {
                return shot.ageMs >= shot.lifetimeMs;
            }
        ),
        attackProjectiles.end()
    );
}

void GameWindow::drawHitFeedbacks(QPainter& p)
{
    if (hitFeedbacks.isEmpty()) return;

    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (const auto& hit : hitFeedbacks) {
        const qreal t = qBound(0.0, static_cast<qreal>(hit.ageMs) / qMax(1, hit.lifetimeMs), 1.0);
        const qreal size = 54.0 + t * 26.0;
        const QPointF screen(hit.worldPos.x() - gm->cameraX, hit.worldPos.y());
        p.setPen(QPen(QColor(156, 244, 255, qRound(210 * (1.0 - t))),
                      3.0 - t * 1.4, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(screen, 18.0 + t * 34.0, 12.0 + t * 22.0);
        drawPixmapCentered(p, imgHitSpark, screen, QSizeF(size, size * 0.82), 1.0 - t * 0.75);
    }
    p.restore();
}

void GameWindow::updateHitFeedbacks()
{
    for (auto& hit : hitFeedbacks) {
        hit.ageMs += 16;
    }

    hitFeedbacks.erase(
        std::remove_if(
            hitFeedbacks.begin(),
            hitFeedbacks.end(),
            [](const HitFeedbackEffect& hit) {
                return hit.ageMs >= hit.lifetimeMs;
            }
        ),
        hitFeedbacks.end()
    );
}

void GameWindow::spawnHitFeedback(const QPointF& worldPos)
{
    HitFeedbackEffect effect;
    effect.worldPos = worldPos;
    hitFeedbacks.append(effect);
}

void GameWindow::showFloatingNotice(const QString& title, const QString& body)
{
    floatingNotice.title = title;
    floatingNotice.body = body;
    floatingNotice.ageMs = 0;
    floatingNotice.lifetimeMs = 2400;
    floatingNotice.active = true;
}

void GameWindow::updateFloatingNotice()
{
    if (!floatingNotice.active) return;
    floatingNotice.ageMs += 16;
    if (floatingNotice.ageMs >= floatingNotice.lifetimeMs) {
        floatingNotice.active = false;
    }
}

void GameWindow::notifyWeaponBrokenIfNeeded(const Weapon* weapon, bool wasBroken)
{
    if (!weapon || wasBroken || !weapon->isBroken()) return;

    showFloatingNotice(QStringLiteral("\u88c5\u5907\u635f\u574f"),
                       QStringLiteral("%1 \u8010\u4e45\u5df2\u7528\u5c3d\uff0c\u56de\u6e2f\u6216\u4f7f\u7528\u4fee\u7406\u5de5\u5177\u540e\u518d\u51fa\u624b\u3002")
                       .arg(QString::fromStdString(weapon->getName())));
}

bool GameWindow::isGunWeapon(const Weapon* weapon) const
{
    if (!weapon) return false;
    const std::string type = weapon->getTypeCode();
    return type == "Pistol" || type == "Shotgun";
}

bool GameWindow::isHarpoonWeapon(const Weapon* weapon) const
{
    return weapon && weapon->getTypeCode() == "Harpoon";
}

void GameWindow::spawnGunProjectiles(const QPointF& targetWorld, const Weapon* weapon)
{
    if (!isGunWeapon(weapon)) return;

    const QPointF startWorld = Player::instance().worldPos();
    QVector2D dir(targetWorld - startWorld);
    if (dir.isNull()) {
        dir = QVector2D(1.0f, 0.0f);
    }
    dir.normalize();

    const QPointF dirPoint = dir.toPointF();
    const qreal dx = targetWorld.x() - startWorld.x();
    const qreal dy = targetWorld.y() - startWorld.y();
    const qreal requestedDistance = std::sqrt(dx * dx + dy * dy);
    const qreal travelDistance = qMin<qreal>(requestedDistance, weapon->getRange());
    const QPointF baseEnd(
        startWorld.x() + dirPoint.x() * travelDistance,
        startWorld.y() + dirPoint.y() * travelDistance
    );

    const std::string type = weapon->getTypeCode();
    if (type == "Shotgun") {
        const QVector2D side(-dir.y(), dir.x());
        const QPointF sidePoint = side.toPointF();
        const int offsets[] = { -34, -17, 0, 17, 34 };
        for (int offset : offsets) {
            AttackProjectileEffect pellet;
            pellet.startWorld = startWorld;
            pellet.endWorld = QPointF(baseEnd.x() + sidePoint.x() * offset,
                                      baseEnd.y() + sidePoint.y() * offset);
            pellet.lifetimeMs = 115;
            pellet.travelMs = pellet.lifetimeMs;
            pellet.radius = 3;
            pellet.color = QColor(255, 190, 85);
            attackProjectiles.append(pellet);
        }
        return;
    }

    AttackProjectileEffect bullet;
    bullet.startWorld = startWorld;
    bullet.endWorld = baseEnd;
    bullet.lifetimeMs = 150;
    bullet.travelMs = bullet.lifetimeMs;
    bullet.radius = 4;
    bullet.color = QColor(255, 235, 130);
    attackProjectiles.append(bullet);
}

void GameWindow::spawnHarpoonProjectile(const QPointF& targetWorld, const Weapon* weapon)
{
    if (!isHarpoonWeapon(weapon)) return;

    const QPointF startWorld = Player::instance().worldPos();
    const QPointF clampedEnd = clampedProjectileEnd(startWorld, targetWorld, weapon->getRange());
    const QLineF throwLine(startWorld, clampedEnd);

    QPointF impact = clampedEnd;
    bool foundImpact = false;

    auto tryHitbox = [&](const QRectF& rawHitbox) {
        if (rawHitbox.isEmpty()) return;
        bool hit = false;
        const qreal pad = Config::HARPOON_PROJECTILE_HIT_PADDING;
        const QRectF visualHitbox = rawHitbox.adjusted(-pad, -pad, pad, pad);
        const QPointF candidate = firstLineRectIntersection(throwLine, visualHitbox, hit);
        if (!hit) return;

        if (!foundImpact ||
            QLineF(startWorld, candidate).length() < QLineF(startWorld, impact).length()) {
            impact = candidate;
            foundImpact = true;
        }
    };

    if (gm && gm->boss) {
        QRectF hitbox;
        QPointF secondaryPos;
        int secondaryHp = 0;
        int secondaryMaxHp = 0;
        if (gm->boss->getSecondaryTarget(secondaryPos, secondaryHp, secondaryMaxHp)) {
            hitbox = QRectF(
                secondaryPos.x() - Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH / 2.0,
                secondaryPos.y() - Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT / 2.0,
                Config::GameConfig::TALI_CLONE_COLLIDER_WIDTH,
                Config::GameConfig::TALI_CLONE_COLLIDER_HEIGHT
            );
        }
        else if (!gm->boss->isInvulnerable()) {
            hitbox = gm->boss->collider();
        }
        tryHitbox(hitbox);
    }

    if (gm) {
        for (auto* enemy : gm->sharks) {
            if (enemy) tryHitbox(enemy->collider());
        }
        for (auto* enemy : gm->swordfishes) {
            if (enemy) tryHitbox(enemy->collider());
        }
        for (auto* enemy : gm->octopuses) {
            if (enemy) tryHitbox(enemy->collider());
        }
    }

    AttackProjectileEffect harpoon;
    harpoon.kind = AttackProjectileKind::Harpoon;
    harpoon.startWorld = startWorld;
    harpoon.endWorld = impact;
    harpoon.lifetimeMs = 280;
    harpoon.travelMs = 125;
    harpoon.radius = 5;
    harpoon.color = QColor(218, 220, 205);
    attackProjectiles.append(harpoon);
}

void GameWindow::saveVictoryHighScore()
{
    if (victoryScoreSaved || !gm) return;

    Player& pl = Player::instance();
    gm->fileManager.saveHighScoreByStats(
        "Captain",
        pl.distance,
        gm->killCount,
        pl.fishCaught,
        pl.fishTotalValue,
        pl.gameSeconds,
        qMin(gm->stage, Config::GameConfig::STAGE_COUNT),
        pl.coins,
        pl.durability(),
        pl.stamina()
    );
    victoryScoreSaved = true;
}

// ============================================================
// 顶部信息栏
// ============================================================

void GameWindow::drawHUD(QPainter& p)
{
    Player& pl = Player::instance();
    if (!imgHudTopStatusBar.isNull()) {
        p.save();
        p.scale(width() / 1280.0, height() / 720.0);

        InventorySystem& inv = InventorySystem::instance();
        Weapon* currentWeapon = inv.currentWeapon();

        auto ratio = [](int value, int maxValue) {
            if (maxValue <= 0) return 0.0;
            return std::clamp(static_cast<double>(value) / maxValue, 0.0, 1.0);
        };
        auto drawPixmapFit = [&](const QPixmap& pixmap, const QRect& rect) {
            if (pixmap.isNull()) return;
            QSize size = pixmap.size();
            size.scale(rect.size(), Qt::KeepAspectRatio);
            QRect target(QPoint(rect.x() + (rect.width() - size.width()) / 2,
                                rect.y() + (rect.height() - size.height()) / 2), size);
            p.drawPixmap(target, pixmap);
        };
        auto drawText = [&](const QRect& rect, const QString& text, int pixelSize,
                            const QColor& color, int flags = Qt::AlignCenter) {
            QFont font("Microsoft YaHei");
            font.setPixelSize(pixelSize);
            font.setWeight(QFont::Bold);
            p.setFont(font);
            p.setPen(QColor(40, 20, 8, 180));
            p.drawText(rect.translated(2, 2), flags, text);
            p.setPen(color);
            p.drawText(rect, flags, text);
        };
        auto weaponIcon = [&](const Weapon* weapon) -> const QPixmap& {
            static QPixmap empty;
            if (!weapon) return imgIconWeaponRod;
            const std::string type = weapon->getTypeCode();
            if (type == "Net") return imgIconWeaponNet;
            if (type == "Harpoon") return imgIconWeaponHarpoon;
            if (type == "Pistol") return imgIconWeaponPistol;
            if (type == "Shotgun") return imgIconWeaponShotgun;
            if (type == "Rod") return imgIconWeaponRod;
            return empty;
        };
        auto weatherName = []() {
            WeatherSystem& weather = WeatherSystem::instance();
            switch (weather.currentWeather()) {
            case WeatherType::SUNNY: return QString("晴朗");
            case WeatherType::FOG: return QString("大雾");
            case WeatherType::STORM:
                return weather.rainLevel() == 1
                    ? QString("小雨")
                    : (weather.rainLevel() == 2 ? QString("中雨") : QString("暴雨"));
            }
            return QString("未知");
        };

        QRect topBar(150, 8, 980, 64);
        p.drawPixmap(topBar, imgHudTopStatusBar);
        drawText(QRect(250, 14, 108, 28), "渔 途", 20, QColor(255, 205, 85));
        drawText(QRect(254, 42, 100, 18), QString("第 %1 关").arg(gm->stage), 13, QColor(245, 220, 150));

        auto drawStatusBar = [&](const QRect& rect, const QPixmap& fill, int value, int maxValue) {
            p.setPen(QPen(QColor(52, 28, 10), 2));
            p.setBrush(QColor(25, 18, 12, 190));
            p.drawRoundedRect(rect, 4, 4);
            QRect fillRect = rect.adjusted(3, 3, -3, -3);
            const int fillWidth = qRound(fillRect.width() * ratio(value, maxValue));
            if (fillWidth > 0 && !fill.isNull()) {
                QRect target(fillRect.x(), fillRect.y(), fillWidth, fillRect.height());
                QRect source(0, 0, qRound(fill.width() * ratio(value, maxValue)), fill.height());
                p.drawPixmap(target, fill, source);
            }
        };

        drawPixmapFit(imgHudIconHeart, QRect(470, 20, 30, 30));
        drawText(QRect(506, 17, 44, 20), "耐久", 13, QColor(245, 225, 170), Qt::AlignLeft | Qt::AlignVCenter);
        drawText(QRect(552, 17, 64, 20), QString("%1/%2").arg(pl.durability()).arg(pl.maxDurability), 12, QColor(255, 244, 210), Qt::AlignCenter);
        drawStatusBar(QRect(506, 43, 112, 12), imgHudHealthFill, pl.durability(), pl.maxDurability);

        drawPixmapFit(imgHudIconLightning, QRect(660, 20, 30, 30));
        drawText(QRect(694, 17, 32, 20), "体力", 12, QColor(245, 225, 170), Qt::AlignLeft | Qt::AlignVCenter);
        drawText(QRect(722, 17, 42, 20), QString("%1/%2").arg(pl.stamina()).arg(pl.maxStamina), 9, QColor(255, 244, 210), Qt::AlignCenter);
        drawStatusBar(QRect(690, 43, 74, 12), imgHudStaminaFill, pl.stamina(), pl.maxStamina);

        const QRect shockPanel(174, 654, 202, 42);
        if (!imgWoodNoticeButton.isNull()) {
            p.drawPixmap(shockPanel, imgWoodNoticeButton, imgWoodNoticeButton.rect());
        }
        drawPixmapFit(imgHudIconLightning, QRect(184, 661, 26, 26));
        const qreal shockRatio = pl.shockChargeRatio();
        if (!imgSirenFocusMeter.isNull()) {
            constexpr int frameCount = 5;
            const int frameWidth = imgSirenFocusMeter.width() / frameCount;
            const int frame = qBound(0, qFloor(shockRatio * frameCount),
                                     frameCount - 1);
            p.drawPixmap(QRect(214, 672, 150, 18),
                         imgSirenFocusMeter,
                         QRect(frame * frameWidth, 0,
                               frameWidth, imgSirenFocusMeter.height()));
        }
        drawText(QRect(216, 656, 146, 17),
                 shockRatio >= 1.0 ? QStringLiteral("E 震荡波 就绪")
                                   : QStringLiteral("E 震荡波 %1%").arg(qRound(shockRatio * 100.0)),
                 10, QColor(255, 232, 170));

        drawPixmapFit(imgHudIconCoin, QRect(796, 20, 28, 28));
        drawText(QRect(824, 17, 46, 18), "金币", 12, QColor(245, 225, 170));
        drawText(QRect(824, 39, 46, 18), coinDisplayText(pl), 12, QColor(255, 244, 210));

        drawPixmapFit(imgHudIconFish, QRect(900, 20, 28, 28));
        drawText(QRect(932, 17, 50, 18), "鱼获", 12, QColor(245, 225, 170));
        drawText(QRect(932, 39, 50, 18), QString::number(pl.fishCaught), 12, QColor(255, 244, 210));

        const QPixmap& weatherIcon = WeatherSystem::instance().currentWeather() == WeatherType::SUNNY
            ? imgHudIconSun
            : (WeatherSystem::instance().currentWeather() == WeatherType::STORM ? imgHudIconLightning : imgHudIconCompass);
        drawPixmapFit(weatherIcon, QRect(1018, 20, 30, 30));
        drawText(QRect(1050, 17, 46, 20), "天气", 13, QColor(245, 225, 170));
        drawText(QRect(1050, 40, 46, 20), weatherName(), 12, QColor(255, 244, 210));

        QRect equipmentPanel(18, 548, 145, 150);
        p.drawPixmap(equipmentPanel, imgHudEquipmentPanel);
        drawText(QRect(50, 558, 82, 20), "当前装备", 13, QColor(255, 225, 150));
        const QString weaponName = currentWeapon ? QString::fromStdString(currentWeapon->getName()) : QString("无");
        QFont equipmentNameFont("Microsoft YaHei");
        equipmentNameFont.setPixelSize(11);
        equipmentNameFont.setWeight(QFont::Bold);
        const QString visibleWeaponName = QFontMetrics(equipmentNameFont)
            .elidedText(weaponName, Qt::ElideRight, 112);
        drawText(QRect(33, 578, 114, 18), visibleWeaponName, 11, QColor(255, 235, 178));
        drawPixmapFit(weaponIcon(currentWeapon), QRect(50, 596, 80, 52));
        const QString durabilityText = !currentWeapon
            ? QStringLiteral("耐久 --")
            : (currentWeapon->isInfiniteDurability()
                ? QStringLiteral("耐久 无限")
                : QStringLiteral("耐久 %1/%2")
                    .arg(currentWeapon->getCurrentDur())
                    .arg(currentWeapon->getMaxDur()));
        drawText(QRect(31, 648, 118, 18), durabilityText, 10, QColor(245, 214, 145));

        QRect minimapPanel(1072, 105, 185, 168);
        p.drawPixmap(minimapPanel, imgHudMinimapPanel);
        drawText(QRect(1122, 112, 86, 20), "航海图", 13, QColor(255, 225, 150));
        QRect mapArea(1098, 140, 134, 96);
        p.setPen(QPen(QColor(98, 160, 170, 120), 1));
        for (int gx = mapArea.left() + 33; gx < mapArea.right(); gx += 33) p.drawLine(gx, mapArea.top(), gx, mapArea.bottom());
        for (int gy = mapArea.top() + 24; gy < mapArea.bottom(); gy += 24) p.drawLine(mapArea.left(), gy, mapArea.right(), gy);
        drawText(QRect(1154, 140, 20, 18), "N", 13, QColor(255, 226, 135));
        drawText(QRect(1154, 216, 20, 18), "S", 13, QColor(255, 226, 135));
        drawText(QRect(1101, 178, 20, 18), "W", 13, QColor(255, 226, 135));
        drawText(QRect(1210, 178, 20, 18), "E", 13, QColor(255, 226, 135));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 205, 65));
        const auto& stageCfg = Config::GameConfig::stageConfig(gm->stage);
        const int mapStageStart = Config::GameConfig::stageStartDistance(gm->stage);
        const int mapStageEnd = stageCfg.targetDistance;
        const int mapStageSpan = qMax(1, mapStageEnd - mapStageStart);
        const int mapStageDistance = qBound(0, pl.distance - mapStageStart, mapStageSpan);
        const int shipX = mapArea.left() + qBound(16, mapStageDistance * mapArea.width() / mapStageSpan, mapArea.width() - 16);
        QPoint shipPoints[3] = { QPoint(shipX, 174), QPoint(shipX - 8, 193), QPoint(shipX + 8, 193) };
        p.drawPolygon(shipPoints, 3);
        p.setBrush(QColor(245, 70, 55));
        p.drawEllipse(mapArea.left() + 92, mapArea.top() + 36, 6, 6);
        p.setBrush(QColor(70, 210, 230));
        p.drawEllipse(mapArea.left() + 44, mapArea.top() + 58, 5, 5);

        QRect logPanel(1072, 286, 185, 172);
        p.drawPixmap(logPanel, imgHudLogPanel);
        drawText(QRect(1122, 294, 86, 20), "航海日志", 13, QColor(255, 225, 150));
        QFont logFont("Microsoft YaHei");
        logFont.setPixelSize(10);
        logFont.setWeight(QFont::Bold);
        p.setFont(logFont);
        p.setPen(QColor(78, 40, 16));
        const int sec = pl.gameSeconds;
        const QString timeText = QString("%1:%2").arg(sec / 60, 2, 10, QChar('0')).arg(sec % 60, 2, 10, QChar('0'));
        p.drawText(QRect(1094, 326, 140, 18), Qt::AlignLeft | Qt::AlignVCenter, QString("[%1] 第 %2 关海域").arg(timeText).arg(gm->stage));
        p.drawText(QRect(1094, 352, 140, 18), Qt::AlignLeft | Qt::AlignVCenter, QString("距离 %1m").arg(pl.distance));
        p.drawText(QRect(1094, 378, 140, 18), Qt::AlignLeft | Qt::AlignVCenter, QString("鱼获 %1  击败 %2").arg(pl.fishCaught).arg(gm->killCount));
        p.setPen(QColor(170, 50, 34));
        QString objectiveText;
        if (gm->boss && gm->boss->alive) {
            switch (gm->boss->kind) {
            case BossKind::FiveHeadShark:
                objectiveText = QStringLiteral("击败五头鲨");
                break;
            case BossKind::TaliMonster:
                objectiveText = QStringLiteral("击败塔里怪物");
                break;
            case BossKind::Siren:
                objectiveText = QStringLiteral("击败塞壬");
                break;
            }
        }
        else {
            const int remainingDistance = qMax(0, stageCfg.targetDistance - pl.distance);
            objectiveText = stageCfg.hasBoss
                ? QStringLiteral("距 Boss %1m").arg(remainingDistance)
                : QStringLiteral("距终点 %1m").arg(remainingDistance);
        }
        p.drawText(QRect(1094, 404, 150, 18), Qt::AlignLeft | Qt::AlignVCenter, objectiveText);

        QRect hotbar(390, 626, 500, 78);
        p.drawPixmap(hotbar, imgHudHotbar);
        struct HotbarEntry {
            const QPixmap* icon = nullptr;
            int count = 0;
            bool selected = false;
            bool disabled = false;
        };
        HotbarEntry entries[6] = {};
        const auto& weapons = inv.weapons();
        for (int slot = 0; slot < 6; ++slot) {
            const int weaponIndex = inv.weaponIndexForQuickSlot(slot);
            if (weaponIndex < 0 || weaponIndex >= static_cast<int>(weapons.size()) || !weapons[weaponIndex]) {
                continue;
            }
            const QPixmap& icon = weaponIcon(weapons[weaponIndex]);
            entries[slot] = {
                &icon,
                -1,
                weaponIndex == inv.currentWeaponIndex(),
                weapons[weaponIndex]->isBroken()
            };
        }

        int nextItemSlot = 0;
        auto addItemSlot = [&](InventoryItemType type, const QPixmap& icon) {
            const int count = inv.getItemCount(type);
            if (count <= 0) return;
            while (nextItemSlot < 6 && entries[nextItemSlot].icon) {
                ++nextItemSlot;
            }
            if (nextItemSlot >= 6) return;
            entries[nextItemSlot++] = { &icon, count, false, false };
        };
        addItemSlot(InventoryItemType::Food, imgIconItemFood);
        addItemSlot(InventoryItemType::ShipRepairT1, imgIconItemRepairT1);
        addItemSlot(InventoryItemType::ShipRepairT2, imgIconItemRepairT2);
        addItemSlot(InventoryItemType::ShipRepairT3, imgIconItemRepairT3);
        addItemSlot(InventoryItemType::EmergencyWeaponRepair, imgIconItemEmergencyRepair);
        const int slotLefts[6] = { 418, 499, 581, 664, 746, 817 };
        for (int i = 0; i < 6; ++i) {
            QRect slot(slotLefts[i], 640, 58, 50);
            if (!entries[i].selected && !imgHudSlotNormal.isNull()) {
                p.drawPixmap(slot.adjusted(-5, -5, 5, 5), imgHudSlotNormal);
            }
            if (entries[i].selected && !imgHudSlotSelected.isNull()) {
                p.drawPixmap(slot.adjusted(-8, -9, 8, 9), imgHudSlotSelected);
            }
            if (entries[i].icon && !entries[i].icon->isNull()) {
                drawPixmapFit(*entries[i].icon, slot.adjusted(8, 2, -8, -2));
            }
            if (entries[i].disabled) {
                p.fillRect(slot.adjusted(5, 3, -5, -3), QColor(30, 30, 30, 130));
                p.setPen(QPen(QColor(210, 70, 60, 190), 3));
                p.drawLine(slot.left() + 10, slot.bottom() - 8, slot.right() - 10, slot.top() + 8);
            }
            drawText(QRect(slot.left() + 2, slot.top() - 12, 20, 18), QString::number(i + 1), 12, QColor(255, 234, 160));
            if (entries[i].count > 0) {
                drawText(QRect(slot.right() - 26, slot.bottom() - 16, 26, 16), QString::number(entries[i].count), 12, QColor(255, 234, 160));
            }
        }

        p.restore();
        return;
    }
    p.fillRect(0, 0, 1280, 44, QColor(0, 0, 0, 170));

    p.setFont(QFont("Microsoft YaHei", 10));
    p.setPen(Qt::white);

    // 耐久条
    p.drawText(10, 28, "耐久");
    p.fillRect(50, 8, 80, 12, QColor(60, 60, 60));
    int durW = qBound(0, 80 * pl.durability() / qMax(1, pl.maxDurability), 80);
    p.fillRect(50, 8, durW, 12, QColor(80, 200, 80));

    // 体力条
    p.drawText(145, 28, "体力");
    p.fillRect(185, 8, 80, 12, QColor(60, 60, 60));
    int staW = qBound(0, 80 * pl.stamina() / qMax(1, pl.maxStamina), 80);
    p.fillRect(185, 8, staW, 12, QColor(200, 200, 50));

    // 文字信息
    p.drawText(280, 28, QString("金:%1").arg(coinDisplayText(pl)));
    p.drawText(360, 28, QString("距:%1m").arg(pl.distance));
    p.drawText(460, 28, QString("鱼:%1").arg(pl.fishCaught));
    p.drawText(530, 28, QString("杀:%1").arg(gm->killCount));

    int sec = pl.gameSeconds;
    p.drawText(600, 28, QString("%1:%2")
        .arg(sec / 60, 2, 10, QChar('0'))
        .arg(sec % 60, 2, 10, QChar('0')));

    // 武器信息
    Weapon* w = InventorySystem::instance().currentWeapon();
    if (w) {
        p.drawText(680, 28, QString("%1 %2/%3")
            .arg(QString::fromStdString(w->getName()))
            .arg(w->getCurrentDur())
            .arg(w->getMaxDur()));
    }

    // 天气
    switch (WeatherSystem::instance().currentWeather()) {
    case WeatherType::SUNNY:
        p.setPen(QColor(255, 220, 80));
        p.drawText(900, 28, "晴天"); break;
    case WeatherType::FOG:
        p.setPen(QColor(200, 200, 200));
        p.drawText(900, 28, "大雾"); break;
    case WeatherType::STORM:
        p.setPen(QColor(100, 150, 255));
        p.drawText(900, 28, "暴风雨"); break;
    }

    // 关卡进度条
    p.setPen(Qt::white);
    p.drawText(1000, 18, QString("关卡%1/%2").arg(gm->stage).arg(Config::GameConfig::STAGE_COUNT));
    p.fillRect(1000, 24, 120, 8, QColor(60, 60, 60));
    const int stageStart = Config::GameConfig::stageStartDistance(gm->stage);
    const int stageEnd = Config::GameConfig::stageConfig(gm->stage).targetDistance;
    const int stageSpan = qMax(1, stageEnd - stageStart);
    const int stageDistance = qBound(0, pl.distance - stageStart, stageSpan);
    int prog = std::min(120, (int)(stageDistance * 120 / stageSpan));
    p.fillRect(1000, 24, prog, 8, QColor(100, 200, 100));
    p.setPen(QPen(Qt::white, 1));
    p.drawRect(1000, 24, 120, 8);

    p.setPen(QColor(180, 180, 180));
    p.setFont(QFont("Microsoft YaHei", 8));
    p.drawText(1110, 28, "左键射击/捕 空格闪避 B背包 4-6补给");
}

void GameWindow::drawTestModeOverlay(QPainter& p)
{
    if (!testModeEnabled) return;

    p.save();
    p.scale(width() / 1280.0, height() / 720.0);
    const QRect badge(14, 498, 210, 42);
    if (!imgWoodNoticeButton.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(badge, imgWoodNoticeButton, imgWoodNoticeButton.rect());
    } else {
        p.fillRect(badge, QColor(78, 42, 19, 210));
    }

    QFont font("Microsoft YaHei");
    font.setPixelSize(13);
    font.setWeight(QFont::Bold);
    p.setFont(font);
    p.setPen(QColor(255, 232, 151));
    p.drawText(badge, Qt::AlignCenter, QStringLiteral("测试模式  O / P商店"));
    p.restore();
}

void GameWindow::drawFloatingNotice(QPainter& p)
{
    if (!floatingNotice.active) return;

    p.save();
    p.scale(width() / 1280.0, height() / 720.0);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal fadeIn = qBound(0.0, static_cast<qreal>(floatingNotice.ageMs) / 180.0, 1.0);
    const qreal fadeOut = qBound(0.0, static_cast<qreal>(floatingNotice.lifetimeMs - floatingNotice.ageMs) / 300.0, 1.0);
    p.setOpacity(qMin(fadeIn, fadeOut));

    const QRect panel(392, 82 + static_cast<int>((1.0 - fadeIn) * -18), 496, 220);
    if (!imgWoodNoticeBoard.isNull()) {
        p.drawPixmap(panel, imgWoodNoticeBoard, imgWoodNoticeBoard.rect());
    } else {
        p.fillRect(panel, QColor(70, 38, 17, 230));
    }

    if (!imgNoticeIconInfo.isNull()) {
        p.drawPixmap(QRect(panel.left() + 64, panel.top() + 80, 52, 52), imgNoticeIconInfo, imgNoticeIconInfo.rect());
    }

    auto drawText = [&](const QRect& rect, const QString& text, int size, const QColor& color, bool bold) {
        QFont font("Microsoft YaHei");
        font.setPixelSize(size);
        font.setWeight(bold ? QFont::Bold : QFont::Normal);
        p.setFont(font);
        p.setPen(color);
        p.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text);
    };

    drawText(QRect(panel.left() + 132, panel.top() + 68, panel.width() - 220, 40),
             floatingNotice.title, 20, QColor(88, 42, 12), true);
    drawText(QRect(panel.left() + 132, panel.top() + 112, panel.width() - 220, 58),
             floatingNotice.body, 13, QColor(78, 48, 18), true);
    p.restore();
}

void GameWindow::drawBossEncounterNotice(QPainter& p)
{
    if (bossEncounterRemainingMs <= 0 || imgBossEncounterWarning.isNull()) return;

    constexpr int durationMs = 1800;
    const int ageMs = durationMs - bossEncounterRemainingMs;
    const qreal fadeIn = qBound<qreal>(0.0, ageMs / 220.0, 1.0);
    const qreal fadeOut = qBound<qreal>(0.0, bossEncounterRemainingMs / 300.0, 1.0);
    const qreal opacity = qMin(fadeIn, fadeOut);
    const int slideOffset = qRound((1.0 - fadeIn) * -36.0);

    p.save();
    p.scale(width() / 1280.0, height() / 720.0);
    p.setOpacity(opacity);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRect panel(250 + slideOffset, 270, 780, 140);
    p.drawPixmap(panel, imgBossEncounterWarning, imgBossEncounterWarning.rect());

    QFont titleFont("Microsoft YaHei");
    titleFont.setPixelSize(30);
    titleFont.setWeight(QFont::Black);
    p.setFont(titleFont);
    p.setPen(QColor(38, 10, 5, 230));
    p.drawText(QRect(390 + slideOffset, 296, 500, 44),
               Qt::AlignCenter, QStringLiteral("BOSS 出没注意"));
    p.setPen(QColor(255, 105, 52));
    p.drawText(QRect(388 + slideOffset, 294, 500, 44),
               Qt::AlignCenter, QStringLiteral("BOSS 出没注意"));

    QFont nameFont("Microsoft YaHei");
    nameFont.setPixelSize(22);
    nameFont.setWeight(QFont::Bold);
    p.setFont(nameFont);
    p.setPen(QColor(255, 229, 178));
    p.drawText(QRect(390 + slideOffset, 352, 500, 34),
               Qt::AlignCenter, bossDisplayName(encounterBossKind));
    p.restore();
}

// ============================================================
// 捕鱼进度条
// ============================================================

void GameWindow::drawFishingHUD(QPainter& p)
{
    if (!isFishing || !targetFish) return;

    QString fishName = fishDisplayName(targetFish->type);

    Weapon* weapon = InventorySystem::instance().currentWeapon();
    const Config::FishingMode mode = weapon ? weapon->getFishingMode() : Config::FishingMode::QTE;

    QColor fishColor = fishAccentColor(targetFish->type);
    QPixmap* fishSprite = nullptr;
    switch (targetFish->type) {
    case Fish::SARDINE:
        fishSprite = &imgSardine;
        break;
    case Fish::TUNA:
        fishSprite = &imgTuna;
        break;
    case Fish::DEEPSEAEEL:
        fishSprite = &imgEel;
        break;
    case Fish::SWORDFISH_FISH:
        fishSprite = &imgGolden;
        break;
    case Fish::ANCHOVY:
        fishSprite = &imgAnchovy;
        break;
    case Fish::CLOWNFISH:
        fishSprite = &imgClownfish;
        break;
    case Fish::MACKEREL:
        fishSprite = &imgMackerel;
        break;
    case Fish::SEA_BREAM:
        fishSprite = &imgSeaBream;
        break;
    case Fish::LANTERNFISH:
        fishSprite = &imgLanternfish;
        break;
    case Fish::GROUPER:
        fishSprite = &imgGrouper;
        break;
    case Fish::KOI:
        fishSprite = &imgKoi;
        break;
    case Fish::CRYSTAL_FISH:
        fishSprite = &imgCrystalFish;
        break;
    }

    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qreal timeRatio = qBound(
            0.0,
            1.0 - static_cast<qreal>(fishTimer) / qMax(1, targetFish->catchTimeLimit),
            1.0
        );
        const QColor timeColor = timeRatio < 0.25 ? QColor(235, 78, 55)
            : (timeRatio < 0.5 ? QColor(238, 177, 58) : QColor(74, 210, 176));

        const QRect panel(448, 72, 384, 112);
        const QRect content = panel.adjusted(42, 24, -42, -18);

        auto drawSmallText = [&](const QRect& rect, const QString& text, int size,
                                 const QColor& color, int flags = Qt::AlignCenter) {
            QFont font("Microsoft YaHei", size, QFont::Bold);
            font.setHintingPreference(QFont::PreferFullHinting);
            p.setFont(font);
            p.setPen(QColor(12, 7, 4, 210));
            p.drawText(rect.translated(1, 1), flags, text);
            p.setPen(color);
            p.drawText(rect, flags, text);
        };

        auto drawMiniMouse = [&](int x, int y) {
            p.fillRect(x + 5, y, 14, 20, QColor(222, 222, 215));
            p.fillRect(x + 2, y + 5, 20, 18, QColor(188, 194, 194));
            p.fillRect(x + 10, y + 2, 3, 9, QColor(72, 78, 80));
            p.fillRect(x + 5, y + 7, 8, 5, QColor(219, 66, 54));
            p.fillRect(x + 13, y + 7, 7, 5, QColor(239, 238, 224));
            p.fillRect(x + 4, y + 21, 16, 3, QColor(54, 58, 60));
        };

        p.save();
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.fillRect(panel.adjusted(5, 7, 5, 8), QColor(0, 0, 0, 86));
        if (!imgFishingQtePanel.isNull()) {
            p.drawPixmap(panel, imgFishingQtePanel);
        }
        else {
            p.fillRect(panel, QColor(42, 23, 12));
            p.fillRect(panel.adjusted(34, 20, -34, -18), QColor(9, 26, 36));
        }

        const QRect timeBar(panel.left() + 62, panel.bottom() - 18, panel.width() - 124, 6);
        p.fillRect(timeBar.adjusted(-1, -1, 1, 1), QColor(16, 10, 6, 220));
        p.fillRect(timeBar, QColor(8, 18, 24, 230));
        p.fillRect(timeBar.left(), timeBar.top(), static_cast<int>(timeBar.width() * timeRatio), timeBar.height(), timeColor);

        if (mode == Config::FishingMode::Calibration) {
            drawSmallText(QRect(content.left(), content.top() - 2, content.width(), 22),
                          QStringLiteral("\u6821\u51c6\u65f6\u673a"), 14, QColor(255, 229, 176));
            drawSmallText(QRect(content.left(), content.top() + 18, content.width(), 18),
                          QStringLiteral("\u7eff\u8272\u533a\u57df\u70b9\u51fb\u5de6\u952e"), 10, QColor(246, 184, 88));

            const QRect ruler(content.left() + 24, content.top() + 42, content.width() - 48, 18);
            p.fillRect(ruler.adjusted(-5, -4, 5, 4), QColor(45, 24, 12));
            p.fillRect(ruler, QColor(135, 76, 34));
            p.fillRect(ruler.adjusted(4, 4, -4, -4), QColor(174, 112, 49));
            const int normalHalf = static_cast<int>(ruler.width() * calibrationWindowSize(false));
            const int perfectHalf = static_cast<int>(ruler.width() * calibrationWindowSize(true));
            const int centerX = ruler.left() + static_cast<int>(
                ruler.width() * calibrationTargetCenterRatio());
            p.fillRect(centerX - normalHalf, ruler.top() + 3, normalHalf * 2, ruler.height() - 6, QColor(224, 180, 62));
            p.fillRect(centerX - perfectHalf, ruler.top() + 3, perfectHalf * 2, ruler.height() - 6, QColor(75, 197, 78));
            for (int x = ruler.left() + 12; x < ruler.right(); x += 18) {
                p.fillRect(x, ruler.bottom() - 8, 2, 7, QColor(45, 25, 13));
            }

            const int markerX = ruler.left() + static_cast<int>(ruler.width() * calibrationMarkerRatio());
            p.fillRect(markerX - 2, ruler.top() - 13, 4, 32, QColor(239, 244, 226));
            p.fillRect(markerX - 10, ruler.top() - 12, 20, 12, QColor(212, 54, 45));
            p.fillRect(markerX - 8, ruler.top() - 9, 16, 5, QColor(255, 226, 177));
            p.fillRect(markerX - 10, ruler.top(), 20, 4, QColor(52, 53, 55));
        }
        else {
            const QRect fishTarget(content.left() + 4, content.top() + 16, 58, 34);
            if (fishSprite && !fishSprite->isNull()) {
                const int frameCount = 4;
                const int frameW = fishSprite->width() / frameCount;
                const int frame = static_cast<int>((now / 140) % frameCount);
                p.drawPixmap(fishTarget, *fishSprite, QRect(frame * frameW, 0, frameW, fishSprite->height()));
            }
            else {
                p.fillRect(fishTarget.adjusted(7, 9, -10, -9), fishColor);
                p.fillRect(fishTarget.right() - 17, fishTarget.center().y() - 8, 13, 16, fishColor.darker(112));
                p.fillRect(fishTarget.left() + 18, fishTarget.top() + 14, 4, 4, QColor(24, 25, 32));
            }
            for (int i = 0; i < 6; ++i) {
                p.fillRect(fishTarget.left() - 8 + i * 11, fishTarget.bottom() - 4 + (i % 2) * 3,
                           6, 3, QColor(116, 218, 255, 145));
            }

            drawSmallText(QRect(content.left() + 76, content.top() + 6, content.width() - 80, 22),
                          QStringLiteral("%1 \u4e0a\u94a9\u4e2d").arg(fishName), 14, QColor(255, 233, 190), Qt::AlignLeft | Qt::AlignVCenter);
            drawMiniMouse(content.left() + 78, content.top() + 36);
            drawSmallText(QRect(content.left() + 108, content.top() + 36, 180, 22),
                          QStringLiteral("\u8fde\u70b9\u6536\u7ebf"), 11, QColor(244, 190, 82), Qt::AlignLeft | Qt::AlignVCenter);

            const int required = qMax(1, targetFish->catchRequired);
            const int pipCount = qBound(3, qMin(required, 8), 8);
            const int litPips = qBound(0, fishClickCount * pipCount / required, pipCount);
            const int startX = content.left() + 222;
            for (int i = 0; i < pipCount; ++i) {
                const QRect pip(startX + i * 9, content.top() + 42, 6, 13);
                p.fillRect(pip, i < litPips ? QColor(255, 211, 70) : QColor(64, 68, 68));
                p.fillRect(pip.left() + 1, pip.top() + 1, 2, 2,
                           i < litPips ? QColor(255, 245, 164) : QColor(103, 108, 108));
            }
            drawSmallText(QRect(content.left() + 246, content.top() + 18, 54, 16),
                          QString("%1/%2").arg(fishClickCount).arg(required), 9, QColor(235, 224, 196));
        }

        p.restore();
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qreal timeRatio = qBound(
        0.0,
        1.0 - static_cast<qreal>(fishTimer) / qMax(1, targetFish->catchTimeLimit),
        1.0
    );
    const QColor timeColor = timeRatio < 0.25 ? QColor(238, 84, 54)
        : (timeRatio < 0.5 ? QColor(238, 184, 64) : QColor(76, 216, 184));

    const QRect panel(226, 92, 828, 294);
    const QRect inner(panel.left() + 92, panel.top() + 98, panel.width() - 184, 154);
    const QRect titlePlaque(panel.center().x() - 176, panel.top() + 44, 352, 44);

    auto drawPixelText = [&](const QRect& rect, const QString& text, int size,
                             const QColor& color, int flags = Qt::AlignCenter) {
        QFont font("Microsoft YaHei", size, QFont::Bold);
        font.setStyleStrategy(QFont::PreferAntialias);
        font.setHintingPreference(QFont::PreferFullHinting);
        p.setFont(font);
        p.setPen(QColor(27, 14, 5, 220));
        p.drawText(rect.translated(3, 3), flags, text);
        p.setPen(QColor(119, 75, 31, 150));
        p.drawText(rect.translated(1, 1), flags, text);
        p.setPen(color);
        p.drawText(rect, flags, text);
    };

    auto drawRivet = [&](int cx, int cy, int size) {
        p.fillRect(cx - size / 2 - 1, cy - size / 2 + 1, size + 2, size + 2, QColor(47, 22, 10));
        p.fillRect(cx - size / 2, cy - size / 2, size, size, QColor(205, 137, 50));
        p.fillRect(cx - size / 2 + 3, cy - size / 2 + 3, qMax(2, size / 3), qMax(2, size / 3), QColor(255, 221, 115));
        p.fillRect(cx + size / 5, cy + size / 5, qMax(2, size / 4), qMax(2, size / 4), QColor(88, 44, 17));
    };

    auto drawAnchor = [&](int cx, int cy, int scale, const QColor& color) {
        QPen shadow(color.darker(175), qMax(3, scale / 5), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        p.setPen(shadow);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - scale / 4 + 2, cy - scale + 2, scale / 2, scale / 2);
        p.drawLine(cx + 2, cy - scale / 2 + 2, cx + 2, cy + scale + 2);
        p.drawLine(cx - scale / 2 + 2, cy - scale / 8 + 2, cx + scale / 2 + 2, cy - scale / 8 + 2);
        p.drawArc(QRect(cx - scale + 2, cy + scale / 4 + 2, scale * 2, scale * 2), 205 * 16, 130 * 16);
        p.drawLine(cx - scale + 2, cy + scale + 2, cx - scale / 2 + 2, cy + scale / 2 + 2);
        p.drawLine(cx + scale + 2, cy + scale + 2, cx + scale / 2 + 2, cy + scale / 2 + 2);

        shadow.setColor(color);
        p.setPen(shadow);
        p.drawEllipse(cx - scale / 4, cy - scale, scale / 2, scale / 2);
        p.drawLine(cx, cy - scale / 2, cx, cy + scale);
        p.drawLine(cx - scale / 2, cy - scale / 8, cx + scale / 2, cy - scale / 8);
        p.drawArc(QRect(cx - scale, cy + scale / 4, scale * 2, scale * 2), 205 * 16, 130 * 16);
        p.drawLine(cx - scale, cy + scale, cx - scale / 2, cy + scale / 2);
        p.drawLine(cx + scale, cy + scale, cx + scale / 2, cy + scale / 2);
    };

    auto drawRope = [&](int x, int y0, int y1) {
        for (int y = y0; y < y1; y += 10) {
            p.fillRect(x, y, 8, 8, QColor(82, 55, 28));
            p.fillRect(x + 7, y + 4, 8, 8, QColor(157, 104, 49));
            p.fillRect(x + 3, y + 1, 3, 10, QColor(214, 158, 82));
        }
        p.fillRect(x - 6, y1 - 2, 27, 8, QColor(55, 34, 18));
    };

    auto drawCornerPlate = [&](const QRect& c) {
        p.fillRect(c, QColor(54, 28, 14));
        p.fillRect(c.adjusted(4, 4, -4, -4), QColor(169, 104, 39));
        p.fillRect(c.adjusted(10, 10, -10, -10), QColor(239, 176, 74));
        p.fillRect(c.left() + 10, c.top() + 10, c.width() - 20, 5, QColor(255, 219, 111, 150));
        p.fillRect(c.left() + 10, c.bottom() - 16, c.width() - 20, 5, QColor(80, 42, 19, 150));
        drawRivet(c.left() + 19, c.top() + 18, 12);
        drawRivet(c.right() - 19, c.top() + 18, 12);
        drawRivet(c.left() + 23, c.bottom() - 20, 11);
        drawRivet(c.right() - 23, c.bottom() - 20, 11);
    };

    auto drawWoodFrame = [&](const QRect& r) {
        p.fillRect(r.adjusted(12, 14, 12, 16), QColor(0, 0, 0, 116));
        p.fillRect(r, QColor(39, 18, 8));
        p.fillRect(r.adjusted(7, 7, -7, -7), QColor(112, 52, 22));

        const QRect top(r.left() + 24, r.top() + 17, r.width() - 48, 38);
        const QRect bottom(r.left() + 24, r.bottom() - 55, r.width() - 48, 38);
        const QRect left(r.left() + 16, r.top() + 50, 42, r.height() - 100);
        const QRect right(r.right() - 58, r.top() + 50, 42, r.height() - 100);
        p.fillRect(top, QColor(137, 68, 30));
        p.fillRect(top.adjusted(0, 4, 0, -24), QColor(201, 113, 45));
        p.fillRect(bottom, QColor(90, 43, 20));
        p.fillRect(bottom.adjusted(0, 6, 0, -26), QColor(151, 76, 32));
        p.fillRect(left, QColor(84, 39, 18));
        p.fillRect(left.adjusted(6, 0, -22, 0), QColor(139, 70, 32));
        p.fillRect(right, QColor(84, 39, 18));
        p.fillRect(right.adjusted(22, 0, -6, 0), QColor(139, 70, 32));

        for (int x = top.left() + 18; x < top.right() - 22; x += 92) {
            p.fillRect(x, top.top() + 8, 58, 4, QColor(230, 146, 59, 130));
            p.fillRect(x + 28, top.bottom() - 9, 72, 4, QColor(47, 22, 10, 150));
        }
        for (int x = bottom.left() + 36; x < bottom.right() - 28; x += 108) {
            p.fillRect(x, bottom.top() + 11, 74, 4, QColor(36, 18, 9, 130));
            p.fillRect(x + 10, bottom.bottom() - 10, 45, 3, QColor(190, 105, 43, 110));
        }
        for (int x = top.left() + 118; x < top.right() - 60; x += 154) {
            p.fillRect(x, top.top() - 4, 6, 50, QColor(47, 24, 13));
            p.fillRect(x + 6, top.top(), 3, 42, QColor(213, 128, 51));
        }

        drawCornerPlate(QRect(r.left() + 10, r.top() + 8, 76, 70));
        drawCornerPlate(QRect(r.right() - 86, r.top() + 8, 76, 70));
        drawCornerPlate(QRect(r.left() + 10, r.bottom() - 78, 76, 70));
        drawCornerPlate(QRect(r.right() - 86, r.bottom() - 78, 76, 70));
    };

    auto drawTitlePlaque = [&](const QRect& r) {
        p.fillRect(r.adjusted(6, 8, 6, 10), QColor(0, 0, 0, 110));
        p.fillRect(r, QColor(42, 19, 8));
        p.fillRect(r.adjusted(7, 7, -7, -7), QColor(112, 52, 22));
        p.fillRect(r.adjusted(22, 16, -22, -16), QColor(73, 37, 17));
        p.fillRect(r.left() + 36, r.top() + 24, r.width() - 72, 7, QColor(177, 95, 38));
        drawCornerPlate(QRect(r.left() + 8, r.top() + 8, 46, 42));
        drawCornerPlate(QRect(r.right() - 54, r.top() + 8, 46, 42));
        drawRivet(r.left() + 76, r.top() + 20, 9);
        drawRivet(r.right() - 76, r.top() + 20, 9);
        drawAnchor(r.left() + 104, r.center().y() + 3, 22, QColor(255, 221, 136));
        drawPixelText(r.adjusted(150, 4, -44, -4), QStringLiteral("\u4e0a\u94a9\u6311\u6218"), 28, QColor(255, 228, 148));
    };

    auto drawHookIcon = [&](int cx, int cy, bool lit, int scale) {
        QColor glow = lit ? QColor(255, 213, 72) : QColor(48, 52, 52);
        QColor metal = lit ? QColor(255, 238, 151) : QColor(79, 86, 84);
        if (lit) {
            p.fillRect(cx - scale / 2 - 10, cy - scale / 2 - 10, scale + 20, scale + 20, QColor(255, 191, 38, 34));
            p.fillRect(cx - scale / 3, cy + scale / 2 + 10, scale / 2, 4, QColor(255, 236, 128, 170));
            p.fillRect(cx - scale / 2 - 2, cy + scale / 2 + 17, scale + 4, 3, QColor(255, 202, 59, 120));
        }
        p.setPen(QPen(glow.darker(150), qMax(3, scale / 8), Qt::SolidLine, Qt::RoundCap));
        p.drawLine(cx, cy - scale / 2, cx, cy + scale / 4);
        p.drawArc(QRect(cx - scale / 2, cy - scale / 10, scale, scale), 200 * 16, 245 * 16);
        p.drawLine(cx + scale / 2 - 2, cy + scale / 3, cx + scale / 2 + scale / 5, cy + scale / 12);
        p.setPen(QPen(metal, qMax(2, scale / 10), Qt::SolidLine, Qt::RoundCap));
        p.drawLine(cx, cy - scale / 2, cx, cy + scale / 4);
        p.drawArc(QRect(cx - scale / 2, cy - scale / 10, scale, scale), 200 * 16, 245 * 16);
        p.drawLine(cx + scale / 2 - 2, cy + scale / 3, cx + scale / 2 + scale / 5, cy + scale / 12);
    };

    auto drawBanner = [&](const QRect& r, const QString& text) {
        QPolygon banner;
        banner << QPoint(r.left(), r.center().y())
               << QPoint(r.left() + 24, r.top())
               << QPoint(r.right() - 24, r.top())
               << QPoint(r.right(), r.center().y())
               << QPoint(r.right() - 24, r.bottom())
               << QPoint(r.left() + 24, r.bottom());
        p.setBrush(QColor(82, 43, 18));
        p.setPen(QPen(QColor(190, 124, 48), 3));
        p.drawPolygon(banner);
        p.fillRect(r.adjusted(28, 7, -28, -7), QColor(42, 25, 15));
        drawRivet(r.left() + 24, r.center().y(), 8);
        drawRivet(r.right() - 24, r.center().y(), 8);
        drawPixelText(r, text, 18, QColor(248, 213, 147));
    };

    auto drawSeaPattern = [&](const QRect& area) {
        for (int i = 0; i < 34; ++i) {
            const int x = area.left() + 24 + (i * 71 + static_cast<int>(now / 90)) % qMax(1, area.width() - 62);
            const int y = area.top() + 22 + (i * 37) % qMax(1, area.height() - 48);
            const QColor c(97, 170, 190, 38 + (i % 3) * 12);
            p.fillRect(x, y, 18, 4, c);
            p.fillRect(x + 11, y + 6, 15, 4, c);
            p.fillRect(x + 27, y + 2, 10, 4, c);
        }
        for (int y = area.top() + 40; y < area.bottom() - 20; y += 58) {
            p.fillRect(area.left() + 18, y, 6, 6, QColor(189, 144, 73, 65));
            p.fillRect(area.right() - 24, y + 18, 6, 6, QColor(189, 144, 73, 65));
        }
    };

    auto drawInputMouse = [&](const QRect& box, const QString& text) {
        p.fillRect(box.adjusted(5, 5, 5, 5), QColor(0, 0, 0, 90));
        QLinearGradient trimGrad(box.topLeft(), box.bottomLeft());
        trimGrad.setColorAt(0.0, QColor(111, 59, 25));
        trimGrad.setColorAt(0.45, QColor(55, 28, 13));
        trimGrad.setColorAt(1.0, QColor(24, 12, 6));
        p.fillRect(box, trimGrad);
        QLinearGradient boxGrad(box.topLeft(), box.bottomLeft());
        boxGrad.setColorAt(0.0, QColor(42, 66, 70));
        boxGrad.setColorAt(0.5, QColor(15, 34, 44));
        boxGrad.setColorAt(1.0, QColor(5, 18, 29));
        p.fillRect(box.adjusted(4, 4, -4, -4), boxGrad);
        p.fillRect(box.left() + 8, box.top() + 7, box.width() - 16, 2, QColor(133, 196, 189, 58));

        const QRect mouse(box.left() + 30, box.center().y() - 17, 34, 34);
        p.fillRect(mouse.adjusted(7, 1, -7, -1), QColor(226, 226, 220));
        p.fillRect(mouse.adjusted(3, 6, -3, -2), QColor(198, 203, 202));
        p.fillRect(mouse.left() + 15, mouse.top() + 3, 4, 13, QColor(85, 91, 92));
        p.fillRect(mouse.left() + 6, mouse.top() + 8, 11, 7, QColor(224, 70, 59));
        p.fillRect(mouse.left() + 17, mouse.top() + 8, 11, 7, QColor(246, 245, 230));
        p.fillRect(mouse.left() + 5, mouse.bottom() - 2, 24, 4, QColor(61, 65, 66));

        drawPixelText(box.adjusted(74, 0, -16, 0), text, 16, QColor(245, 227, 194), Qt::AlignVCenter | Qt::AlignLeft);
    };

    auto drawBigFish = [&](const QRect& fishTarget) {
        for (int i = 0; i < 30; ++i) {
            const int sx = fishTarget.left() - 50 + (i * 19) % (fishTarget.width() + 110);
            const int sy = fishTarget.bottom() - 28 + (i % 5) * 10 - static_cast<int>((now / 95) % 7);
            p.fillRect(sx, sy, 7, 7, QColor(110, 218, 255, 155));
            p.fillRect(sx + 8, sy + 5, 12, 4, QColor(204, 248, 255, 120));
            if (i % 4 == 0) {
                p.fillRect(sx - 4, sy - 10, 4, 8, QColor(84, 194, 235, 115));
            }
        }
        if (fishSprite && !fishSprite->isNull()) {
            const int frameCount = 4;
            const int frameW = fishSprite->width() / frameCount;
            const int frame = static_cast<int>((now / 140) % frameCount);
            QRect source(frame * frameW, 0, frameW, fishSprite->height());
            p.drawPixmap(fishTarget, *fishSprite, source);
        }
        else {
            p.fillRect(fishTarget.adjusted(20, 26, -42, -34), fishColor);
            p.fillRect(fishTarget.right() - 58, fishTarget.center().y() - 26, 46, 52, fishColor.darker(116));
            p.fillRect(fishTarget.left() + 52, fishTarget.top() + 44, 9, 9, QColor(24, 25, 32));
            p.fillRect(fishTarget.left() + 18, fishTarget.center().y() - 8, 28, 16, fishColor.lighter(132));
        }
        p.fillRect(fishTarget.left() + 10, fishTarget.bottom() - 10, fishTarget.width() - 22, 5, QColor(0, 8, 20, 70));
    };

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    p.fillRect(panel.adjusted(18, 22, 18, 24), QColor(0, 0, 0, 92));
    if (!imgMenuTitlePlaque.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(panel, imgMenuTitlePlaque);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    }
    else {
        drawRope(panel.left() + 96, -18, panel.top() + 54);
        drawRope(panel.right() - 112, -18, panel.top() + 54);
        drawWoodFrame(panel);
    }

    QLinearGradient seaGrad(inner.topLeft(), inner.bottomLeft());
    seaGrad.setColorAt(0.0, QColor(9, 37, 58, 230));
    seaGrad.setColorAt(0.45, QColor(4, 24, 40, 238));
    seaGrad.setColorAt(1.0, QColor(2, 14, 27, 242));
    p.fillRect(inner.adjusted(5, 6, 5, 7), QColor(0, 0, 0, 76));
    p.fillRect(inner, QColor(38, 20, 9, 180));
    p.fillRect(inner.adjusted(3, 3, -3, -3), seaGrad);
    p.fillRect(inner.left() + 8, inner.top() + 8, inner.width() - 16, 3, QColor(119, 189, 204, 62));
    p.fillRect(inner.left() + 8, inner.bottom() - 12, inner.width() - 16, 4, QColor(0, 7, 17, 130));
    drawSeaPattern(inner.adjusted(12, 10, -12, -10));
    drawAnchor(titlePlaque.left() + 48, titlePlaque.center().y() + 2, 15, QColor(255, 221, 136));
    drawPixelText(titlePlaque.adjusted(88, 0, -16, 0), QStringLiteral("\u4e0a\u94a9\u6311\u6218"), 24, QColor(255, 228, 148));

    if (mode == Config::FishingMode::Calibration) {
        drawBanner(QRect(inner.center().x() - 166, inner.top() + 8, 332, 38),
                   QStringLiteral("\u6e14\u7f51 / \u9c7c\u53c9\u6821\u51c6 QTE"));
        drawPixelText(QRect(inner.left() + 226, inner.top() + 42, 314, 34),
                      QStringLiteral("\u6821\u51c6\u65f6\u673a"), 24, QColor(255, 240, 203));
        drawPixelText(QRect(inner.left() + 154, inner.top() + 72, 458, 24),
                      QStringLiteral("\u6d6e\u6807\u8fdb\u5165\u7eff\u8272\u533a\u65f6\u70b9\u51fb\u5de6\u952e\uff01"), 14, QColor(248, 187, 88));

        const QRect ruler(inner.left() + 118, inner.top() + 94, 408, 44);
        p.fillRect(ruler.adjusted(-13, -12, 13, 12), QColor(44, 21, 10));
        p.fillRect(ruler.adjusted(-9, -8, 9, 8), QColor(150, 86, 36));
        p.fillRect(ruler.adjusted(-3, -4, 3, 4), QColor(94, 47, 22));
        p.fillRect(ruler, QColor(146, 82, 37));
        p.fillRect(ruler.adjusted(8, 7, -8, -7), QColor(174, 106, 48));

        const int normalHalf = static_cast<int>(ruler.width() * calibrationWindowSize(false));
        const int perfectHalf = static_cast<int>(ruler.width() * calibrationWindowSize(true));
        const int centerX = ruler.left() + static_cast<int>(
            ruler.width() * calibrationTargetCenterRatio());
        p.fillRect(centerX - normalHalf, ruler.top() + 8, normalHalf * 2, ruler.height() - 16, QColor(232, 189, 69));
        p.fillRect(centerX - perfectHalf, ruler.top() + 8, perfectHalf * 2, ruler.height() - 16, QColor(78, 203, 82));
        p.fillRect(centerX - perfectHalf, ruler.top() + 8, perfectHalf * 2, 5, QColor(202, 255, 156, 120));
        p.setPen(QPen(QColor(255, 252, 216, 190), 2, Qt::DashLine));
        p.drawLine(centerX - perfectHalf, ruler.top() + 3, centerX - perfectHalf, ruler.bottom() + 10);
        p.drawLine(centerX + perfectHalf, ruler.top() + 3, centerX + perfectHalf, ruler.bottom() + 10);
        p.setPen(Qt::NoPen);
        for (int x = ruler.left() + 14; x < ruler.right(); x += 22) {
            const int h = (x - ruler.left()) % 66 == 0 ? 18 : 11;
            p.fillRect(x, ruler.bottom() - h - 7, 2, h, QColor(52, 30, 15));
        }
        p.fillRect(ruler.left() - 10, ruler.top() - 10, 20, ruler.height() + 20, QColor(88, 48, 23));
        p.fillRect(ruler.right() - 10, ruler.top() - 10, 20, ruler.height() + 20, QColor(88, 48, 23));
        drawRivet(ruler.left() - 1, ruler.top() - 2, 10);
        drawRivet(ruler.right() + 1, ruler.top() - 2, 10);
        drawRivet(ruler.left() - 1, ruler.bottom() + 2, 10);
        drawRivet(ruler.right() + 1, ruler.bottom() + 2, 10);

        const int markerX = ruler.left() + static_cast<int>(ruler.width() * calibrationMarkerRatio());
        p.setPen(QPen(QColor(221, 244, 255, 180), 2, Qt::DashLine));
        p.drawLine(markerX - 54, ruler.top() - 30, markerX + 24, ruler.top() - 6);
        p.drawLine(markerX + 38, ruler.top() - 27, markerX + 3, ruler.top() - 6);
        p.setPen(Qt::NoPen);
        p.fillRect(markerX - 3, ruler.top() - 23, 6, 72, QColor(239, 244, 226));
        p.fillRect(markerX - 16, ruler.top() - 22, 32, 21, QColor(216, 54, 46));
        p.fillRect(markerX - 12, ruler.top() - 18, 24, 8, QColor(255, 226, 177));
        p.fillRect(markerX - 16, ruler.top() - 1, 32, 7, QColor(55, 55, 58));
        p.fillRect(markerX - 6, ruler.top() - 35, 12, 13, QColor(239, 244, 226));
        for (int i = 0; i < 9; ++i) {
            const int sx = markerX - 62 + i * 15;
            const int sy = ruler.top() - 14 + (i % 3) * 6;
            p.fillRect(sx, sy, 5, 5, QColor(198, 238, 246, 130));
        }

        drawPixelText(QRect(ruler.left() - 24, ruler.bottom() + 8, 76, 22),
                      QStringLiteral("\u504f\u65e9"), 12, QColor(241, 111, 86));
        drawPixelText(QRect(centerX - normalHalf - 34, ruler.bottom() + 8, 86, 22),
                      QStringLiteral("\u6b63\u5e38"), 12, QColor(255, 226, 82));
        drawPixelText(QRect(centerX - 48, ruler.bottom() + 8, 96, 22),
                      QStringLiteral("\u2605 \u5b8c\u7f8e\uff01\u2605"), 12, QColor(126, 244, 117));
        drawPixelText(QRect(ruler.right() - 48, ruler.bottom() + 8, 76, 22),
                      QStringLiteral("\u504f\u665a"), 12, QColor(241, 111, 86));

    }
    else {
        drawBanner(QRect(inner.left() + 214, inner.top() + 8, 280, 38),
                   QStringLiteral("\u9c7c\u7aff\u6536\u7ebf QTE"));

        const QRect fishTarget(inner.left() + 28, inner.top() + 50, 150, 86);
        drawBigFish(fishTarget);

        const int hookX = inner.left() + 78;
        p.fillRect(hookX - 2, inner.top() + 14, 4, 86, QColor(229, 231, 216));
        p.fillRect(hookX + 2, inner.top() + 14, 2, 86, QColor(78, 80, 76));
        drawHookIcon(hookX + 8, inner.top() + 104, true, 40);

        drawPixelText(QRect(inner.left() + 212, inner.top() + 46, 346, 36),
                      QStringLiteral("%1 \u4e0a\u94a9\u4e2d").arg(fishName), 24, QColor(255, 240, 203));
        drawPixelText(QRect(inner.left() + 280, inner.top() + 78, 190, 24),
                      QStringLiteral("\u8fde\u70b9\u6536\u7ebf"), 14, QColor(249, 185, 76));
        p.fillRect(inner.left() + 206, inner.top() + 72, 20, 4, QColor(61, 184, 228));
        p.fillRect(inner.left() + 232, inner.top() + 76, 14, 3, QColor(92, 214, 246));
        p.fillRect(inner.left() + 520, inner.top() + 72, 20, 4, QColor(61, 184, 228));
        p.fillRect(inner.left() + 498, inner.top() + 76, 14, 3, QColor(92, 214, 246));

        const QRect rope(inner.left() + 214, inner.top() + 102, 304, 24);
        p.fillRect(rope.adjusted(-16, -5, 16, 5), QColor(44, 25, 13));
        p.fillRect(rope.adjusted(-10, -2, 10, 2), QColor(142, 84, 38));
        for (int x = rope.left(); x < rope.right(); x += 16) {
            p.fillRect(x, rope.top(), 13, rope.height(), QColor(170, 118, 61));
            p.fillRect(x + 7, rope.top() + 3, 7, rope.height() - 6, QColor(222, 168, 86));
            p.fillRect(x + 2, rope.top() + rope.height() - 5, 12, 2, QColor(83, 50, 24, 120));
        }
        p.fillRect(rope.left() - 12, rope.top() - 5, 12, rope.height() + 10, QColor(111, 60, 26));
        p.fillRect(rope.right(), rope.top() - 5, 12, rope.height() + 10, QColor(111, 60, 26));
        p.fillRect(rope.left(), rope.top(), static_cast<int>(rope.width() * timeRatio), 5,
                   QColor(timeColor.red(), timeColor.green(), timeColor.blue(), 120));

        const int required = qMax(1, targetFish->catchRequired);
        const int pipCount = qBound(3, qMin(required, 9), 9);
        const int litPips = qBound(0, fishClickCount * pipCount / required, pipCount);
        const int gap = 38;
        const int hooksWidth = (pipCount - 1) * gap;
        const int startX = inner.center().x() - hooksWidth / 2;
        for (int i = 0; i < pipCount; ++i) {
            drawHookIcon(startX + i * gap, inner.top() + 126, i < litPips, 24);
        }

        drawInputMouse(QRect(panel.left() + 292, panel.bottom() - 46, 294, 34),
                       QStringLiteral("\u5feb\u901f\u70b9\u51fb\u9f20\u6807\u5de6\u952e\uff01  %1/%2").arg(fishClickCount).arg(required));
    }

    const QRect timeBar(panel.left() + 170, panel.top() + 92, panel.width() - 340, 7);
    p.fillRect(timeBar.adjusted(-3, -3, 3, 3), QColor(39, 20, 11));
    p.fillRect(timeBar, QColor(8, 22, 31));
    p.fillRect(timeBar.left(), timeBar.top(),
               static_cast<int>(timeBar.width() * timeRatio), timeBar.height(), timeColor);
    p.fillRect(timeBar.left(), timeBar.top(), static_cast<int>(timeBar.width() * timeRatio), 3, QColor(230, 255, 239, 90));

    p.restore();
}

// ============================================================
// 商店
// ============================================================

void GameWindow::openShop()
{
    ShopDialog dlg(this);
    dlg.exec();
}

void GameWindow::openTestModeShop()
{
    if (!testModeEnabled) return;

    resetFishingState(true);
    Player::instance().clearInputState();
    applyTestModeBenefits();
    timer->stop();
    openShop();
    applyTestModeBenefits();
    timer->start(16);
    update();
}

void GameWindow::openBackpack()
{
    BackpackDialog dlg(gm ? gm->stage : 1, this);
    dlg.exec();
}

void GameWindow::openEncyclopedia()
{
    EncyclopediaDialog dlg(gm ? gm->stage : 1, this);
    dlg.exec();
}

bool GameWindow::useQuickItemSlot(int hotbarIndex)
{
    if (hotbarIndex < 0 || hotbarIndex > 5) {
        return false;
    }

    InventorySystem& inv = InventorySystem::instance();
    if (inv.weaponIndexForQuickSlot(hotbarIndex) >= 0) {
        return false;
    }

    int itemSlots[6] = { -1, -1, -1, -1, -1, -1 };
    int nextEmptySlot = 0;
    auto addVisibleItem = [&](InventoryItemType type) {
        if (inv.getItemCount(type) <= 0) return;
        while (nextEmptySlot < 6 && inv.weaponIndexForQuickSlot(nextEmptySlot) >= 0) {
            ++nextEmptySlot;
        }
        if (nextEmptySlot >= 6) return;
        itemSlots[nextEmptySlot++] = static_cast<int>(type);
    };

    addVisibleItem(InventoryItemType::Food);
    addVisibleItem(InventoryItemType::ShipRepairT1);
    addVisibleItem(InventoryItemType::ShipRepairT2);
    addVisibleItem(InventoryItemType::ShipRepairT3);
    addVisibleItem(InventoryItemType::EmergencyWeaponRepair);

    if (itemSlots[hotbarIndex] < 0) {
        return false;
    }

    bool used = false;
    switch (static_cast<InventoryItemType>(itemSlots[hotbarIndex])) {
    case InventoryItemType::Food:
        used = inv.useFood(Player::instance());
        break;
    case InventoryItemType::ShipRepairT1:
        used = inv.useShipRepairKit(Player::instance(), 1);
        break;
    case InventoryItemType::ShipRepairT2:
        used = inv.useShipRepairKit(Player::instance(), 2);
        break;
    case InventoryItemType::ShipRepairT3:
        used = inv.useShipRepairKit(Player::instance(), 3);
        break;
    case InventoryItemType::EmergencyWeaponRepair:
        used = inv.useEmergencyWeaponRepair(inv.currentWeaponIndex());
        break;
    }

    if (used) {
        update();
    }
    return used;
}

void GameWindow::toggleTestMode()
{
    testModeEnabled = !testModeEnabled;
    if (testModeEnabled) {
        applyTestModeBenefits();
    } else {
        Player::instance().testModeInfiniteCoins = false;
    }
    update();
}

void GameWindow::applyTestModeBenefits()
{
    if (!testModeEnabled) return;

    Player& pl = Player::instance();
    if (pl.isDead()) {
        pl.restoreSavedProgress(
            pl.distance,
            pl.maxDurability,
            pl.maxStamina,
            pl.maxDurability,
            pl.maxStamina,
            pl.baseSpeed()
        );
    }

    pl.clearMaxStaminaPenalty();
    pl.restoreDurability(pl.maxDurability);
    pl.restoreStamina(pl.maxStamina);
    pl.testModeInfiniteCoins = true;
    if (gm) {
        gm->gameOver = false;
    }
}

void GameWindow::confirmStagePrompt()
{
    promptButtonHover = false;
    setCursor(Qt::ArrowCursor);

    if (state == STATE_STAGE_START) {
        state = STATE_PLAYING;
        update();
        return;
    }

    if (state != STATE_STAGE_CLEAR || !gm) {
        return;
    }

    resetFishingState(true);
    Player::instance().clearInputState();

    if (gm->stage >= Config::GameConfig::STAGE_COUNT) {
        gm->stageClear = false;
        gm->clearStageEntities();
        gm->victory = true;
        saveVictoryHighScore();
        state = STATE_VICTORY;
        update();
        return;
    }

    gm->stageClear = false;
    gm->stage++;
    bossEncounterShown = false;
    bossEncounterRemainingMs = 0;

    timer->stop();
    openShop();
    gm->resetStageRuntime();
    gm->saveAndQuit();
    timer->start(16);

    state = STATE_STAGE_START;
    update();
}

void GameWindow::startNewGame()
{
    gm->fileManager.deleteSave();

    Player::instance().reset();

    InventorySystem::instance().clearAll();
    InventorySystem::instance().initDefaultWeaponIfNeeded();

    delete gm;
    gm = new GameManager();

    resetFishingState(false);
    attackProjectiles.clear();
    victoryScoreSaved = false;
    testModeEnabled = false;
    Player::instance().testModeInfiniteCoins = false;
    bossEncounterShown = false;
    bossEncounterRemainingMs = 0;
    victoryButtonHover = -1;
    menuHoverIndex = -1;
    setCursor(Qt::ArrowCursor);

    promptButtonHover = false;
    state = STATE_STAGE_START;
    update();
}

void GameWindow::continueGame()
{
    if (!gm->fileManager.hasSave()) {
        return;
    }

    gm->loadSave();

    resetFishingState(false);
    attackProjectiles.clear();
    victoryScoreSaved = false;
    testModeEnabled = false;
    Player::instance().testModeInfiniteCoins = false;
    bossEncounterShown = gm->bossSpawned;
    bossEncounterRemainingMs = 0;
    victoryButtonHover = -1;
    menuHoverIndex = -1;
    setCursor(Qt::ArrowCursor);

    const int stageStart = Config::GameConfig::stageStartDistance(gm->stage);
    promptButtonHover = false;
    state = Player::instance().distance <= stageStart + 5
        ? STATE_STAGE_START
        : STATE_PLAYING;
    update();
}

QRect GameWindow::menuButtonRect(int index) const
{
    const int x = 445;
    const int y = 205 + index * 48;
    return QRect(x, y, 390, 52);
}

int GameWindow::menuButtonAt(const QPoint& pos) const
{
    for (int i = 0; i < 6; ++i) {
        if (i == 1 && !gm->fileManager.hasSave()) {
            continue;
        }
        if (menuButtonRect(i).contains(pos)) {
            return i;
        }
    }

    return -1;
}

QRect GameWindow::stagePromptButtonRect() const
{
    return QRect(438, 593, 404, 64);
}

QRect GameWindow::pauseMainMenuButtonRect() const
{
    return QRect(475, 438, 330, 64);
}

QRect GameWindow::victoryButtonRect(int index) const
{
    const qreal designW = !imgFinalVictoryBoard.isNull() ? imgFinalVictoryBoard.width() : 1672.0;
    const qreal designH = !imgFinalVictoryBoard.isNull() ? imgFinalVictoryBoard.height() : 941.0;
    auto mapRect = [&](qreal x, qreal y, qreal w, qreal h) {
        return QRect(qRound(x * 1280.0 / designW),
                     qRound(y * 720.0 / designH),
                     qRound(w * 1280.0 / designW),
                     qRound(h * 720.0 / designH));
    };

    switch (index) {
    case 0: return mapRect(428, 831, 212, 49);
    case 1: return mapRect(724, 831, 222, 49);
    case 2: return mapRect(1031, 831, 214, 49);
    default: return QRect();
    }
}

int GameWindow::victoryButtonAt(const QPoint& pos) const
{
    for (int i = 0; i < 3; ++i) {
        if (victoryButtonRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

void GameWindow::returnToMainMenu()
{
    if (!gm) return;

    Player::instance().clearInputState();
    resetFishingState(true);
    attackProjectiles.clear();
    hitFeedbacks.clear();
    floatingNotice.active = false;
    gm->saveAndQuit();

    pauseMainMenuHover = false;
    victoryButtonHover = -1;
    menuHoverIndex = -1;
    setCursor(Qt::ArrowCursor);
    state = STATE_MENU;
    update();
}

void GameWindow::resetRunAndReturnToMenu()
{
    Player::instance().reset();

    InventorySystem::instance().clearAll();
    InventorySystem::instance().initDefaultWeaponIfNeeded();

    resetFishingState(false);
    delete gm;
    gm = new GameManager();

    attackProjectiles.clear();
    hitFeedbacks.clear();
    floatingNotice.active = false;
    victoryScoreSaved = false;
    testModeEnabled = false;
    Player::instance().testModeInfiniteCoins = false;
    bossEncounterShown = false;
    bossEncounterRemainingMs = 0;
    victoryButtonHover = -1;
    pauseMainMenuHover = false;
    promptButtonHover = false;
    menuHoverIndex = -1;

    setCursor(Qt::ArrowCursor);
    state = STATE_MENU;
    update();
}

// ============================================================
// 暂停、胜利与失败
// ============================================================

void GameWindow::drawPaused(QPainter& p)
{
    p.fillRect(0, 0, 1280, 720, QColor(0, 0, 0, 120));
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 36, QFont::Bold));
    p.drawText(0, 280, 1280, 80, Qt::AlignCenter, "游戏暂停");
    p.setFont(QFont("Microsoft YaHei", 18));
    p.drawText(0, 370, 1280, 40, Qt::AlignCenter, "按 ESC 继续    按 H 打开航海图鉴    按 Q 保存退出");

    const QRect button = pauseMainMenuButtonRect();
    if (!imgWoodNoticeButton.isNull()) {
        p.save();
        p.setOpacity(pauseMainMenuHover ? 1.0 : 0.9);
        p.drawPixmap(button, imgWoodNoticeButton, imgWoodNoticeButton.rect());
        p.restore();
    }
    p.setPen(QColor(255, 232, 170));
    p.setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    p.drawText(button, Qt::AlignCenter, "保存并返回主页面");
}

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

void GameWindow::drawVictory(QPainter& p)
{
    Player& pl = Player::instance();
    const int stagesCleared = Config::GameConfig::STAGE_COUNT;
    const int score = gm->fileManager.calculateScore(
        qMin(gm->stage, Config::GameConfig::STAGE_COUNT),
        pl.fishTotalValue,
        pl.fishCaught,
        gm->killCount,
        pl.coins,
        pl.durability(),
        pl.stamina(),
        pl.gameSeconds
    );
    const QString grade = victoryGrade(score);

    int fishFound = 0;
    for (int i = 0; i < 12; ++i) {
        if (gm->fileManager.isFishDiscovered(i)) ++fishFound;
    }

    int enemyFound = 0;
    for (int i = 0; i < 5; ++i) {
        if (gm->fileManager.isEnemyDiscovered(i)) ++enemyFound;
    }

    int bossFound = 0;
    if (gm->fileManager.isBossDiscovered(0)) ++bossFound;
    if (gm->fileManager.isBossDiscovered(2)) ++bossFound;

    p.fillRect(0, 0, 1280, 720, QColor(18, 42, 58));
    if (!imgFinalVictoryBoard.isNull()) {
        p.drawPixmap(QRect(0, 0, 1280, 720), imgFinalVictoryBoard, imgFinalVictoryBoard.rect());
    } else if (!imgWoodNoticeBoard.isNull()) {
        p.drawPixmap(QRect(150, 38, 980, 655), imgWoodNoticeBoard, imgWoodNoticeBoard.rect());
    } else {
        p.fillRect(QRect(120, 54, 1040, 612), QColor(180, 126, 64));
    }

    auto drawText = [&](const QRect& rect, const QString& text, int pixelSize,
                        const QColor& color, bool bold = false,
                        Qt::Alignment flags = Qt::AlignCenter,
                        const QColor& shadow = QColor(76, 40, 14, 70)) {
        QFont font(QStringLiteral("Microsoft YaHei"));
        font.setPixelSize(pixelSize);
        font.setWeight(bold ? QFont::Bold : QFont::Normal);
        const int textFlags = static_cast<int>(flags) | Qt::TextWordWrap;
        while (pixelSize > 10) {
            QFontMetrics fm(font);
            QRect bound = fm.boundingRect(rect, textFlags, text);
            if (bound.width() <= rect.width() && bound.height() <= rect.height()) break;
            font.setPixelSize(--pixelSize);
        }
        p.setFont(font);
        if (shadow.alpha() > 0) {
            p.setPen(shadow);
            p.drawText(rect.translated(1, 1), textFlags, text);
        }
        p.setPen(color);
        p.drawText(rect, textFlags, text);
    };
    auto drawInkCenteredText = [&](const QRect& rect, const QString& text, int pixelSize,
                                   const QColor& color, bool bold = false,
                                   const QColor& shadow = QColor(0, 0, 0, 0)) {
        QFont font(QStringLiteral("Microsoft YaHei"));
        font.setWeight(bold ? QFont::Bold : QFont::Normal);
        QPainterPath path;
        QRectF bounds;
        while (pixelSize > 10) {
            font.setPixelSize(pixelSize);
            path = QPainterPath();
            path.addText(0, 0, font, text);
            bounds = path.boundingRect();
            if (bounds.width() <= rect.width() && bounds.height() <= rect.height()) break;
            --pixelSize;
        }
        const QPointF delta(rect.center().x() - bounds.center().x(),
                            rect.center().y() - bounds.center().y());
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        if (shadow.alpha() > 0) {
            p.fillPath(path.translated(delta + QPointF(1, 1)), shadow);
        }
        p.fillPath(path.translated(delta), color);
        p.restore();
    };

    auto drawRow = [&](const QRect& rect, const QString& label, const QString& value,
                       int labelSize = 18, int valueSize = 20) {
        const int labelWidth = static_cast<int>(rect.width() * 0.40);
        const int valueWidth = static_cast<int>(rect.width() * 0.40);
        drawText(QRect(rect.left(), rect.top(), labelWidth, rect.height()), label, labelSize,
                 QColor(102, 58, 26), true, Qt::AlignLeft | Qt::AlignVCenter,
                 QColor(0, 0, 0, 0));
        drawText(QRect(rect.right() - valueWidth + 1, rect.top(), valueWidth, rect.height()), value, valueSize,
                 QColor(53, 33, 18), true, Qt::AlignRight | Qt::AlignVCenter,
                 QColor(0, 0, 0, 0));
    };
    auto drawCenteredLines = [&](const QRect& rect, const QStringList& lines,
                                 int pixelSize = 17, int lineGap = 8) {
        if (lines.isEmpty()) return;
        QFont font(QStringLiteral("Microsoft YaHei"));
        font.setPixelSize(pixelSize);
        font.setWeight(QFont::Bold);
        p.setFont(font);
        QFontMetrics fm(font);
        const int lineHeight = fm.height();
        const int totalHeight = lineHeight * lines.size() + lineGap * (lines.size() - 1);
        int y = rect.center().y() - totalHeight / 2;
        for (const QString& line : lines) {
            drawText(QRect(rect.left() + 12, y, rect.width() - 24, lineHeight), line, pixelSize,
                     QColor(63, 38, 20), true, Qt::AlignCenter, QColor(0, 0, 0, 0));
            y += lineHeight + lineGap;
        }
    };

    const qreal designW = !imgFinalVictoryBoard.isNull() ? imgFinalVictoryBoard.width() : 1672.0;
    const qreal designH = !imgFinalVictoryBoard.isNull() ? imgFinalVictoryBoard.height() : 941.0;
    auto mapRect = [&](qreal x, qreal y, qreal w, qreal h) {
        return QRect(qRound(x * 1280.0 / designW),
                     qRound(y * 720.0 / designH),
                     qRound(w * 1280.0 / designW),
                     qRound(h * 720.0 / designH));
    };

    drawText(mapRect(560, 76, 548, 42), QStringLiteral("远航归来"), 30,
             QColor(86, 38, 11), true, Qt::AlignCenter, QColor(255, 244, 204, 60));
    drawText(mapRect(560, 122, 548, 24),
             QStringLiteral("总评分 %1 · 第 %2 关全域通关")
                 .arg(score)
                 .arg(Config::GameConfig::STAGE_COUNT),
             15, QColor(118, 72, 28), true, Qt::AlignCenter, QColor(255, 240, 190, 10));
    drawText(mapRect(430, 154, 810, 24),
             QStringLiteral("评分构成：通关30% · 时间22% · 捕鱼27% · 击杀17% · 生存4%"),
             12, QColor(104, 67, 34), true, Qt::AlignCenter, QColor(255, 240, 190, 8));
    drawText(mapRect(500, 181, 670, 22),
             QStringLiteral("S≥8500　A≥7200　B≥5800　C≥4300"),
             11, QColor(112, 73, 37), true, Qt::AlignCenter, QColor(255, 240, 190, 8));

    drawRow(mapRect(218, 288, 560, 34), QStringLiteral("通关海域"),
            QStringLiteral("%1/%2").arg(stagesCleared).arg(Config::GameConfig::STAGE_COUNT), 18, 21);
    drawRow(mapRect(218, 350, 560, 34), QStringLiteral("航海评价"),
            victoryGradeTitle(grade), 18, 21);
    drawRow(mapRect(218, 410, 560, 34), QStringLiteral("总航程"),
            QStringLiteral("%1 m").arg(pl.distance), 18, 21);
    drawRow(mapRect(218, 476, 560, 34), QStringLiteral("航行用时"),
            formatGameTime(pl.gameSeconds), 18, 21);

    drawText(mapRect(1066, 228, 344, 34), QStringLiteral("最终评级"), 20,
             QColor(97, 49, 18), true, Qt::AlignCenter, QColor(255, 236, 182, 40));
    drawInkCenteredText(mapRect(1069, 313, 312, 154), grade, 90, QColor(150, 43, 21), true);
    drawText(mapRect(1060, 490, 354, 32), victoryGradeTitle(grade), 20,
             QColor(77, 41, 17), true, Qt::AlignCenter, QColor(255, 234, 182, 32));

    drawInkCenteredText(mapRect(280, 573, 190, 30), QStringLiteral("战利品"), 18,
                        QColor(95, 47, 18), true);
    drawCenteredLines(mapRect(184, 654, 384, 132), {
        QStringLiteral("鱼获 %1 条").arg(pl.fishCaught),
        QStringLiteral("鱼获价值 %1").arg(pl.fishTotalValue),
        QStringLiteral("剩余金币 %1").arg(coinDisplayText(pl))
    }, 15, 10);

    drawInkCenteredText(mapRect(727, 573, 190, 30), QStringLiteral("战斗记录"), 18,
                        QColor(95, 47, 18), true);
    drawCenteredLines(mapRect(620, 654, 394, 132), {
        QStringLiteral("击败敌人 %1").arg(gm->killCount),
        QStringLiteral("Boss 击破 %1/2").arg(bossFound),
        QStringLiteral("船体 %1/%2  体力 %3/%4")
            .arg(pl.durability()).arg(pl.maxDurability)
            .arg(pl.stamina()).arg(pl.maxStamina)
    }, 15, 10);

    drawInkCenteredText(mapRect(1181, 573, 190, 30), QStringLiteral("图鉴发现"), 18,
                        QColor(95, 47, 18), true);
    drawCenteredLines(mapRect(1068, 654, 414, 132), {
        QStringLiteral("鱼类 %1/12").arg(fishFound),
        QStringLiteral("敌人 %1/5").arg(enemyFound),
        QStringLiteral("Boss %1/2").arg(bossFound)
    }, 15, 10);

    const QString labels[3] = {
        QStringLiteral("再来一局"),
        QStringLiteral("查看图鉴"),
        QStringLiteral("返回主菜单")
    };
    for (int i = 0; i < 3; ++i) {
        const QRect button = victoryButtonRect(i);
        if (victoryButtonHover == i && !imgWoodNoticeButton.isNull()) {
            p.save();
            p.setOpacity(0.86);
            p.drawPixmap(button.adjusted(-8, -9, 8, 9), imgWoodNoticeButton, imgWoodNoticeButton.rect());
            p.restore();
        }
        drawInkCenteredText(button.adjusted(8, 2, -8, -2), labels[i], 20,
                            victoryButtonHover == i ? QColor(255, 239, 178) : QColor(255, 224, 148),
                            true, QColor(56, 26, 8, 170));
    }
}

// ============================================================
// 捕鱼逻辑更新
// ============================================================

Fish* GameWindow::nearestFishInWeaponRange(const Weapon* weapon) const
{
    if (!weapon || !weapon->canFish() || weapon->isBroken()) {
        return nullptr;
    }

    Fish* nearest = nullptr;
    qreal nearestDist = 0.0;
    const QPointF playerPos = Player::instance().worldPos();

    for (auto f : gm->fish) {
        if (!f || f->caught || f->escaped) continue;
        if (!f->isNearPlayer(gm->playerX(), gm->playerY(), weapon->getRange())) continue;

        const qreal dx = f->x - playerPos.x();
        const qreal dy = f->y - playerPos.y();
        const qreal dist = dx * dx + dy * dy;
        if (!nearest || dist < nearestDist) {
            nearest = f;
            nearestDist = dist;
        }
    }

    return nearest;
}

void GameWindow::resetFishingState(bool releaseTarget)
{
    if (releaseTarget && targetFish) {
        targetFish->lockedForCatch = false;
    }

    isFishing = false;
    targetFish = nullptr;
    fishClickCount = 0;
    fishTimer = 0;
    calibrationTargetRatio = 0.5;
}

qreal GameWindow::calibrationMarkerRatio() const
{
    if (!targetFish || targetFish->catchTimeLimit <= 0) {
        return 0.5;
    }

    int sweepFrames = 112;
    switch (targetFish->type) {
    case Fish::SARDINE:
    case Fish::ANCHOVY:
        sweepFrames = 112;
        break;
    case Fish::CLOWNFISH:
    case Fish::TUNA:
        sweepFrames = 96;
        break;
    case Fish::MACKEREL:
    case Fish::SEA_BREAM:
        sweepFrames = 82;
        break;
    case Fish::DEEPSEAEEL:
    case Fish::LANTERNFISH:
    case Fish::GROUPER:
        sweepFrames = 64;
        break;
    case Fish::SWORDFISH_FISH:
    case Fish::KOI:
        sweepFrames = 50;
        break;
    case Fish::CRYSTAL_FISH:
        sweepFrames = 42;
        break;
    }
    const qreal phase = static_cast<qreal>(fishTimer % sweepFrames) / sweepFrames;
    return phase <= 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
}

qreal GameWindow::calibrationTargetCenterRatio() const
{
    return qBound<qreal>(0.12, calibrationTargetRatio, 0.88);
}

qreal GameWindow::calibrationWindowSize(bool perfect) const
{
    if (!targetFish) {
        return perfect ? 0.08 : 0.22;
    }

    qreal normal = 0.24;
    qreal perfectWindow = 0.085;
    switch (targetFish->type) {
    case Fish::SARDINE:
        normal = 0.28;
        perfectWindow = 0.10;
        break;
    case Fish::TUNA:
        normal = 0.24;
        perfectWindow = 0.085;
        break;
    case Fish::ANCHOVY:
        normal = 0.28;
        perfectWindow = 0.10;
        break;
    case Fish::CLOWNFISH:
        normal = 0.26;
        perfectWindow = 0.095;
        break;
    case Fish::MACKEREL:
    case Fish::SEA_BREAM:
        normal = 0.23;
        perfectWindow = 0.08;
        break;
    case Fish::DEEPSEAEEL:
        normal = 0.20;
        perfectWindow = 0.07;
        break;
    case Fish::LANTERNFISH:
    case Fish::GROUPER:
        normal = 0.19;
        perfectWindow = 0.068;
        break;
    case Fish::SWORDFISH_FISH:
    case Fish::KOI:
    case Fish::CRYSTAL_FISH:
        normal = 0.18;
        perfectWindow = 0.06;
        break;
    }

    return perfect ? perfectWindow : normal;
}

Config::FishingResult GameWindow::calibrationFishingResult() const
{
    const qreal distance = qAbs(
        calibrationMarkerRatio() - calibrationTargetCenterRatio());
    if (distance <= calibrationWindowSize(true)) {
        return Config::FishingResult::Perfect;
    }
    if (distance <= calibrationWindowSize(false)) {
        return Config::FishingResult::Normal;
    }
    return Config::FishingResult::Fail;
}

void GameWindow::finishFishing(Config::FishingResult result)
{
    if (!targetFish) return;

    Weapon* weapon = InventorySystem::instance().currentWeapon();
    Player& pl = Player::instance();

    auto consumeFishingStamina = [&]() {
        int cost = targetFish ? targetFish->staminaCost : 0;
        if (result == Config::FishingResult::Perfect) {
            cost = (cost + 1) / 2;
        }
        if (cost <= 0) {
            return;
        }
        if (!pl.consumeStamina(cost)) {
            pl.consumeStamina(pl.stamina());
        }
    };

    if (result == Config::FishingResult::Fail) {
        consumeFishingStamina();
        if (weapon && weapon->canFish()) {
            const bool wasBroken = weapon->isBroken();
            weapon->consumeFishingDurability(Config::FishingResult::Fail);
            notifyWeaponBrokenIfNeeded(weapon, wasBroken);
        }
        QPointF fleeDir = targetFish->position() - Player::instance().worldPos();
        qreal fleeLength = std::hypot(fleeDir.x(), fleeDir.y());
        if (fleeLength < 0.001) {
            fleeDir = QPointF(1.0, 0.0);
            fleeLength = 1.0;
        }
        const qreal fleeSpeed = dynamic_cast<RareFish*>(targetFish) ? 4.2 : 3.0;
        targetFish->vx = static_cast<float>(fleeDir.x() / fleeLength * fleeSpeed);
        targetFish->vy = static_cast<float>(fleeDir.y() / fleeLength * fleeSpeed);
        targetFish->fleeing = true;
        targetFish->fleeCooldown = 150;
        targetFish->lifeTimer = 0;
        resetFishingState(true);
        return;
    }

    targetFish->caught = true;
    consumeFishingStamina();

    int fishValue = (int)(targetFish->value * WeatherSystem::instance().currentFishValueBonus());
    pl.coins += fishValue;
    pl.fishCaught++;
    pl.fishTotalValue += fishValue;

    const char* fishNameForLog = "未知鱼";
    int fishId = 0;
    switch (targetFish->type) {
    case Fish::SARDINE:
        fishNameForLog = "沙丁鱼";
        fishId = 0;
        break;
    case Fish::TUNA:
        fishNameForLog = "金枪鱼";
        fishId = 1;
        break;
    case Fish::DEEPSEAEEL:
        fishNameForLog = "深海鳗";
        fishId = 2;
        break;
    case Fish::SWORDFISH_FISH:
        fishNameForLog = "金鱼";
        fishId = 3;
        break;
    case Fish::ANCHOVY:
        fishNameForLog = u8"\u94f6\u9cca\u9c7c";
        fishId = 4;
        break;
    case Fish::CLOWNFISH:
        fishNameForLog = u8"\u5c0f\u4e11\u9c7c";
        fishId = 5;
        break;
    case Fish::MACKEREL:
        fishNameForLog = u8"\u84dd\u9cb5";
        fishId = 6;
        break;
    case Fish::SEA_BREAM:
        fishNameForLog = u8"\u771f\u9cb7";
        fishId = 7;
        break;
    case Fish::LANTERNFISH:
        fishNameForLog = u8"\u706f\u7b3c\u9c7c";
        fishId = 8;
        break;
    case Fish::GROUPER:
        fishNameForLog = u8"\u77f3\u6591\u9c7c";
        fishId = 9;
        break;
    case Fish::KOI:
        fishNameForLog = u8"\u9526\u9ca4";
        fishId = 10;
        break;
    case Fish::CRYSTAL_FISH:
        fishNameForLog = u8"\u6676\u9cde\u9c7c";
        fishId = 11;
        break;
    }
    gm->fileManager.markFishDiscovered(fishId, fishNameForLog);

    pl.restoreStamina(targetFish->staminaGain);

    if (weapon && weapon->canFish()) {
        const bool wasBroken = weapon->isBroken();
        weapon->consumeFishingDurability(result);
        notifyWeaponBrokenIfNeeded(weapon, wasBroken);
    }

    resetFishingState(true);
}

void GameWindow::updateFishing()
{
    if (!isFishing || !targetFish) return;
    if (targetFish->caught || targetFish->escaped) {
        resetFishingState(false);
        return;
    }

    targetFish->lockedForCatch = true;

    fishTimer++;

    Weapon* weapon = InventorySystem::instance().currentWeapon();

    // 捕捉超时：鱼逃跑，并按 Fail 结果消耗捕鱼工具耐久
    if (fishTimer >= targetFish->catchTimeLimit) {
        finishFishing(Config::FishingResult::Fail);
        return;
    }

    const Config::FishingMode mode = weapon ? weapon->getFishingMode() : Config::FishingMode::QTE;

    if (mode == Config::FishingMode::Calibration) {
        if (fishClickCount > 0) {
            finishFishing(calibrationFishingResult());
        }
        return;
    }

    if (fishClickCount >= targetFish->catchRequired) {
        const Config::FishingResult result =
            (fishTimer < targetFish->catchTimeLimit / 2)
            ? Config::FishingResult::Perfect
            : Config::FishingResult::Normal;
        finishFishing(result);
    }
}

// ============================================================
// 键盘与鼠标输入控制 (完全重构)
// ============================================================

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    if (state == STATE_INTRO) {
        state = STATE_MENU; update();
        return;
    }

    if (state == STATE_MENU) {
        if (event->key() == Qt::Key_N) {
            startNewGame();
        }
        else if (event->key() == Qt::Key_C && gm->fileManager.hasSave()) {
            continueGame();
        }
        return;
    }

    if (event->key() == Qt::Key_O && state != STATE_DEFEAT && state != STATE_VICTORY) {
        toggleTestMode();
        return;
    }

    if (event->key() == Qt::Key_P && testModeEnabled &&
        state != STATE_MENU && state != STATE_DEFEAT && state != STATE_VICTORY) {
        openTestModeShop();
        return;
    }

    if (state == STATE_STAGE_START || state == STATE_STAGE_CLEAR) {
        if (event->key() == Qt::Key_Space ||
            event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter) {
            confirmStagePrompt();
        }
        return;
    }

    if (state == STATE_DEFEAT || state == STATE_VICTORY) {
        if (state == STATE_VICTORY && event->key() == Qt::Key_H) {
            openEncyclopedia();
            update();
            return;
        }
        if (state == STATE_VICTORY && event->key() == Qt::Key_N) {
            startNewGame();
            return;
        }
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter ||
            (state == STATE_DEFEAT && event->key() == Qt::Key_N)) {
            resetRunAndReturnToMenu();
        }
        return;
    }

    if (state == STATE_PAUSED) {
        if (event->key() == Qt::Key_Escape) {
            Player::instance().clearInputState();
            pauseMainMenuHover = false;
            setCursor(Qt::ArrowCursor);
            state = STATE_PLAYING;
        }
        else if (event->key() == Qt::Key_Q) { gm->saveAndQuit(); close(); }
        else if (event->key() == Qt::Key_H) { openEncyclopedia(); update(); }
        else if (event->key() == Qt::Key_M) { returnToMainMenu(); }
        return;
    }

    if (state == STATE_PLAYING) {
        Player::instance().keyPress(event);

        switch (event->key()) {
        case Qt::Key_Space: // 极限冲刺
            Player::instance().triggerDash();
            break;
        case Qt::Key_E: // 新增：震荡波救场
            if (Player::instance().canShock()) {
                Player::instance().triggerShock();
                gm->triggerShockWave();
            }
            break;
        case Qt::Key_B:
            if (isFishing) {
                break;
            }
            Player::instance().clearInputState();
            timer->stop();
            openBackpack();
            timer->start(16);
            break;
        case Qt::Key_H:
            if (isFishing) {
                break;
            }
            Player::instance().clearInputState();
            timer->stop();
            openEncyclopedia();
            timer->start(16);
            break;
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
            if (!isFishing) {
                const int slot = event->key() - Qt::Key_1;
                if (!InventorySystem::instance().selectQuickWeaponSlot(slot)) {
                    useQuickItemSlot(slot);
                }
            }
            break;
        case Qt::Key_Escape:
            Player::instance().clearInputState();
            state = STATE_PAUSED;
            break;
        case Qt::Key_Q: gm->saveAndQuit(); close(); break;
        default: break;
        }
    }
}

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (state == STATE_PLAYING) {
        Player::instance().keyRelease(event);
    }
    else {
        Player::instance().clearInputState();
    }
}

void GameWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (state == STATE_MENU) {
        const int hoverIndex = menuButtonAt(event->position().toPoint());
        if (hoverIndex != menuHoverIndex) {
            menuHoverIndex = hoverIndex;
            setCursor(menuHoverIndex >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
            update();
        }
        return;
    }

    if (state == STATE_STAGE_START || state == STATE_STAGE_CLEAR) {
        const bool hover = stagePromptButtonRect().contains(event->position().toPoint());
        if (hover != promptButtonHover) {
            promptButtonHover = hover;
            setCursor(promptButtonHover ? Qt::PointingHandCursor : Qt::ArrowCursor);
            update();
        }
        return;
    }

    if (state == STATE_PAUSED) {
        const bool hover = pauseMainMenuButtonRect().contains(event->position().toPoint());
        if (hover != pauseMainMenuHover) {
            pauseMainMenuHover = hover;
            setCursor(hover ? Qt::PointingHandCursor : Qt::ArrowCursor);
            update();
        }
        return;
    }

    if (state == STATE_VICTORY) {
        const int hoverIndex = victoryButtonAt(event->position().toPoint());
        if (hoverIndex != victoryButtonHover) {
            victoryButtonHover = hoverIndex;
            setCursor(victoryButtonHover >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
            update();
        }
        return;
    }

    if (menuHoverIndex != -1 || promptButtonHover || pauseMainMenuHover) {
        menuHoverIndex = -1;
        promptButtonHover = false;
        pauseMainMenuHover = false;
        setCursor(Qt::ArrowCursor);
    }
}

void GameWindow::leaveEvent(QEvent* event)
{
    if (menuHoverIndex != -1) {
        menuHoverIndex = -1;
        setCursor(Qt::ArrowCursor);
        update();
    }

    if (promptButtonHover) {
        promptButtonHover = false;
        setCursor(Qt::ArrowCursor);
        update();
    }

    if (pauseMainMenuHover) {
        pauseMainMenuHover = false;
        setCursor(Qt::ArrowCursor);
        update();
    }

    if (victoryButtonHover != -1) {
        victoryButtonHover = -1;
        setCursor(Qt::ArrowCursor);
        update();
    }

    QWidget::leaveEvent(event);
}

// 鼠标左键：统一接管捕鱼与武器射击
void GameWindow::mousePressEvent(QMouseEvent* event)
{
    if (state == STATE_MENU) {
        if (event->button() != Qt::LeftButton) return;

        const int buttonIndex = menuButtonAt(event->position().toPoint());
        if (buttonIndex == 0) {
            startNewGame();
        }
        else if (buttonIndex == 1) {
            continueGame();
        }
        else if (buttonIndex == 2) {
            openEncyclopedia();
            menuHoverIndex = -1;
            setCursor(Qt::ArrowCursor);
            update();
        }
        else if (buttonIndex == 3) {
            GameUi::showOperationGuide(this);
            menuHoverIndex = -1;
            setCursor(Qt::ArrowCursor);
            update();
        }
        else if (buttonIndex == 4) {
            GameUi::showWoodMessage(this,
                                    QStringLiteral("\u6e38\u620f\u8bbe\u7f6e"),
                                    QStringLiteral("\u8bbe\u7f6e\u754c\u9762\u540e\u7eed\u63a5\u5165\u3002"));
        }
        else if (buttonIndex == 5) {
            close();
        }
        return;
    }

    if (state == STATE_STAGE_START || state == STATE_STAGE_CLEAR) {
        if (event->button() == Qt::LeftButton &&
            stagePromptButtonRect().contains(event->position().toPoint())) {
            confirmStagePrompt();
        }
        return;
    }

    if (state == STATE_PAUSED) {
        if (event->button() == Qt::LeftButton &&
            pauseMainMenuButtonRect().contains(event->position().toPoint())) {
            returnToMainMenu();
        }
        return;
    }

    if (state == STATE_VICTORY) {
        if (event->button() != Qt::LeftButton) return;
        const int buttonIndex = victoryButtonAt(event->position().toPoint());
        if (buttonIndex == 0) {
            startNewGame();
        }
        else if (buttonIndex == 1) {
            openEncyclopedia();
            update();
        }
        else if (buttonIndex == 2) {
            resetRunAndReturnToMenu();
        }
        return;
    }

    if (state != STATE_PLAYING) return;
    if (event->button() != Qt::LeftButton) return;

    QPointF clickPos = event->position();
    int worldX = (int)clickPos.x() + gm->cameraX;
    int worldY = (int)clickPos.y();

    // 0. 正在捕鱼中：点击目标鱼附近视为 QTE 连击
    if (isFishing && targetFish) {
        fishClickCount++;
        spawnHitFeedback(targetFish->position());
        return;
    }

    Weapon* weapon = InventorySystem::instance().currentWeapon();
    if (!weapon) return;
    if (weapon->isBroken()) {
        showFloatingNotice(QStringLiteral("\u88c5\u5907\u5df2\u635f\u574f"),
                           QStringLiteral("\u8bf7\u56de\u6e2f\u4fee\u590d\uff0c\u6216\u4f7f\u7528\u7d27\u6025\u88c5\u5907\u4fee\u7406\u5de5\u5177\u3002"));
        return;
    }
    const bool wasWeaponBroken = weapon->isBroken();

    // 1. 优先尝试攻击敌人
    // 如果命中敌人，则本次点击结束，不再进入捕鱼逻辑。
    bool hitEnemy = false;
    QPointF hitFeedbackWorld(worldX, worldY);

    if (weapon->canAttack() && gm->canAttemptAttack(weapon)) {
        if (isGunWeapon(weapon)) {
            spawnGunProjectiles(QPointF(worldX, worldY), weapon);
        }
        hitEnemy = gm->attackAt(worldX, worldY, weapon);
        if (hitEnemy && isHarpoonWeapon(weapon)) {
            spawnHarpoonProjectile(QPointF(worldX, worldY), weapon);
            if (!attackProjectiles.isEmpty()) {
                hitFeedbackWorld = attackProjectiles.last().endWorld;
            }
        }
    }

    if (hitEnemy) {
        spawnHitFeedback(hitFeedbackWorld);
        notifyWeaponBrokenIfNeeded(weapon, wasWeaponBroken);
        return;
    }

    // 2. 如果没有命中敌人，再尝试捕鱼
    // 这样可以避免鱼叉同一次点击既打中敌人又开始捕鱼。
    if (weapon->canFish() && !isFishing) {
        Fish* nearest = nearestFishInWeaponRange(weapon);
        if (nearest) {
            targetFish = nearest;
            targetFish->lockedForCatch = true;
            isFishing = true;
            fishClickCount = 0;
            fishTimer = 0;
            const qreal normalHalf = calibrationWindowSize(false);
            const qreal margin = qBound<qreal>(0.12, normalHalf + 0.035, 0.34);
            calibrationTargetRatio = margin +
                QRandomGenerator::global()->generateDouble() * (1.0 - margin * 2.0);
            return;
        }
    }
}
