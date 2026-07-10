#include "BackpackDialog.h"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <algorithm>
#include <cmath>

#include "GameConfig.h"
#include "GameUiDialog.h"
#include "Player.h"
#include "Weapon.h"
#include "weathersystem.h"

namespace {
constexpr QRect kTopStatusRect(84, 14, 1128, 58);
constexpr QRect kMainRect(136, 92, 1008, 588);
constexpr QRect kTitleRect(400, 88, 480, 84);
constexpr QRect kInfoRect(282, 182, 716, 40);
constexpr QRect kEquipTabRect(250, 236, 190, 44);
constexpr QRect kItemTabRect(452, 236, 190, 44);
constexpr QRect kListPanelRect(202, 282, 378, 334);
constexpr QRect kDetailPanelRect(596, 282, 512, 334);
constexpr QRect kEquipButtonRect(202, 620, 170, 54);
constexpr QRect kUseButtonRect(386, 620, 170, 54);
constexpr QRect kSlotButtonRect(570, 620, 170, 54);
constexpr QRect kDiscardButtonRect(754, 620, 170, 54);
constexpr QRect kCloseButtonRect(938, 620, 170, 54);
constexpr QRect kFooterRect(390, 684, 500, 32);
constexpr int kVisibleRows = 5;
constexpr int kRowHeight = 62;

QString coinDisplayText(const Player& player)
{
    return player.testModeInfiniteCoins
        ? QStringLiteral("\u221e")
        : QString::number(player.coins);
}

QString firstAvailableFont(const QStringList& candidates)
{
    const QStringList families = QFontDatabase::families();
    for (const QString& candidate : candidates) {
        for (const QString& family : families) {
            if (family.compare(candidate, Qt::CaseInsensitive) == 0) {
                return family;
            }
        }
    }
    return QStringLiteral("Microsoft YaHei UI");
}

QFont uiFont(int size, int weight = QFont::Normal)
{
    static const QString family = firstAvailableFont({
        QStringLiteral("Microsoft YaHei UI"),
        QStringLiteral("Microsoft YaHei"),
        QStringLiteral("Noto Sans CJK SC"),
        QStringLiteral("Source Han Sans SC"),
        QStringLiteral("SimHei"),
        QStringLiteral("YouYuan"),
        QStringLiteral("SimSun")
    });
    QFont font(family, size, weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    return font;
}

QFont titleFont(int size, int weight = QFont::Bold)
{
    static const QString family = firstAvailableFont({
        QStringLiteral("STXinwei"),
        QStringLiteral("FZShuTi"),
        QStringLiteral("STKaiti"),
        QStringLiteral("Microsoft YaHei UI")
    });
    QFont font(family, size, weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    return font;
}

QString weatherName()
{
    WeatherSystem& weather = WeatherSystem::instance();
    switch (weather.currentWeather()) {
    case WeatherType::SUNNY: return QStringLiteral("晴朗");
    case WeatherType::FOG: return QStringLiteral("雾天");
    case WeatherType::STORM:
        return weather.rainLevel() == 1
            ? QStringLiteral("小雨")
            : (weather.rainLevel() == 2 ? QStringLiteral("中雨") : QStringLiteral("暴雨"));
    }
    return QStringLiteral("晴朗");
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

int durabilityPercent(const Weapon* weapon)
{
    if (!weapon || weapon->getMaxDur() <= 0) return 0;
    if (weapon->isInfiniteDurability()) return 100;
    return std::clamp(weapon->getCurrentDur() * 100 / weapon->getMaxDur(), 0, 100);
}

QString weaponDurabilityText(const Weapon* weapon)
{
    if (!weapon) return QStringLiteral("-");
    return weapon->isInfiniteDurability()
        ? QStringLiteral("\u221e")
        : QStringLiteral("%1/%2").arg(weapon->getCurrentDur()).arg(weapon->getMaxDur());
}

QString capacityText(int current, int max)
{
    return max >= 999 ? QStringLiteral("%1/∞").arg(current)
                      : QStringLiteral("%1/%2").arg(current).arg(max);
}

QString weaponDescription(const Weapon* weapon)
{
    if (!weapon) return {};
    const QString type = QString::fromStdString(weapon->getTypeCode());
    if (type == QStringLiteral("Rod")) {
        return QStringLiteral("稳固耐用的基础钓具，适合近海稳定捕捞。");
    }
    if (type == QStringLiteral("Net")) {
        return QStringLiteral("适合捕捞普通鱼群，节奏更快但需要靠近目标。");
    }
    if (type == QStringLiteral("Harpoon")) {
        return QStringLiteral("兼顾捕鱼与防身，适合危险海域的灵活作战。");
    }
    if (type == QStringLiteral("Pistol")) {
        return QStringLiteral("轻便的远程武器，适合在敌人靠近前压制威胁。");
    }
    if (type == QStringLiteral("Shotgun")) {
        return QStringLiteral("近距离火力强劲，但耐久和使用节奏需要谨慎管理。");
    }
    return QStringLiteral("船舱装备，可根据航行状况灵活切换使用。");
}

void drawRivet(QPainter& p, const QPoint& center, int radius = 5)
{
    p.save();
    QRadialGradient rg(center.x() - radius / 3.0, center.y() - radius / 3.0, radius * 1.4);
    rg.setColorAt(0.0, QColor(255, 231, 139));
    rg.setColorAt(0.42, QColor(200, 134, 35));
    rg.setColorAt(1.0, QColor(80, 42, 13));
    p.setPen(QPen(QColor(52, 27, 8, 190), 1));
    p.setBrush(rg);
    p.drawEllipse(center, radius, radius);
    p.setPen(QPen(QColor(62, 31, 8, 150), 1));
    p.drawLine(center.x() - radius + 2, center.y(), center.x() + radius - 2, center.y());
    p.restore();
}

void drawBrassFrame(QPainter& p, const QRect& rect, int radius = 6, int lineWidth = 3)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(45, 21, 6, 220), lineWidth + 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect.adjusted(2, 3, -2, -2), radius, radius);

    QLinearGradient edge(rect.topLeft(), rect.bottomLeft());
    edge.setColorAt(0.0, QColor(255, 210, 82));
    edge.setColorAt(0.45, QColor(157, 91, 18));
    edge.setColorAt(1.0, QColor(83, 43, 10));
    p.setPen(QPen(QBrush(edge), lineWidth));
    p.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius, radius);

    p.setPen(QPen(QColor(255, 232, 128, 120), 1));
    p.drawRoundedRect(rect.adjusted(lineWidth + 3, lineWidth + 3, -lineWidth - 3, -lineWidth - 3),
                      std::max(2, radius - 2), std::max(2, radius - 2));

    p.setPen(QPen(QColor(255, 226, 122, 95), 1));
    p.drawLine(rect.left() + radius + 5, rect.top() + 3, rect.right() - radius - 6, rect.top() + 3);
    p.drawLine(rect.left() + 3, rect.top() + radius + 4, rect.left() + 3, rect.bottom() - radius - 6);
    p.setPen(QPen(QColor(54, 24, 6, 120), 1));
    p.drawLine(rect.left() + radius + 8, rect.bottom() - 3, rect.right() - radius - 8, rect.bottom() - 3);
    p.drawLine(rect.right() - 3, rect.top() + radius + 8, rect.right() - 3, rect.bottom() - radius - 8);

    QRect wear = rect.adjusted(lineWidth + 2, lineWidth + 2, -lineWidth - 2, -lineWidth - 2);
    p.setClipRect(rect.adjusted(1, 1, -1, -1));
    for (int i = 0; i < 12; ++i) {
        const int x = rect.left() + 12 + (i * 53) % std::max(1, rect.width() - 28);
        const int y = (i % 2 == 0) ? rect.top() + 5 + (i % 3) : rect.bottom() - 6 - (i % 3);
        const int len = 10 + (i % 4) * 6;
        p.setPen(QPen(QColor(83, 43, 10, 82), 1));
        p.drawLine(x, y, std::min(rect.right() - 9, x + len), y + ((i % 3) - 1));
        p.setPen(QPen(QColor(255, 219, 119, 42), 1));
        p.drawLine(x + 1, y - 1, std::min(rect.right() - 8, x + len - 2), y - 1);
    }
    for (int i = 0; i < 8; ++i) {
        const int y = rect.top() + 12 + (i * 41) % std::max(1, rect.height() - 28);
        const int x = (i % 2 == 0) ? rect.left() + 5 + (i % 3) : rect.right() - 6 - (i % 3);
        p.setPen(QPen(QColor(73, 38, 9, 72), 1));
        p.drawLine(x, y, x + ((i % 2 == 0) ? 1 : -1), std::min(rect.bottom() - 9, y + 14 + (i % 3) * 4));
    }
    p.setClipping(false);

    p.setPen(QPen(QColor(58, 91, 56, 56), 1));
    p.drawPoint(wear.left() + 6, wear.top() + 5);
    p.drawPoint(wear.right() - 8, wear.top() + 8);
    p.drawPoint(wear.left() + 9, wear.bottom() - 7);
    p.drawPoint(wear.right() - 10, wear.bottom() - 6);
    p.restore();
}

void drawWoodPanel(QPainter& p, const QRect& rect, bool ornate = true)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(4, 2, 1, 120));
    p.drawRoundedRect(rect.translated(8, 9), 8, 8);

    QLinearGradient body(rect.topLeft(), rect.bottomLeft());
    body.setColorAt(0.0, QColor(102, 61, 30));
    body.setColorAt(0.50, QColor(67, 38, 19));
    body.setColorAt(1.0, QColor(43, 24, 13));
    p.setBrush(body);
    p.setPen(QPen(QColor(24, 12, 6), 4));
    p.drawRoundedRect(rect, 8, 8);

    const int inset = rect.height() < 64 ? 8 : 18;
    QRect inner = rect.adjusted(inset, inset, -inset, -inset);
    if (inner.isValid() && inner.width() > 24 && inner.height() > 8) {
        p.setClipRect(inner);
        for (int y = inner.top() + std::min(30, std::max(10, inner.height() / 3)); y < inner.bottom(); y += 56) {
            p.setPen(QPen(QColor(19, 9, 4, 72), 1));
            p.drawLine(inner.left() + 12, y, inner.right() - 12, y + ((y / 56) % 3 - 1));
            p.setPen(QPen(QColor(157, 92, 41, 34), 1));
            p.drawLine(inner.left() + 16, y + 6, inner.right() - 16, y + 5);
        }
        for (int i = 0; i < 5; ++i) {
            const int x = inner.left() + 48 + (i * 137) % std::max(1, inner.width() - 120);
            const int y = inner.top() + 12 + (i * 71) % std::max(1, inner.height() - 24);
            QRect knot(x, y, 62 + (i % 3) * 14, 14 + (i % 2) * 5);
            p.setPen(QPen(QColor(18, 8, 3, 38), 1));
            p.drawArc(knot, 190 * 16, 150 * 16);
            p.setPen(QPen(QColor(145, 82, 36, 30), 1));
            p.drawArc(knot.adjusted(8, 4, -8, -4), 180 * 16, 160 * 16);
        }
        p.setClipping(false);
    }

    drawBrassFrame(p, rect, 8, 3);
    if (ornate) {
        const QVector<QRect> plates = {
            QRect(rect.left() + 8, rect.top() + 8, 72, 32),
            QRect(rect.right() - 79, rect.top() + 8, 72, 32),
            QRect(rect.left() + 8, rect.bottom() - 39, 72, 32),
            QRect(rect.right() - 79, rect.bottom() - 39, 72, 32),
        };
        for (const QRect& plate : plates) {
            QLinearGradient brass(plate.topLeft(), plate.bottomLeft());
            brass.setColorAt(0.0, QColor(207, 137, 32));
            brass.setColorAt(1.0, QColor(82, 43, 9));
            p.setBrush(brass);
            p.setPen(QPen(QColor(244, 183, 61), 2));
            p.drawRoundedRect(plate, 4, 4);
            drawRivet(p, QPoint(plate.left() + 13, plate.top() + 12), 4);
            drawRivet(p, QPoint(plate.right() - 13, plate.bottom() - 12), 4);
        }
    }
    p.restore();
}

void drawParchmentPanel(QPainter& p, const QRect& rect, bool ruled = false)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(32, 12, 3, 42));
    p.drawRoundedRect(rect.translated(4, 5), 5, 5);

    QLinearGradient paper(rect.topLeft(), rect.bottomRight());
    paper.setColorAt(0.0, QColor(248, 224, 174));
    paper.setColorAt(0.40, QColor(241, 210, 153));
    paper.setColorAt(0.72, QColor(228, 188, 121));
    paper.setColorAt(1.0, QColor(211, 160, 88));
    p.setBrush(paper);
    p.setPen(QPen(QColor(111, 65, 24), 2));
    p.drawRoundedRect(rect, 5, 5);
    p.setPen(QPen(QColor(180, 111, 39, 170), 1));
    p.drawRoundedRect(rect.adjusted(5, 5, -5, -5), 3, 3);
    p.setPen(QPen(QColor(255, 239, 192, 96), 1));
    p.drawRoundedRect(rect.adjusted(13, 13, -13, -13), 2, 2);
    p.setPen(QPen(QColor(255, 247, 208, 96), 1));
    p.drawLine(rect.left() + 12, rect.top() + 7, rect.right() - 18, rect.top() + 7);
    p.drawLine(rect.left() + 7, rect.top() + 12, rect.left() + 7, rect.bottom() - 18);
    p.setPen(QPen(QColor(122, 68, 21, 72), 1));
    p.drawLine(rect.left() + 16, rect.bottom() - 7, rect.right() - 13, rect.bottom() - 7);
    p.drawLine(rect.right() - 7, rect.top() + 15, rect.right() - 7, rect.bottom() - 16);

    p.setClipRect(rect.adjusted(2, 2, -2, -2));
    QRadialGradient softLight(rect.center(), std::max(rect.width(), rect.height()) * 0.62);
    softLight.setColorAt(0.0, QColor(255, 244, 207, 40));
    softLight.setColorAt(0.55, QColor(255, 235, 176, 12));
    softLight.setColorAt(1.0, QColor(139, 78, 22, 16));
    p.setBrush(softLight);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(rect.center()), rect.width() * 0.62, rect.height() * 0.58);

    QLinearGradient topBurn(rect.topLeft(), QPoint(rect.left(), rect.top() + 42));
    topBurn.setColorAt(0.0, QColor(124, 72, 22, 34));
    topBurn.setColorAt(1.0, QColor(64, 31, 9, 0));
    p.fillRect(QRect(rect.left(), rect.top(), rect.width(), 42), topBurn);
    QLinearGradient bottomBurn(QPoint(rect.left(), rect.bottom() - 46), rect.bottomLeft());
    bottomBurn.setColorAt(0.0, QColor(56, 28, 8, 0));
    bottomBurn.setColorAt(1.0, QColor(104, 55, 16, 44));
    p.fillRect(QRect(rect.left(), rect.bottom() - 46, rect.width(), 47), bottomBurn);
    QLinearGradient leftBurn(rect.topLeft(), rect.topLeft() + QPoint(30, 0));
    leftBurn.setColorAt(0.0, QColor(111, 60, 18, 34));
    leftBurn.setColorAt(1.0, QColor(57, 27, 7, 0));
    p.fillRect(QRect(rect.left(), rect.top(), 30, rect.height()), leftBurn);
    QLinearGradient rightBurn(rect.topRight() - QPoint(30, 0), rect.topRight());
    rightBurn.setColorAt(0.0, QColor(57, 27, 7, 0));
    rightBurn.setColorAt(1.0, QColor(111, 60, 18, 34));
    p.fillRect(QRect(rect.right() - 30, rect.top(), 31, rect.height()), rightBurn);

    auto cornerBurn = [&](const QPointF& center, qreal rx, qreal ry, int alpha) {
        QRadialGradient burn(center, std::max(rx, ry));
        burn.setColorAt(0.0, QColor(64, 30, 8, alpha));
        burn.setColorAt(0.68, QColor(76, 36, 10, alpha / 4));
        burn.setColorAt(1.0, QColor(76, 36, 10, 0));
        p.setBrush(burn);
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, rx, ry);
    };
    cornerBurn(rect.topLeft(), 64, 48, 38);
    cornerBurn(rect.topRight(), 74, 56, 34);
    cornerBurn(rect.bottomLeft(), 86, 58, 42);
    cornerBurn(rect.bottomRight(), 70, 52, 38);

    p.setPen(Qt::NoPen);
    for (int i = 0; i < 18; ++i) {
        const int x = rect.left() + 18 + (i * 47 + i * i * 11) % std::max(1, rect.width() - 36);
        const int y = rect.top() + 16 + (i * 31 + i * i * 7) % std::max(1, rect.height() - 32);
        const int rx = 3 + (i % 5) * 2;
        const int ry = 2 + ((i + 2) % 4);
        p.setBrush(QColor(113, 68, 25, 7 + (i % 4) * 3));
        p.drawEllipse(QPoint(x, y), rx, ry);
    }
    for (int i = 0; i < 42; ++i) {
        const int x = rect.left() + 16 + (i * 29 + i * i * 7) % std::max(1, rect.width() - 32);
        const int y = rect.top() + 14 + (i * 17 + i * i * 3) % std::max(1, rect.height() - 28);
        const int len = 7 + (i % 5) * 5;
        p.setPen(QPen(QColor(142, 86, 29, 12 + (i % 3) * 4), 1));
        p.drawLine(x, y, std::min(rect.right() - 14, x + len), y + ((i % 3) - 1));
    }
    for (int i = 0; i < 26; ++i) {
        const int x = rect.left() + 18 + (i * 43 + i * i) % std::max(1, rect.width() - 36);
        const int y = rect.top() + 16 + (i * 31 + i * i * 5) % std::max(1, rect.height() - 32);
        p.setPen(QPen(QColor(255, 243, 204, 18 + (i % 2) * 8), 1));
        p.drawLine(x, y, std::min(rect.right() - 12, x + 5 + (i % 4) * 4), y - 1);
    }
    for (int i = 0; i < 7; ++i) {
        const int x = rect.left() + 34 + (i * 61) % std::max(1, rect.width() - 74);
        const int y = rect.top() + 28 + (i * 43) % std::max(1, rect.height() - 62);
        QRect stain(x, y, 48 + (i % 3) * 16, 13 + (i % 2) * 8);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(116, 67, 19, 8 + (i % 3) * 3));
        p.drawEllipse(stain);
        p.setPen(QPen(QColor(151, 95, 34, 16), 1));
        p.drawArc(stain.adjusted(4, 2, -4, -2), 10 * 16, 160 * 16);
    }

    for (int i = 0; i < 5; ++i) {
        const int x = rect.left() + rect.width() / 5 + i * std::max(18, rect.width() / 8);
        const int top = rect.top() + 22 + (i % 2) * 9;
        const int bottom = rect.bottom() - 24 - ((i + 1) % 3) * 8;
        p.setPen(QPen(QColor(129, 78, 25, 13), 1));
        p.drawLine(x, top, x + ((i % 2) ? 2 : -2), bottom);
        p.setPen(QPen(QColor(255, 240, 196, 17), 1));
        p.drawLine(x + 2, top + 2, x + 3 + ((i % 2) ? 2 : -2), bottom - 2);
    }

    p.setPen(QPen(QColor(126, 78, 29, 26), 1));
    for (int i = 0; i < 22; ++i) {
        const int y = rect.top() + 15 + (i * 19) % std::max(1, rect.height() - 30);
        const int x = rect.left() + 13 + (i * 37) % std::max(1, rect.width() - 84);
        const int len = 30 + (i * 17) % 58;
        p.drawLine(x, y, std::min(rect.right() - 14, x + len), y + ((i % 3) - 1));
    }
    p.setPen(QPen(QColor(255, 238, 188, 32), 1));
    for (int i = 0; i < 14; ++i) {
        const int x = rect.left() + 18 + (i * 41) % std::max(1, rect.width() - 52);
        const int y = rect.top() + 18 + (i * 29) % std::max(1, rect.height() - 44);
        p.drawLine(x, y, std::min(rect.right() - 16, x + 18 + (i % 4) * 8), y - 1);
    }
    p.setClipping(false);

    p.setPen(QPen(QColor(107, 58, 17, 70), 1));
    const QVector<QLine> cracks = {
        QLine(rect.left() + 16, rect.top() + 21, rect.left() + 34, rect.top() + 27),
        QLine(rect.right() - 42, rect.top() + 18, rect.right() - 22, rect.top() + 14),
        QLine(rect.left() + 18, rect.bottom() - 24, rect.left() + 39, rect.bottom() - 18),
        QLine(rect.right() - 58, rect.bottom() - 18, rect.right() - 24, rect.bottom() - 24),
    };
    for (const QLine& crack : cracks) {
        p.drawLine(crack);
        p.setPen(QPen(QColor(255, 235, 180, 46), 1));
        p.drawLine(crack.translated(0, 1));
        p.setPen(QPen(QColor(107, 58, 17, 70), 1));
    }

    p.setPen(QPen(QColor(118, 69, 22, 68), 1));
    for (int i = 0; i < 9; ++i) {
        const int x = rect.left() + 18 + (i * 47) % std::max(1, rect.width() - 42);
        p.drawLine(x, rect.top() + 4 + (i % 3), std::min(rect.right() - 16, x + 10 + (i % 4) * 6), rect.top() + 4);
        p.drawLine(x + 5, rect.bottom() - 4 - (i % 2), std::min(rect.right() - 18, x + 18 + (i % 3) * 7), rect.bottom() - 5);
    }
    for (int i = 0; i < 7; ++i) {
        const int y = rect.top() + 18 + (i * 39) % std::max(1, rect.height() - 42);
        p.drawLine(rect.left() + 4, y, rect.left() + 9 + (i % 3) * 4, y + ((i % 2) ? 1 : -1));
        p.drawLine(rect.right() - 4, y + 5, rect.right() - 11 - (i % 3) * 4, y + 5 + ((i % 2) ? -1 : 1));
    }

    if (ruled) {
        p.setPen(QPen(QColor(128, 78, 27, 44), 1));
        for (int y = rect.top() + 54; y < rect.bottom() - 26; y += 28) {
            p.drawLine(rect.left() + 24, y, rect.left() + rect.width() / 2 - 8, y + ((y / 28) % 2));
            p.drawLine(rect.left() + rect.width() / 2 + 8, y + 1, rect.right() - 24, y);
            p.setPen(QPen(QColor(255, 236, 183, 30), 1));
            p.drawLine(rect.left() + 30, y + 1, rect.right() - 34, y + 1);
            p.setPen(QPen(QColor(128, 78, 27, 44), 1));
        }
    }
    p.restore();
}

void drawCabinInsetPanel(QPainter& p, const QRect& rect)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(3, 1, 0, 150));
    p.drawRoundedRect(rect.translated(6, 8), 8, 8);

    QLinearGradient wood(rect.topLeft(), rect.bottomLeft());
    wood.setColorAt(0.0, QColor(103, 55, 22));
    wood.setColorAt(0.28, QColor(74, 36, 15));
    wood.setColorAt(0.65, QColor(53, 24, 9));
    wood.setColorAt(1.0, QColor(32, 14, 6));
    p.setBrush(wood);
    p.setPen(QPen(QColor(18, 7, 2), 4));
    p.drawRoundedRect(rect, 8, 8);
    p.setPen(QPen(QColor(176, 101, 39, 72), 1));
    p.drawLine(rect.left() + 18, rect.top() + 7, rect.right() - 18, rect.top() + 7);
    p.drawLine(rect.left() + 7, rect.top() + 18, rect.left() + 7, rect.bottom() - 18);
    p.setPen(QPen(QColor(12, 4, 1, 118), 1));
    p.drawLine(rect.left() + 20, rect.bottom() - 6, rect.right() - 20, rect.bottom() - 6);
    p.drawLine(rect.right() - 6, rect.top() + 18, rect.right() - 6, rect.bottom() - 18);

    QRect woodClip = rect.adjusted(8, 8, -8, -8);
    p.setClipRect(woodClip);
    for (int y = rect.top() + 18; y < rect.bottom() - 10; y += 34) {
        p.setPen(QPen(QColor(17, 6, 2, 138), 2));
        p.drawLine(rect.left() + 10, y, rect.right() - 10, y + ((y / 34) % 3 - 1));
        p.setPen(QPen(QColor(140, 82, 34, 42), 1));
        p.drawLine(rect.left() + 14, y + 5, rect.right() - 14, y + 4);
    }
    for (int i = 0; i < 13; ++i) {
        const int x = rect.left() + 24 + (i * 73 + i * i * 9) % std::max(1, rect.width() - 58);
        const int y = rect.top() + 14 + (i * 41 + i * i * 5) % std::max(1, rect.height() - 40);
        QRect knot(x, y, 46 + (i % 3) * 16, 12 + (i % 2) * 6);
        p.setPen(QPen(QColor(18, 7, 2, 70), 1));
        p.drawArc(knot, 180 * 16, 170 * 16);
        p.setPen(QPen(QColor(111, 64, 27, 28), 1));
        p.drawArc(knot.adjusted(8, 3, -7, -3), 10 * 16, 150 * 16);
    }
    p.setPen(QPen(QColor(9, 3, 1, 70), 1));
    for (int i = 0; i < 24; ++i) {
        const int x = rect.left() + 18 + (i * 37) % std::max(1, rect.width() - 36);
        const int y = rect.top() + 12 + (i * 23) % std::max(1, rect.height() - 26);
        p.drawLine(x, y, std::min(rect.right() - 12, x + 18 + (i % 5) * 9), y + ((i % 3) - 1));
    }
    for (int i = 0; i < 18; ++i) {
        const int x = rect.left() + 18 + (i * 67 + i * i * 5) % std::max(1, rect.width() - 44);
        const int y = rect.top() + 13 + (i * 29 + i * i * 3) % std::max(1, rect.height() - 32);
        p.setPen(QPen(QColor(173, 102, 43, 28), 1));
        p.drawLine(x, y, std::min(rect.right() - 18, x + 20 + (i % 4) * 8), y - 1);
        p.setPen(QPen(QColor(9, 3, 1, 62), 1));
        p.drawLine(x + 2, y + 2, std::min(rect.right() - 18, x + 18 + (i % 4) * 7), y + 2);
    }
    p.setClipping(false);

    QRect paper = rect.adjusted(14, 14, -14, -14);
    drawParchmentPanel(p, paper, false);
    p.setPen(QPen(QColor(31, 13, 4, 112), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(paper.adjusted(-1, -1, 1, 1), 5, 5);
    p.setPen(QPen(QColor(255, 222, 137, 68), 1));
    p.drawLine(paper.left() + 10, paper.top() - 2, paper.right() - 10, paper.top() - 2);
    p.drawLine(paper.left() - 2, paper.top() + 10, paper.left() - 2, paper.bottom() - 10);
    p.setPen(QPen(QColor(30, 12, 3, 86), 1));
    p.drawLine(paper.left() + 12, paper.bottom() + 2, paper.right() - 12, paper.bottom() + 2);
    p.drawLine(paper.right() + 2, paper.top() + 10, paper.right() + 2, paper.bottom() - 10);

    p.setPen(QPen(QColor(24, 9, 3), 3));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 7, 7);
    drawBrassFrame(p, rect.adjusted(5, 5, -5, -5), 6, 2);

    const QVector<QRect> caps = {
        QRect(rect.left() + 9, rect.top() + 9, 34, 22),
        QRect(rect.right() - 42, rect.top() + 9, 34, 22),
        QRect(rect.left() + 9, rect.bottom() - 30, 34, 22),
        QRect(rect.right() - 42, rect.bottom() - 30, 34, 22),
    };
    for (const QRect& cap : caps) {
        QLinearGradient brass(cap.topLeft(), cap.bottomLeft());
        brass.setColorAt(0.0, QColor(169, 111, 35));
        brass.setColorAt(0.55, QColor(105, 57, 18));
        brass.setColorAt(1.0, QColor(43, 21, 6));
        p.setBrush(brass);
        p.setPen(QPen(QColor(35, 16, 5), 2));
        p.drawRoundedRect(cap, 3, 3);
        p.setPen(QPen(QColor(71, 98, 66, 70), 1));
        p.drawLine(cap.left() + 5, cap.top() + 5, cap.right() - 6, cap.top() + 4);
        p.setPen(QPen(QColor(246, 195, 83, 36), 1));
        p.drawLine(cap.left() + 6, cap.bottom() - 5, cap.right() - 7, cap.bottom() - 6);
        drawRivet(p, QPoint(cap.left() + 8, cap.top() + 8), 3);
        drawRivet(p, QPoint(cap.right() - 8, cap.bottom() - 8), 3);
    }

    p.restore();
}

void drawIconWell(QPainter& p, const QRect& rect, bool selected, bool disabled)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(36, 15, 4, 70));
    p.drawRoundedRect(rect.translated(4, 5), 7, 7);

    QLinearGradient frame(rect.topLeft(), rect.bottomLeft());
    frame.setColorAt(0.0, selected ? QColor(231, 170, 55) : QColor(185, 113, 35));
    frame.setColorAt(0.38, QColor(133, 72, 22));
    frame.setColorAt(1.0, QColor(66, 31, 9));
    p.setBrush(frame);
    p.setPen(QPen(QColor(65, 29, 8), 2));
    p.drawRoundedRect(rect, 7, 7);
    p.setPen(QPen(QColor(55, 88, 58, disabled ? 32 : 62), 1));
    p.drawRoundedRect(rect.adjusted(4, 5, -4, -5), 5, 5);
    p.setPen(QPen(QColor(255, 219, 112, selected ? 96 : 52), 1));
    p.drawLine(rect.left() + 10, rect.top() + 5, rect.right() - 10, rect.top() + 5);
    p.setPen(QPen(QColor(54, 24, 6, 98), 1));
    p.drawLine(rect.left() + 10, rect.bottom() - 5, rect.right() - 10, rect.bottom() - 5);
    for (int i = 0; i < 5; ++i) {
        const int x = rect.left() + 9 + (i * 19) % std::max(1, rect.width() - 18);
        const int y = rect.top() + 5 + (i % 2) * (rect.height() - 11);
        p.setPen(QPen(QColor(88, 44, 10, 66), 1));
        p.drawLine(x, y, std::min(rect.right() - 8, x + 6 + (i % 3) * 4), y + ((i % 2) ? -1 : 1));
    }

    QRect inner = rect.adjusted(8, 8, -8, -8);
    QLinearGradient paper(inner.topLeft(), inner.bottomRight());
    paper.setColorAt(0.0, disabled ? QColor(231, 206, 154) : QColor(246, 220, 164));
    paper.setColorAt(0.58, disabled ? QColor(216, 181, 118) : QColor(235, 197, 129));
    paper.setColorAt(1.0, disabled ? QColor(196, 151, 82) : QColor(217, 165, 88));
    p.setBrush(paper);
    p.setPen(QPen(QColor(113, 65, 22), 2));
    p.drawRoundedRect(inner, 5, 5);
    p.setPen(QPen(QColor(255, 235, 173, selected ? 150 : 86), 1));
    p.drawRoundedRect(inner.adjusted(4, 4, -4, -4), 3, 3);

    p.setClipRect(inner.adjusted(4, 4, -4, -4));
    p.setPen(QPen(QColor(133, 82, 31, disabled ? 22 : 34), 1));
    for (int i = 0; i < 8; ++i) {
        const int y = inner.top() + 8 + (i * 13) % std::max(1, inner.height() - 14);
        p.drawLine(inner.left() + 7, y, inner.right() - 8, y + ((i % 3) - 1));
    }
    for (int i = 0; i < 12; ++i) {
        const int x = inner.left() + 8 + (i * 17 + i * i) % std::max(1, inner.width() - 16);
        const int y = inner.top() + 8 + (i * 11 + i * i * 2) % std::max(1, inner.height() - 16);
        p.setPen(QPen(QColor(255, 241, 198, disabled ? 16 : 24), 1));
        p.drawLine(x, y, std::min(inner.right() - 7, x + 5 + (i % 3) * 4), y - 1);
        p.setPen(QPen(QColor(132, 78, 24, disabled ? 14 : 22), 1));
        p.drawLine(x + 1, y + 2, std::min(inner.right() - 7, x + 7 + (i % 4) * 3), y + 2);
    }
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 6; ++i) {
        const int x = inner.left() + 8 + (i * 19) % std::max(1, inner.width() - 16);
        const int y = inner.top() + 8 + (i * 17) % std::max(1, inner.height() - 16);
        p.setBrush(QColor(115, 65, 18, disabled ? 10 : 17));
        p.drawEllipse(QPoint(x, y), 2 + (i % 3), 1 + (i % 2));
    }
    p.setClipping(false);

    if (selected) {
        p.setPen(QPen(QColor(231, 177, 54, 178), 2));
        p.drawRoundedRect(rect.adjusted(3, 3, -3, -3), 6, 6);
        p.setPen(QPen(QColor(255, 230, 126, 70), 1));
        p.drawLine(rect.left() + 12, rect.top() + 10, rect.right() - 12, rect.top() + 9);
    }

    p.restore();
}

void drawTabBase(QPainter& p, const QRect& rect, bool selected, bool hovered)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(4, 2, 1, selected ? 130 : 95));
    p.drawRoundedRect(rect.translated(4, selected ? 6 : 5), 7, 7);

    QLinearGradient fill(rect.topLeft(), rect.bottomLeft());
    if (selected) {
        fill.setColorAt(0.0, QColor(172, 101, 25));
        fill.setColorAt(0.52, QColor(101, 53, 16));
        fill.setColorAt(1.0, QColor(58, 27, 9));
    } else {
        fill.setColorAt(0.0, QColor(88, 50, 20));
        fill.setColorAt(0.55, QColor(48, 25, 10));
        fill.setColorAt(1.0, QColor(25, 12, 5));
    }
    p.setBrush(fill);
    p.setPen(QPen(QColor(35, 15, 4), 3));
    p.drawRoundedRect(rect, 7, 7);

    QRect face = rect.adjusted(7, 6, -7, -7);
    p.setClipRect(face);
    for (int y = face.top() + 8; y < face.bottom(); y += 16) {
        p.setPen(QPen(QColor(15, 6, 2, selected ? 90 : 72), 1));
        p.drawLine(face.left() + 4, y, face.right() - 4, y + ((y / 16) % 2));
        p.setPen(QPen(QColor(186, 109, 38, selected ? 54 : 36), 1));
        p.drawLine(face.left() + 8, y + 4, face.right() - 8, y + 3);
    }
    p.setClipping(false);

    drawBrassFrame(p, rect.adjusted(2, 2, -2, -2), 6, selected ? 3 : 2);
    if (hovered && !selected) {
        p.fillRect(rect.adjusted(9, 8, -9, -8), QColor(255, 215, 99, 30));
    }
    if (selected) {
        p.setPen(QPen(QColor(255, 221, 91, 130), 2));
        p.drawLine(rect.left() + 16, rect.bottom() - 5, rect.right() - 16, rect.bottom() - 5);
    }
    drawRivet(p, QPoint(rect.left() + 18, rect.top() + 12), 4);
    drawRivet(p, QPoint(rect.right() - 18, rect.top() + 12), 4);
    p.restore();
}

void drawButtonBase(QPainter& p, const QRect& rect, const QColor& color, bool disabled, bool hovered)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(5, 2, 1, 112));
    p.drawRoundedRect(rect.translated(6, 7), 7, 7);

    QLinearGradient fill(rect.topLeft(), rect.bottomLeft());
    QColor top = color.lighter(disabled ? 90 : 136);
    QColor bottom = color.darker(disabled ? 170 : 142);
    fill.setColorAt(0.0, top);
    fill.setColorAt(0.5, color);
    fill.setColorAt(1.0, bottom);
    p.setBrush(fill);
    p.setPen(QPen(QColor(32, 14, 5), 4));
    p.drawRoundedRect(rect, 7, 7);
    p.setPen(QPen(QColor(236, 171, 50), 2));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 4, 4);
    p.setPen(QPen(QColor(255, 238, 154, disabled ? 35 : 95), 1));
    p.drawLine(rect.left() + 18, rect.top() + 12, rect.right() - 18, rect.top() + 12);
    if (hovered && !disabled) {
        p.fillRect(rect.adjusted(9, 9, -9, -9), QColor(255, 230, 120, 34));
    }
    if (disabled) {
        p.fillRect(rect.adjusted(7, 7, -7, -7), QColor(25, 18, 12, 112));
    }
    p.restore();
}

void drawSlotBase(QPainter& p, const QRect& rect, bool selected, bool disabled)
{
    p.save();
    drawParchmentPanel(p, rect, false);
    p.setPen(QPen(selected ? QColor(255, 200, 58) : QColor(109, 67, 22), selected ? 3 : 2));
    p.setBrush(disabled ? QColor(70, 52, 34, 70) : QColor(58, 34, 14, 28));
    p.drawRoundedRect(rect.adjusted(10, 10, -10, -10), 4, 4);
    p.restore();
}

void drawTagBase(QPainter& p, const QRect& rect, const QColor& color, bool disabled)
{
    p.save();
    QLinearGradient fill(rect.topLeft(), rect.bottomLeft());
    QColor base = disabled ? QColor(75, 56, 34) : color.darker(128);
    fill.setColorAt(0.0, base.lighter(disabled ? 104 : 118));
    fill.setColorAt(0.55, base.darker(disabled ? 112 : 120));
    fill.setColorAt(1.0, base.darker(disabled ? 150 : 166));
    p.setBrush(fill);
    p.setPen(QPen(QColor(28, 13, 4, disabled ? 130 : 210), 2));
    p.drawRoundedRect(rect, 4, 4);
    p.setPen(QPen(QColor(255, 218, 119, disabled ? 24 : 58), 1));
    p.drawLine(rect.left() + 6, rect.top() + 5, rect.right() - 6, rect.top() + 5);
    p.setPen(QPen(QColor(41, 20, 7, disabled ? 42 : 74), 1));
    p.drawLine(rect.left() + 7, rect.bottom() - 5, rect.right() - 7, rect.bottom() - 6);
    if (!disabled) {
        p.setPen(QPen(QColor(50, 82, 52, 48), 1));
        p.drawPoint(rect.left() + 10, rect.top() + 9);
        p.drawPoint(rect.right() - 12, rect.bottom() - 8);
    }
    p.restore();
}
}

BackpackDialog::BackpackDialog(int stage, QWidget* parent)
    : QDialog(parent)
{
    m_stage = std::max(1, stage);
    InventorySystem::instance().initDefaultWeaponIfNeeded();
    setWindowTitle(QStringLiteral("船舱背包"));
    setFixedSize(1280, 720);
    setMouseTracking(true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    loadAssets();
    clampSelections();
}

void BackpackDialog::loadAssets()
{
    m_seaBackground.load(QStringLiteral(":/FishingVoyage/ui/shop/background.png"));
    if (m_seaBackground.isNull()) {
        m_seaBackground.load(QStringLiteral(":/FishingVoyage/backgrounds/sea.png"));
    }
    m_topStatusBar.load(QStringLiteral(":/FishingVoyage/ui/shop/top_status_bar.png"));
    if (m_topStatusBar.isNull()) {
        m_topStatusBar.load(QStringLiteral(":/FishingVoyage/ui/backpack/top_status_bar.png"));
    }
    m_windowPanel.load(QStringLiteral(":/FishingVoyage/ui/shop/window_panel.png"));
    if (m_windowPanel.isNull()) {
        m_windowPanel.load(QStringLiteral(":/FishingVoyage/ui/backpack/window_panel.png"));
    }
    m_titlePlaque.load(QStringLiteral(":/FishingVoyage/ui/shop/title_plaque.png"));
    if (m_titlePlaque.isNull()) {
        m_titlePlaque.load(QStringLiteral(":/FishingVoyage/ui/backpack/title_plaque.png"));
    }
    m_infoStrip = QPixmap();
    m_tabSelected = QPixmap();
    m_tabNormal = QPixmap();
    m_listPanel.load(QStringLiteral(":/FishingVoyage/ui/shop/content_panel.png"));
    m_detailPanel.load(QStringLiteral(":/FishingVoyage/ui/shop/content_panel.png"));
    m_detailCard = QPixmap();
    m_statsSheet = QPixmap();
    m_rowNormal = QPixmap();
    m_rowSelected = QPixmap();
    m_rowDisabled = QPixmap();
    m_slotFrame = QPixmap();
    m_buttonGreen.load(QStringLiteral(":/FishingVoyage/ui/shop/button_green.png"));
    m_buttonBlue.load(QStringLiteral(":/FishingVoyage/ui/shop/button_blue.png"));
    m_buttonRed.load(QStringLiteral(":/FishingVoyage/ui/shop/button_red.png"));
    m_footerHint = QPixmap();

    m_iconHeart.load(QStringLiteral(":/FishingVoyage/ui/hud/icon_heart.png"));
    m_iconLightning.load(QStringLiteral(":/FishingVoyage/ui/hud/icon_lightning.png"));
    m_iconCoin.load(QStringLiteral(":/FishingVoyage/ui/hud/icon_coin.png"));
    m_iconFish.load(QStringLiteral(":/FishingVoyage/ui/hud/icon_fish.png"));
    m_iconSun.load(QStringLiteral(":/FishingVoyage/ui/hud/icon_sun.png"));
    m_iconRod.load(QStringLiteral(":/FishingVoyage/ui/icons/weapon_rod.png"));
    m_iconNet.load(QStringLiteral(":/FishingVoyage/ui/icons/weapon_net.png"));
    m_iconHarpoon.load(QStringLiteral(":/FishingVoyage/ui/icons/weapon_harpoon.png"));
    m_iconPistol.load(QStringLiteral(":/FishingVoyage/ui/icons/weapon_pistol.png"));
    m_iconShotgun.load(QStringLiteral(":/FishingVoyage/ui/icons/weapon_shotgun.png"));
    m_iconFood.load(QStringLiteral(":/FishingVoyage/ui/icons/item_food.png"));
    m_iconRepair1.load(QStringLiteral(":/FishingVoyage/ui/icons/item_repair_t1.png"));
    m_iconRepair2.load(QStringLiteral(":/FishingVoyage/ui/icons/item_repair_t2.png"));
    m_iconRepair3.load(QStringLiteral(":/FishingVoyage/ui/icons/item_repair_t3.png"));
    m_iconEmergencyRepair.load(QStringLiteral(":/FishingVoyage/ui/icons/item_emergency_repair.png"));
}

void BackpackDialog::paintEvent(QPaintEvent*)
{
    m_zones.clear();
    clampSelections();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    drawBackground(p);
    drawTopStatus(p);
    drawMainFrame(p);
    drawTitle(p);
    drawInfoStrip(p);
    drawTabs(p);
    if (m_page == Page::Equipment) {
        drawEquipmentList(p);
        drawEquipmentDetail(p);
    } else {
        drawItemList(p);
        drawItemDetail(p);
    }
    drawActions(p);
    drawFooterHint(p);
}

void BackpackDialog::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->pos();
    setCursor(zoneAt(m_mousePos) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

void BackpackDialog::leaveEvent(QEvent*)
{
    m_mousePos = QPoint(-1, -1);
    setCursor(Qt::ArrowCursor);
    update();
}

void BackpackDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    if (const ClickZone* zone = zoneAt(event->pos())) {
        handleAction(zone->action, zone->index);
    }
}

void BackpackDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_B:
        accept();
        return;
    case Qt::Key_Left:
    case Qt::Key_A:
        switchPage(Page::Equipment);
        return;
    case Qt::Key_Right:
    case Qt::Key_D:
        switchPage(Page::Items);
        return;
    case Qt::Key_Up:
    case Qt::Key_W:
        selectNext(-1);
        return;
    case Qt::Key_Down:
    case Qt::Key_S:
        selectNext(1);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        handleAction(m_page == Page::Equipment ? Action::Equip : Action::Use, -1);
        return;
    default:
        QDialog::keyPressEvent(event);
        return;
    }
}

void BackpackDialog::drawBackground(QPainter& p)
{
    if (!m_seaBackground.isNull()) {
        p.drawPixmap(rect(), m_seaBackground);
    } else {
        p.fillRect(rect(), QColor(13, 71, 112));
    }
    p.fillRect(rect(), QColor(0, 15, 28, 132));
}

void BackpackDialog::drawTopStatus(QPainter& p)
{
    p.save();
    if (!m_topStatusBar.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(kTopStatusRect, m_topStatusBar);
    } else {
        drawWoodPanel(p, kTopStatusRect, false);
    }
    drawPixmapFit(p, m_iconSun, QRect(kTopStatusRect.left() + 20, kTopStatusRect.top() + 8, 38, 38));

    drawTextShadow(p, QRect(kTopStatusRect.left() + 104, kTopStatusRect.top() + 4, 104, 24),
                   QStringLiteral("渔 途"), titleFont(18, QFont::Bold), QColor(255, 232, 166),
                   Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(kTopStatusRect.left() + 110, kTopStatusRect.top() + 30, 94, 16),
                   QStringLiteral("第%1关").arg(m_stage), uiFont(10, QFont::Bold), QColor(216, 166, 92),
                   Qt::AlignLeft | Qt::AlignVCenter);

    Player& pl = Player::instance();
    struct Cell {
        QString label;
        QString value;
        QColor color;
        int current;
        int max;
        const QPixmap* icon;
        bool progress;
    };
    QVector<Cell> cells = {
        {QStringLiteral("耐久"), QString("%1/%2").arg(pl.durability()).arg(pl.maxDurability), QColor(220, 66, 58), pl.durability(), pl.maxDurability, &m_iconHeart, true},
        {QStringLiteral("体力"), QString("%1/%2").arg(pl.stamina()).arg(pl.maxStamina), QColor(52, 124, 232), pl.stamina(), pl.maxStamina, &m_iconLightning, true},
        {QStringLiteral("金币"), coinDisplayText(pl), QColor(235, 173, 42), 1, 1, &m_iconCoin, false},
        {QStringLiteral("鱼获"), QString("%1/25").arg(pl.fishCaught), QColor(90, 178, 220), pl.fishCaught, 25, &m_iconFish, true},
        {QStringLiteral("天气"), weatherName(), QColor(255, 211, 64), 1, 1, &m_iconSun, false}
    };

    int x = kTopStatusRect.left() + 230;
    for (const Cell& cell : cells) {
        QRect cellRect(x, kTopStatusRect.top() + 7, 150, 40);
        p.setPen(QPen(QColor(181, 112, 35, 96), 1));
        p.setBrush(QColor(12, 8, 6, 118));
        p.drawRoundedRect(cellRect, 4, 4);
        drawPixmapFit(p, *cell.icon, QRect(cellRect.left() + 8, cellRect.top() + 6, 28, 28));
        drawTextShadow(p, QRect(cellRect.left() + 42, cellRect.top() + 3, 48, 17), cell.label,
                       uiFont(10, QFont::Bold), QColor(228, 183, 112), Qt::AlignLeft | Qt::AlignVCenter);
        drawTextShadow(p, QRect(cellRect.left() + 84, cellRect.top() + 3, 58, 17), cell.value,
                       uiFont(10, QFont::Bold), QColor(255, 245, 220), Qt::AlignRight | Qt::AlignVCenter);
        if (cell.progress) {
            QRect bar(cellRect.left() + 42, cellRect.bottom() - 11, cellRect.width() - 52, 7);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(32, 24, 18));
            p.drawRoundedRect(bar, 3, 3);
            const int fillWidth = cell.max > 0 ? qBound(0, cell.current * bar.width() / cell.max, bar.width()) : bar.width();
            p.setBrush(cell.color);
            p.drawRoundedRect(QRect(bar.left(), bar.top(), fillWidth, bar.height()), 3, 3);
        }
        x += 168;
    }
    p.restore();
}

void BackpackDialog::drawMainFrame(QPainter& p)
{
    if (!m_windowPanel.isNull()) {
        p.save();
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(kMainRect, m_windowPanel);
        p.restore();
    } else {
        drawWoodPanel(p, kMainRect, true);
    }

}

void BackpackDialog::drawTitle(QPainter& p)
{
    if (!m_titlePlaque.isNull()) {
        p.drawPixmap(kTitleRect, m_titlePlaque);
    } else {
        drawWoodPanel(p, kTitleRect, false);
    }
    drawTextShadow(p, QRect(kTitleRect.left() + 46, kTitleRect.top() + 18, kTitleRect.width() - 92, 34),
                   QStringLiteral("船舱背包"), titleFont(27, QFont::Bold), QColor(255, 226, 148));
    drawTextShadow(p, QRect(kTitleRect.left() + 52, kTitleRect.top() + 52, kTitleRect.width() - 104, 18),
                   QStringLiteral("SHIP CABIN BACKPACK"), uiFont(10, QFont::Bold), QColor(255, 218, 134));
}

void BackpackDialog::drawInfoStrip(QPainter& p)
{
    if (!m_infoStrip.isNull()) {
        p.drawPixmap(kInfoRect, m_infoStrip);
    } else {
        drawParchmentPanel(p, kInfoRect, false);
    }
    InventorySystem& inv = InventorySystem::instance();
    const Weapon* current = inv.currentWeapon();
    const QString currentName = current ? QString::fromStdString(current->getName()) : QStringLiteral("无");
    Player& pl = Player::instance();

    drawPixmapFit(p, m_iconCoin, QRect(kInfoRect.left() + 54, kInfoRect.top() + 6, 24, 24));
    drawTextShadow(p, QRect(kInfoRect.left() + 82, kInfoRect.top() + 5, 150, 24),
                   QStringLiteral("金币：%1").arg(coinDisplayText(pl)), uiFont(12, QFont::Bold),
                   QColor(75, 43, 16), Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(kInfoRect.left() + 252, kInfoRect.top() + 5, 230, 24),
                   QStringLiteral("舱位：装备 %1/%2  道具 %3")
                       .arg(inv.weaponCount()).arg(inv.maxWeaponCapacity())
                       .arg(capacityText(inv.getTotalItemCount(), Config::MAX_ITEM_BACKPACK)),
                   uiFont(12, QFont::Bold), QColor(75, 43, 16), Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(kInfoRect.left() + 500, kInfoRect.top() + 5, 210, 24),
                   QStringLiteral("当前装备：%1").arg(currentName), uiFont(12, QFont::Bold),
                   QColor(75, 43, 16), Qt::AlignLeft | Qt::AlignVCenter);

    p.save();
    p.setPen(QPen(QColor(89, 52, 20, 150), 2));
    p.drawLine(kInfoRect.left() + 232, kInfoRect.top() + 7, kInfoRect.left() + 232, kInfoRect.bottom() - 8);
    p.drawLine(kInfoRect.left() + 486, kInfoRect.top() + 7, kInfoRect.left() + 486, kInfoRect.bottom() - 8);
    p.restore();
}

void BackpackDialog::drawTabs(QPainter& p)
{
    auto drawTab = [&](const QRect& rect, bool selected) {
        const QPixmap& bg = selected ? m_tabSelected : m_tabNormal;
        if (!bg.isNull()) {
            p.drawPixmap(rect, bg);
            if (isHovered(rect) && !selected) {
                p.fillRect(rect.adjusted(9, 8, -9, -8), QColor(255, 213, 88, 32));
            }
        } else {
            drawTabBase(p, rect, selected, isHovered(rect));
        }
    };
    drawTab(kEquipTabRect, m_page == Page::Equipment);
    drawTab(kItemTabRect, m_page == Page::Items);
    drawTextShadow(p, kEquipTabRect.adjusted(0, 2, 0, -4), QStringLiteral("装备背包"),
                   titleFont(15, QFont::Bold), m_page == Page::Equipment ? QColor(255, 236, 160) : QColor(222, 190, 125));
    drawTextShadow(p, kItemTabRect.adjusted(0, 2, 0, -4), QStringLiteral("道具背包"),
                   titleFont(15, QFont::Bold), m_page == Page::Items ? QColor(255, 236, 160) : QColor(222, 190, 125));
    addZone(kEquipTabRect, Action::SwitchEquipment);
    addZone(kItemTabRect, Action::SwitchItems);
}

void BackpackDialog::drawEquipmentList(QPainter& p)
{
    drawCabinInsetPanel(p, kListPanelRect);
    InventorySystem& inv = InventorySystem::instance();
    const auto& weapons = inv.weapons();
    const int capacity = inv.maxWeaponCapacity();
    const int weaponCount = static_cast<int>(weapons.size());
    const int maxFirstVisible = qMax(0, weaponCount - kVisibleRows);
    const int firstVisible = qBound(
        0,
        m_selectedEquipmentIndex - kVisibleRows / 2,
        maxFirstVisible);

    for (int rowIndex = 0; rowIndex < kVisibleRows; ++rowIndex) {
        const int weaponIndex = firstVisible + rowIndex;
        QRect row(kListPanelRect.left() + 22,
                  kListPanelRect.top() + 12 + rowIndex * kRowHeight,
                  340, kRowHeight);
        const bool hasWeapon = weaponIndex < weaponCount && weapons[weaponIndex];
        const bool selected = hasWeapon && weaponIndex == m_selectedEquipmentIndex;
        const bool locked = weaponIndex >= capacity;

        if (hasWeapon) {
            const Weapon* w = weapons[weaponIndex];
            drawRowText(p, row, weaponIcon(QString::fromStdString(w->getTypeCode())),
                        QString::fromStdString(w->getName()),
                        QStringLiteral("耐久 %1  Lv.%2 +%3").arg(weaponDurabilityText(w)).arg(w->getTier()).arg(w->getEnhancementLevel()),
                        weaponStatusText(weaponIndex, w), weaponStatusColor(weaponIndex, w), selected, false);
            addZone(row, Action::SelectEquipment, weaponIndex);
        } else {
            const QString title = locked ? QStringLiteral("未开放舱位") : QStringLiteral("空槽位");
            const QString subtitle = locked ? QStringLiteral("随游戏进程扩展") : QStringLiteral("购买或替换装备后显示在这里");
            drawRowText(p, row, QPixmap(), title, subtitle, locked ? QStringLiteral("锁定") : QStringLiteral("空"),
                        QColor("#6b573d"), false, true);
        }
    }
}

void BackpackDialog::drawItemList(QPainter& p)
{
    drawCabinInsetPanel(p, kListPanelRect);
    const QVector<ItemDef> defs = itemDefs();
    InventorySystem& inv = InventorySystem::instance();

    for (int i = 0; i < std::min(kVisibleRows, static_cast<int>(defs.size())); ++i) {
        const ItemDef& def = defs[i];
        const int count = inv.getItemCount(def.type);
        QRect row(kListPanelRect.left() + 22, kListPanelRect.top() + 12 + i * kRowHeight, 340, kRowHeight);
        drawRowText(p, row, def.icon ? *def.icon : QPixmap(),
                    def.name, def.subtitle,
                    QStringLiteral("x%1").arg(count),
                    count > 0 ? QColor("#2f7a45") : QColor("#7a5d38"),
                    i == m_selectedItemIndex, count <= 0);
        addZone(row, Action::SelectItem, i);
    }
}

void BackpackDialog::drawEquipmentDetail(QPainter& p)
{
    drawCabinInsetPanel(p, kDetailPanelRect);
    const QRect cardRect(kDetailPanelRect.left() + 24, kDetailPanelRect.top() + 16, 204, 294);
    const QRect statsRect(kDetailPanelRect.left() + 236, kDetailPanelRect.top() + 16, 262, 294);
    drawParchmentPanel(p, cardRect, false);
    drawParchmentPanel(p, statsRect, true);

    const Weapon* w = selectedWeapon();
    if (!w) {
        drawTextShadow(p, statsRect.adjusted(22, 76, -22, -76),
                       QStringLiteral("当前没有可查看的装备。\n从商店购买或在冒险中获得装备后，会自动进入这里。"),
                       uiFont(13, QFont::Bold), QColor(95, 62, 25));
        return;
    }

    QRect imageBox(cardRect.left() + 34, cardRect.top() + 26, 136, 120);
    drawIconWell(p, imageBox.adjusted(8, 2, -8, -4), true, false);
    drawPixmapFit(p, weaponIcon(QString::fromStdString(w->getTypeCode())),
                  imageBox.adjusted(22, 18, -22, -20));

    const int pct = durabilityPercent(w);
    QRect bar(cardRect.left() + 34, cardRect.top() + 160, 136, 10);
    p.save();
    p.setPen(QPen(QColor(87, 55, 20, 150), 1));
    p.setBrush(QColor(58, 35, 17, 160));
    p.drawRoundedRect(bar, 4, 4);
    QLinearGradient durGrad(bar.topLeft(), bar.topRight());
    durGrad.setColorAt(0.0, pct <= 30 ? QColor(176, 62, 36) : QColor(81, 148, 48));
    durGrad.setColorAt(1.0, pct <= 30 ? QColor(239, 153, 54) : QColor(165, 203, 61));
    p.setPen(Qt::NoPen);
    p.setBrush(durGrad);
    p.drawRoundedRect(QRect(bar.left(), bar.top(), qBound(0, pct * bar.width() / 100, bar.width()), bar.height()), 4, 4);
    p.restore();

    drawTextShadow(p, QRect(cardRect.left() + 24, cardRect.top() + 178, 156, 22),
                   QStringLiteral("耐久状态：%1%").arg(pct), uiFont(10, QFont::Bold),
                   QColor(88, 55, 22), Qt::AlignCenter);
    drawTextShadow(p, QRect(cardRect.left() + 24, cardRect.top() + 208, 156, 62),
                   weaponDescription(w), uiFont(10, QFont::Bold),
                   QColor(88, 55, 22), Qt::AlignLeft | Qt::AlignTop);

    int y = statsRect.top() + 58;
    const int statsX = statsRect.left() + 22;
    drawTextShadow(p, QRect(statsX, statsRect.top() + 18, 220, 28),
                   QString::fromStdString(w->getName()), titleFont(16, QFont::Bold),
                   QColor(73, 43, 16), Qt::AlignLeft | Qt::AlignVCenter);
    drawStatLine(p, y, QStringLiteral("类型"), weaponRoleText(w));
    drawStatLine(p, y, QStringLiteral("装备品阶"), QStringLiteral("T%1（固定）").arg(w->getTier()));
    drawStatLine(p, y, QStringLiteral("强化等级"), QStringLiteral("+%1").arg(w->getEnhancementLevel()));
    drawStatLine(p, y, QStringLiteral("用途"), w->canFish() && w->canAttack() ? QStringLiteral("捕鱼 / 攻击")
                                                    : w->canFish() ? QStringLiteral("捕鱼")
                                                                   : QStringLiteral("攻击"));
    drawStatLine(p, y, QStringLiteral("捕鱼方式"), fishingModeText(w));
    drawStatLine(p, y, QStringLiteral("攻击伤害"), w->canAttack() ? QString::number(w->getDamage()) : QStringLiteral("-"));
    drawStatLine(p, y, QStringLiteral("攻击范围"), QString::number(w->getRange()));
    drawStatLine(p, y, QStringLiteral("耐久"), weaponDurabilityText(w));
    drawStatLine(p, y, QStringLiteral("状态"), weaponStatusText(m_selectedEquipmentIndex, w));
    const int quickSlot = InventorySystem::instance().quickSlotForWeapon(m_selectedEquipmentIndex);
    drawStatLine(p, y, QStringLiteral("快捷槽"),
                 quickSlot >= 0 ? QString::number(quickSlot + 1) : QStringLiteral("未设置"));
}

void BackpackDialog::drawItemDetail(QPainter& p)
{
    drawCabinInsetPanel(p, kDetailPanelRect);
    const QRect cardRect(kDetailPanelRect.left() + 24, kDetailPanelRect.top() + 16, 204, 294);
    const QRect statsRect(kDetailPanelRect.left() + 236, kDetailPanelRect.top() + 16, 262, 294);
    drawParchmentPanel(p, cardRect, false);
    drawParchmentPanel(p, statsRect, true);

    const ItemDef* item = selectedItemDef();
    if (!item) return;

    QRect imageBox(cardRect.left() + 40, cardRect.top() + 38, 124, 124);
    drawIconWell(p, imageBox.adjusted(6, 0, -6, -4), true, false);
    if (item->icon) {
        drawPixmapFit(p, *item->icon, imageBox.adjusted(26, 22, -26, -24));
    }

    const int count = InventorySystem::instance().getItemCount(item->type);
    drawTextShadow(p, QRect(cardRect.left() + 24, cardRect.top() + 174, 156, 28),
                   QStringLiteral("持有：%1").arg(count), titleFont(15, QFont::Bold),
                   count > 0 ? QColor(43, 111, 52) : QColor(118, 76, 37));
    drawTextShadow(p, QRect(cardRect.left() + 24, cardRect.top() + 214, 156, 62),
                   item->description, uiFont(10, QFont::Bold), QColor(88, 55, 22),
                   Qt::AlignLeft | Qt::AlignTop);

    int y = statsRect.top() + 58;
    const int statsX = statsRect.left() + 22;
    drawTextShadow(p, QRect(statsX, statsRect.top() + 18, 220, 28),
                   item->name, titleFont(16, QFont::Bold), QColor(73, 43, 16),
                   Qt::AlignLeft | Qt::AlignVCenter);
    drawStatLine(p, y, QStringLiteral("分类"), QStringLiteral("消耗道具"));
    drawStatLine(p, y, QStringLiteral("数量"), QString::number(count));
    drawStatLine(p, y, QStringLiteral("可用状态"), count > 0 ? QStringLiteral("可使用") : QStringLiteral("暂无库存"));

    QRect hintRect(statsX, y + 14, 210, 96);
    const QString hint = item->type == InventoryItemType::Food
        ? QStringLiteral("航行中体力不足时使用，能立刻恢复行动能力。")
        : item->type == InventoryItemType::EmergencyWeaponRepair
            ? QStringLiteral("用于修复选中的装备；建议留给低耐久或关键装备。")
            : QStringLiteral("船体耐久不足时使用，越高级的修理包恢复越多。");
    drawTextShadow(p, hintRect, QStringLiteral("航海建议：%1").arg(hint), uiFont(10, QFont::Bold),
                   QColor(73, 43, 16), Qt::AlignLeft | Qt::AlignTop);
}

void BackpackDialog::drawActions(QPainter& p)
{
    if (m_page == Page::Equipment) {
        const bool selectedCurrent = m_selectedEquipmentIndex == InventorySystem::instance().currentWeaponIndex();
        const Weapon* w = selectedWeapon();
        drawButton(p, kEquipButtonRect, m_buttonGreen, selectedCurrent ? QStringLiteral("已装备") : QStringLiteral("装备"),
                   Action::Equip, -1, !w || w->isBroken() || selectedCurrent);
        drawButton(p, kUseButtonRect, m_buttonBlue, QStringLiteral("修理"),
                   Action::Repair, -1, !canRepairSelectedWeapon());
        drawButton(p, kSlotButtonRect, m_buttonBlue, QStringLiteral("快捷槽"),
                   Action::AssignSlot, -1, !w);
        drawButton(p, kDiscardButtonRect, m_buttonRed, QStringLiteral("丢弃"),
                   Action::Discard, -1, !w || !w->isBroken());
    } else {
        drawButton(p, kEquipButtonRect, m_buttonGreen, QStringLiteral("使用"),
                   Action::Use, -1, !canUseSelectedItem());
        drawButton(p, kUseButtonRect, m_buttonBlue, QStringLiteral("装备页"),
                   Action::SwitchEquipment, -1, false);
    }
    drawButton(p, kCloseButtonRect, m_buttonRed, QStringLiteral("返回"), Action::Close, -1, false);

    if (!m_statusMessage.isEmpty()) {
        drawTextShadow(p, QRect(310, 552, 660, 22), m_statusMessage,
                       uiFont(11, QFont::Bold), QColor(255, 226, 142));
    }
}

void BackpackDialog::drawFooterHint(QPainter& p)
{
    if (!m_footerHint.isNull()) {
        p.drawPixmap(kFooterRect, m_footerHint);
    } else {
        drawWoodPanel(p, kFooterRect, false);
    }
    drawTextShadow(p, kFooterRect.adjusted(12, 0, -12, 0),
                   QStringLiteral("↑↓ 选择    Enter 确认    Esc 关闭背包"),
                   uiFont(10, QFont::Bold), QColor(255, 231, 180));
}

void BackpackDialog::drawRowText(QPainter& p, const QRect& row, const QPixmap& icon,
                                 const QString& title, const QString& subtitle,
                                 const QString& tag, const QColor& tagColor,
                                 bool selected, bool disabled)
{
    const QPixmap& rowBg = disabled ? m_rowDisabled : (selected ? m_rowSelected : m_rowNormal);
    if (!rowBg.isNull()) {
        p.drawPixmap(row, rowBg);
        if (isHovered(row) && !selected && !disabled) {
            p.fillRect(row.adjusted(9, 9, -9, -9), QColor(255, 221, 112, 28));
        }
    } else {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(34, 13, 3, 66));
        p.drawRoundedRect(row.translated(4, 5), 8, 8);

        QRect body = row.adjusted(4, 4, -4, -4);
        QLinearGradient fill(body.topLeft(), body.bottomRight());
        fill.setColorAt(0.0, disabled ? QColor(234, 208, 157) : QColor(246, 221, 169));
        fill.setColorAt(0.43, disabled ? QColor(225, 190, 127) : QColor(237, 201, 135));
        fill.setColorAt(0.72, disabled ? QColor(211, 168, 99) : QColor(224, 178, 105));
        fill.setColorAt(1.0, disabled ? QColor(192, 140, 68) : QColor(204, 148, 72));
        p.setBrush(fill);
        p.setPen(QPen(QColor(112, 61, 18), 2));
        p.drawRoundedRect(body, 7, 7);
        p.setPen(QPen(QColor(255, 241, 196, disabled ? 80 : 112), 1));
        p.drawLine(body.left() + 12, body.top() + 5, body.right() - 14, body.top() + 5);
        p.drawLine(body.left() + 5, body.top() + 12, body.left() + 5, body.bottom() - 12);
        p.setPen(QPen(QColor(120, 62, 17, disabled ? 48 : 68), 1));
        p.drawLine(body.left() + 14, body.bottom() - 5, body.right() - 12, body.bottom() - 5);
        p.drawLine(body.right() - 5, body.top() + 12, body.right() - 5, body.bottom() - 12);

        p.setClipRect(body.adjusted(3, 3, -3, -3));
        QLinearGradient rowTop(body.topLeft(), QPoint(body.left(), body.top() + 18));
        rowTop.setColorAt(0.0, QColor(255, 240, 195, disabled ? 52 : 70));
        rowTop.setColorAt(1.0, QColor(255, 240, 195, 0));
        p.fillRect(QRect(body.left(), body.top(), body.width(), 18), rowTop);
        QLinearGradient rowBottom(QPoint(body.left(), body.bottom() - 18), body.bottomLeft());
        rowBottom.setColorAt(0.0, QColor(92, 48, 13, 0));
        rowBottom.setColorAt(1.0, QColor(126, 70, 20, disabled ? 30 : 44));
        p.fillRect(QRect(body.left(), body.bottom() - 18, body.width(), 19), rowBottom);
        QLinearGradient rowLeft(body.topLeft(), body.topLeft() + QPoint(28, 0));
        rowLeft.setColorAt(0.0, QColor(124, 69, 20, disabled ? 30 : 42));
        rowLeft.setColorAt(1.0, QColor(124, 69, 20, 0));
        p.fillRect(QRect(body.left(), body.top(), 28, body.height()), rowLeft);
        QLinearGradient rowRight(body.topRight() - QPoint(28, 0), body.topRight());
        rowRight.setColorAt(0.0, QColor(124, 69, 20, 0));
        rowRight.setColorAt(1.0, QColor(124, 69, 20, disabled ? 26 : 38));
        p.fillRect(QRect(body.right() - 28, body.top(), 29, body.height()), rowRight);
        p.setClipping(false);

        QLinearGradient bevel(row.topLeft(), row.bottomLeft());
        bevel.setColorAt(0.0, QColor(241, 187, 69, disabled ? 78 : 132));
        bevel.setColorAt(0.48, QColor(166, 91, 27, disabled ? 72 : 126));
        bevel.setColorAt(1.0, QColor(74, 34, 9, disabled ? 116 : 170));
        p.setPen(QPen(QBrush(bevel), selected ? 4 : 2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(row.adjusted(1, 1, -1, -1), 7, 7);

        p.setClipRect(body.adjusted(6, 5, -6, -5));
        p.setPen(QPen(QColor(139, 86, 32, disabled ? 28 : 38), 1));
        for (int y = row.top() + 14; y < row.bottom() - 8; y += 15) {
            p.drawLine(row.left() + 18, y, row.right() - 20, y + ((y / 15) % 2));
        }
        p.setPen(QPen(QColor(255, 237, 185, disabled ? 28 : 36), 1));
        for (int y = row.top() + 20; y < row.bottom() - 8; y += 22) {
            p.drawLine(row.left() + 28, y, row.right() - 46, y - 1);
        }
        p.setPen(Qt::NoPen);
        for (int i = 0; i < 16; ++i) {
            const int x = row.left() + 18 + (i * 43 + i * i * 3) % std::max(1, row.width() - 42);
            const int y = row.top() + 8 + (i * 19 + i * i) % std::max(1, row.height() - 16);
            p.setBrush(QColor(122, 72, 21, disabled ? 8 : 14));
            p.drawEllipse(QPoint(x, y), 1 + (i % 3), 1 + ((i + 1) % 2));
        }
        for (int i = 0; i < 20; ++i) {
            const int x = body.left() + 14 + (i * 31 + i * i * 5) % std::max(1, body.width() - 28);
            const int y = body.top() + 10 + (i * 13 + i * i * 3) % std::max(1, body.height() - 18);
            p.setPen(QPen(QColor(255, 244, 204, disabled ? 16 : 24), 1));
            p.drawLine(x, y, std::min(body.right() - 10, x + 6 + (i % 4) * 4), y - 1);
            p.setPen(QPen(QColor(120, 70, 21, disabled ? 14 : 20), 1));
            p.drawLine(x + 2, y + 2, std::min(body.right() - 12, x + 11 + (i % 3) * 5), y + 2);
        }
        p.setPen(QPen(QColor(118, 65, 18, disabled ? 26 : 38), 1));
        for (int i = 0; i < 7; ++i) {
            const int x = body.left() + 16 + (i * 39) % std::max(1, body.width() - 54);
            const int y = body.top() + 8 + (i * 17) % std::max(1, body.height() - 16);
            p.drawLine(x, y, x + 9 + (i % 3) * 7, y + ((i % 2) ? 1 : -1));
        }
        p.setClipping(false);
        p.setPen(QPen(QColor(112, 63, 18, disabled ? 54 : 72), 1));
        for (int i = 0; i < 7; ++i) {
            const int x = body.left() + 18 + (i * 41) % std::max(1, body.width() - 36);
            p.drawLine(x, body.top() + 2 + (i % 2), std::min(body.right() - 12, x + 8 + (i % 4) * 5), body.top() + 2);
            p.drawLine(x + 4, body.bottom() - 2 - (i % 2), std::min(body.right() - 12, x + 14 + (i % 3) * 6), body.bottom() - 2);
        }

        if (selected) {
            p.setPen(QPen(QColor(218, 155, 45, 220), 2));
            p.drawRoundedRect(row.adjusted(5, 5, -5, -5), 6, 6);
            p.setPen(QPen(QColor(255, 226, 115, 70), 1));
            p.drawLine(row.left() + 18, row.top() + 9, row.right() - 22, row.top() + 8);
            p.setBrush(QColor(205, 135, 33, 210));
            p.setPen(QPen(QColor(59, 25, 6), 1));
            QPolygon pointer;
            pointer << QPoint(row.left() - 13, row.center().y())
                    << QPoint(row.left() - 2, row.center().y() - 9)
                    << QPoint(row.left() - 2, row.center().y() + 9);
            p.drawPolygon(pointer);
        } else if (isHovered(row) && !disabled) {
            p.fillRect(row.adjusted(9, 9, -9, -9), QColor(255, 218, 122, 28));
        }
        if (disabled) {
            p.fillRect(row.adjusted(5, 5, -5, -5), QColor(160, 106, 42, 18));
        }
        p.restore();
    }

    if (!icon.isNull()) {
        QRect slotRect(row.left() + 8, row.top() + 6, 74, 54);
        if (!m_slotFrame.isNull()) {
            p.drawPixmap(slotRect, m_slotFrame);
            if (selected) {
                p.setPen(QPen(QColor(255, 203, 62), 2));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(slotRect.adjusted(3, 3, -3, -3), 4, 4);
            }
        } else {
            drawIconWell(p, slotRect, selected, disabled);
        }
        drawPixmapFit(p, icon, QRect(row.left() + 18, row.top() + 10, 54, 46));
    } else {
        QRect slotRect(row.left() + 8, row.top() + 6, 74, 54);
        if (!m_slotFrame.isNull()) {
            p.drawPixmap(slotRect, m_slotFrame);
            p.fillRect(slotRect.adjusted(8, 8, -8, -8), QColor(72, 52, 32, 70));
        } else {
            drawIconWell(p, slotRect, selected, true);
        }
    }

    const QColor textColor = disabled ? QColor(101, 67, 34) : QColor(49, 27, 9);
    drawTextShadow(p, QRect(row.left() + 92, row.top() + 8, 154, 22), title,
                   uiFont(13, QFont::Bold), textColor, Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, QRect(row.left() + 92, row.top() + 33, 164, 20), subtitle,
                   uiFont(10, QFont::Bold), textColor, Qt::AlignLeft | Qt::AlignVCenter);

    QRect tagRect(row.right() - 82, row.top() + 18, 70, 24);
    drawTagBase(p, tagRect, tagColor, disabled);
    drawTextShadow(p, tagRect, tag, uiFont(10, QFont::Bold),
                   disabled ? QColor(196, 173, 122) : QColor(255, 241, 184));
}

void BackpackDialog::drawButton(QPainter& p, const QRect& rect, const QPixmap& bg,
                                const QString& text, Action action, int index, bool disabled)
{
    QColor color("#607d20");
    if (&bg == &m_buttonBlue) color = QColor("#285f78");
    if (&bg == &m_buttonRed) color = QColor("#8a3d1c");
    if (!bg.isNull()) {
        p.drawPixmap(rect, bg);
        if (isHovered(rect) && !disabled) {
            p.fillRect(rect.adjusted(12, 10, -12, -10), QColor(255, 230, 128, 38));
        }
        if (disabled) {
            p.fillRect(rect.adjusted(8, 8, -8, -8), QColor(30, 22, 14, 116));
        }
    } else {
        drawButtonBase(p, rect, color, disabled, isHovered(rect));
    }
    drawTextShadow(p, rect.adjusted(16, 0, -16, -2), text,
                   titleFont(17, QFont::Bold), disabled ? QColor(155, 130, 92) : QColor(255, 234, 164));
    if (!disabled) addZone(rect, action, index);
}

void BackpackDialog::drawTextShadow(QPainter& p, const QRect& rect, const QString& text,
                                    const QFont& font, const QColor& color, int flags)
{
    if (rect.isEmpty() || text.isEmpty()) return;

    QFont cleanFont = font;
    if (cleanFont.pointSize() <= 0) {
        cleanFont.setPointSize(10);
    }
    cleanFont.setStyleStrategy(QFont::PreferAntialias);
    cleanFont.setHintingPreference(QFont::PreferFullHinting);

    const bool wraps = (flags & Qt::TextWordWrap) || text.contains('\n');
    const int drawFlags = wraps
        ? (flags | Qt::TextWordWrap | Qt::TextWrapAnywhere)
        : (flags & ~Qt::TextWordWrap);
    const int minPointSize = wraps ? 8 : 9;
    const int hPad = rect.width() <= 64 ? 1 : 3;
    const int vPad = rect.height() <= 22 ? 0 : 1;
    QRect textRect = rect.adjusted(hPad, vPad, -hPad, -vPad);
    if (textRect.width() <= 0 || textRect.height() <= 0) return;

    while (cleanFont.pointSize() > minPointSize) {
        QFontMetrics metrics(cleanFont);
        QRect measured = metrics.boundingRect(textRect, drawFlags, text);
        if (measured.width() <= textRect.width() + 1 && measured.height() <= textRect.height() + 1) {
            break;
        }
        cleanFont.setPointSize(cleanFont.pointSize() - 1);
    }

    QString drawText = text;
    if (!wraps) {
        QFontMetrics metrics(cleanFont);
        drawText = metrics.elidedText(text, Qt::ElideRight, textRect.width());
    }

    const int brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    p.save();
    p.setClipRect(rect.adjusted(1, 1, -1, -1));
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(cleanFont);
    if (brightness > 150 && color.alpha() > 150) {
        p.setPen(QColor(26, 13, 5, 120));
        p.drawText(textRect.translated(1, 1), drawFlags, drawText);
    }
    p.setPen(color);
    p.drawText(textRect, drawFlags, drawText);
    p.restore();
}

void BackpackDialog::drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& rect)
{
    if (pixmap.isNull() || rect.isEmpty()) return;
    QSize target = pixmap.size();
    target.scale(rect.size(), Qt::KeepAspectRatio);
    QRect targetRect(QPoint(0, 0), target);
    targetRect.moveCenter(rect.center());
    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawPixmap(targetRect, pixmap);
    p.restore();
}

void BackpackDialog::drawStatLine(QPainter& p, int& y, const QString& label, const QString& value)
{
    const int statsLeft = kDetailPanelRect.left() + 236;
    const int statsRight = kDetailPanelRect.right() - 30;
    QRect labelRect(statsLeft + 22, y, 76, 22);
    QRect valueRect(statsLeft + 100, y, std::max(82, statsRight - (statsLeft + 100) + 1), 22);
    drawTextShadow(p, labelRect, label + QStringLiteral("："), uiFont(11, QFont::Bold),
                   QColor(70, 39, 13), Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, valueRect, value, uiFont(11, QFont::Bold),
                   QColor(38, 23, 9), Qt::AlignLeft | Qt::AlignVCenter);
    p.save();
    p.setPen(QPen(QColor(76, 43, 13, 66), 1));
    p.drawLine(statsLeft + 18, y + 23, statsLeft + 132, y + 23);
    p.drawLine(statsLeft + 146, y + 24, statsRight - 12, y + 23);
    p.setPen(QPen(QColor(232, 199, 129, 18), 1));
    p.drawLine(statsLeft + 24, y + 24, statsRight - 28, y + 24);
    p.restore();
    y += 28;
}

QVector<BackpackDialog::ItemDef> BackpackDialog::itemDefs() const
{
    return {
        {InventoryItemType::Food, QStringLiteral("航海干粮"), QStringLiteral("恢复 30 体力"),
         QStringLiteral("压缩干粮与淡水补给。适合长航程中快速恢复体力。"), &m_iconFood},
        {InventoryItemType::ShipRepairT1, QStringLiteral("初级船体修理包"), QStringLiteral("恢复 20 耐久"),
         QStringLiteral("基础船板与补漏材料，适合处理轻微碰撞损伤。"), &m_iconRepair1},
        {InventoryItemType::ShipRepairT2, QStringLiteral("中级船体修理包"), QStringLiteral("恢复 40 耐久"),
         QStringLiteral("更可靠的船体修复材料，可处理较严重的破损。"), &m_iconRepair2},
        {InventoryItemType::ShipRepairT3, QStringLiteral("高级船体修理包"), QStringLiteral("恢复 100 耐久"),
         QStringLiteral("专业级修理套件，关键时刻可以救回濒危船体。"), &m_iconRepair3},
        {InventoryItemType::EmergencyWeaponRepair, QStringLiteral("紧急装备修理工具"), QStringLiteral("修复装备 25% 耐久"),
         QStringLiteral("小型工具组，优先修复当前选中的背包装备。"), &m_iconEmergencyRepair}
    };
}

const BackpackDialog::ItemDef* BackpackDialog::selectedItemDef() const
{
    const QVector<ItemDef> defs = itemDefs();
    if (m_selectedItemIndex < 0 || m_selectedItemIndex >= defs.size()) return nullptr;
    m_selectedItemCache = defs[m_selectedItemIndex];
    return &m_selectedItemCache;
}

const Weapon* BackpackDialog::selectedWeapon() const
{
    const auto& weapons = InventorySystem::instance().weapons();
    if (m_selectedEquipmentIndex < 0 || m_selectedEquipmentIndex >= static_cast<int>(weapons.size())) return nullptr;
    return weapons[m_selectedEquipmentIndex];
}

Weapon* BackpackDialog::selectedWeapon()
{
    const auto& weapons = InventorySystem::instance().weapons();
    if (m_selectedEquipmentIndex < 0 || m_selectedEquipmentIndex >= static_cast<int>(weapons.size())) return nullptr;
    return weapons[m_selectedEquipmentIndex];
}

QPixmap BackpackDialog::weaponIcon(const QString& typeCode) const
{
    if (typeCode == QStringLiteral("Rod")) return m_iconRod;
    if (typeCode == QStringLiteral("Net")) return m_iconNet;
    if (typeCode == QStringLiteral("Harpoon")) return m_iconHarpoon;
    if (typeCode == QStringLiteral("Pistol")) return m_iconPistol;
    if (typeCode == QStringLiteral("Shotgun")) return m_iconShotgun;
    return m_iconRod;
}

int BackpackDialog::selectedItemCount() const
{
    const ItemDef* item = selectedItemDef();
    return item ? InventorySystem::instance().getItemCount(item->type) : 0;
}

QString BackpackDialog::weaponRoleText(const Weapon* weapon) const
{
    if (!weapon) return QStringLiteral("未知");
    switch (weapon->getRole()) {
    case Config::EquipmentRole::FishingTool: return QStringLiteral("捕鱼工具");
    case Config::EquipmentRole::AttackWeapon: return QStringLiteral("攻击武器");
    case Config::EquipmentRole::HybridTool: return QStringLiteral("双用工具");
    }
    return QStringLiteral("未知装备");
}

QString BackpackDialog::fishingModeText(const Weapon* weapon) const
{
    if (!weapon) return QStringLiteral("未知");
    switch (weapon->getFishingMode()) {
    case Config::FishingMode::None: return QStringLiteral("不可捕鱼");
    case Config::FishingMode::QTE: return QStringLiteral("鱼竿 QTE");
    case Config::FishingMode::Calibration: return QStringLiteral("校准捕鱼");
    }
    return QStringLiteral("未知");
}

QString BackpackDialog::weaponStatusText(int index, const Weapon* weapon) const
{
    if (!weapon) return QStringLiteral("空");
    if (weapon->isBroken()) return QStringLiteral("已损坏");
    if (index == InventorySystem::instance().currentWeaponIndex()) return QStringLiteral("已装备");
    if (durabilityPercent(weapon) <= 30) return QStringLiteral("低耐久");
    return QStringLiteral("可装备");
}

QColor BackpackDialog::weaponStatusColor(int index, const Weapon* weapon) const
{
    if (!weapon) return QColor("#6b573d");
    if (weapon->isBroken()) return QColor("#9d3737");
    if (index == InventorySystem::instance().currentWeaponIndex()) return QColor("#61851f");
    if (durabilityPercent(weapon) <= 30) return QColor("#b36a18");
    return QColor("#247347");
}

bool BackpackDialog::canUseSelectedItem() const
{
    const ItemDef* item = selectedItemDef();
    if (!item || selectedItemCount() <= 0) return false;
    if (item->type == InventoryItemType::EmergencyWeaponRepair) {
        return InventorySystem::instance().currentWeaponIndex() >= 0;
    }
    return true;
}

bool BackpackDialog::canRepairSelectedWeapon() const
{
    const Weapon* weapon = selectedWeapon();
    if (!weapon) return false;
    if (weapon->isInfiniteDurability()) return false;
    if (weapon->getCurrentDur() >= weapon->getMaxDur()) return false;
    return InventorySystem::instance().getItemCount(InventoryItemType::EmergencyWeaponRepair) > 0;
}

void BackpackDialog::switchPage(Page page)
{
    m_page = page;
    clampSelections();
    update();
}

void BackpackDialog::selectNext(int delta)
{
    if (m_page == Page::Equipment) {
        const int count = static_cast<int>(InventorySystem::instance().weapons().size());
        if (count > 0) {
            m_selectedEquipmentIndex = std::clamp(m_selectedEquipmentIndex + delta, 0, count - 1);
        }
    } else {
        const int count = itemDefs().size();
        if (count > 0) {
            m_selectedItemIndex = std::clamp(m_selectedItemIndex + delta, 0, count - 1);
        }
    }
    update();
}

void BackpackDialog::handleAction(Action action, int index)
{
    InventorySystem& inv = InventorySystem::instance();
    switch (action) {
    case Action::SwitchEquipment:
        switchPage(Page::Equipment);
        return;
    case Action::SwitchItems:
        switchPage(Page::Items);
        return;
    case Action::SelectEquipment:
        m_selectedEquipmentIndex = index;
        m_page = Page::Equipment;
        setStatusMessage({});
        update();
        return;
    case Action::SelectItem:
        m_selectedItemIndex = index;
        m_page = Page::Items;
        setStatusMessage({});
        update();
        return;
    case Action::Equip:
        if (selectedWeapon() && inv.selectWeapon(m_selectedEquipmentIndex)) {
            setStatusMessage(QStringLiteral("已切换当前装备。"));
        } else {
            setStatusMessage(QStringLiteral("该装备暂时无法装备。"));
        }
        update();
        return;
    case Action::Repair:
        if (inv.useEmergencyWeaponRepair(m_selectedEquipmentIndex)) {
            setStatusMessage(QStringLiteral("装备耐久已修复。"));
        } else {
            setStatusMessage(QStringLiteral("没有可用修理工具，或装备无需修理。"));
        }
        update();
        return;
    case Action::AssignSlot: {
        const Weapon* weapon = selectedWeapon();
        if (!weapon) return;

        QStringList options;
        for (int slot = 0; slot < 6; ++slot) {
            const int weaponIndex = inv.weaponIndexForQuickSlot(slot);
            const Weapon* assigned =
                weaponIndex >= 0 && weaponIndex < static_cast<int>(inv.weapons().size())
                ? inv.weapons()[weaponIndex]
                : nullptr;
            options << QStringLiteral("%1号槽  %2")
                           .arg(slot + 1)
                           .arg(assigned ? QString::fromStdString(assigned->getName())
                                         : QStringLiteral("空"));
        }

        const int slot = GameUi::selectWoodOption(
            this,
            QStringLiteral("设置快捷槽"),
            QStringLiteral("选择该武器要放入的快捷位置"),
            options
        );
        if (slot >= 0 && inv.assignWeaponToQuickSlot(m_selectedEquipmentIndex, slot)) {
            setStatusMessage(QStringLiteral("已放入 %1 号快捷槽。").arg(slot + 1));
        }
        update();
        return;
    }
    case Action::Discard: {
        const Weapon* weapon = selectedWeapon();
        if (!weapon || !weapon->isBroken()) {
            setStatusMessage(QStringLiteral("只有已损坏的武器可以丢弃。"));
            update();
            return;
        }

        const int choice = GameUi::selectWoodOption(
            this,
            QStringLiteral("丢弃损坏武器"),
            QStringLiteral("丢弃后无法找回：%1").arg(QString::fromStdString(weapon->getName())),
            {QStringLiteral("确认丢弃")}
        );
        if (choice == 0 && inv.removeWeapon(m_selectedEquipmentIndex)) {
            clampSelections();
            setStatusMessage(QStringLiteral("损坏武器已丢弃。"));
        }
        update();
        return;
    }
    case Action::Use: {
        const ItemDef* item = selectedItemDef();
        if (!item) return;
        bool ok = false;
        if (item->type == InventoryItemType::Food) {
            ok = inv.useFood(Player::instance());
        } else if (item->type == InventoryItemType::ShipRepairT1) {
            ok = inv.useShipRepairKit(Player::instance(), 1);
        } else if (item->type == InventoryItemType::ShipRepairT2) {
            ok = inv.useShipRepairKit(Player::instance(), 2);
        } else if (item->type == InventoryItemType::ShipRepairT3) {
            ok = inv.useShipRepairKit(Player::instance(), 3);
        } else if (item->type == InventoryItemType::EmergencyWeaponRepair) {
            const int weaponIndex = selectedWeapon() ? m_selectedEquipmentIndex : inv.currentWeaponIndex();
            ok = inv.useEmergencyWeaponRepair(weaponIndex);
        }
        setStatusMessage(ok ? QStringLiteral("道具已使用。") : QStringLiteral("该道具现在无法使用。"));
        update();
        return;
    }
    case Action::Close:
        accept();
        return;
    }
}

void BackpackDialog::clampSelections()
{
    const int weaponCount = static_cast<int>(InventorySystem::instance().weapons().size());
    if (weaponCount <= 0) {
        m_selectedEquipmentIndex = -1;
    } else {
        if (m_selectedEquipmentIndex < 0) {
            m_selectedEquipmentIndex = std::max(0, InventorySystem::instance().currentWeaponIndex());
        }
        m_selectedEquipmentIndex = std::clamp(m_selectedEquipmentIndex, 0, weaponCount - 1);
    }
    const int itemCount = itemDefs().size();
    m_selectedItemIndex = itemCount <= 0 ? 0 : std::clamp(m_selectedItemIndex, 0, itemCount - 1);
}

void BackpackDialog::addZone(const QRect& rect, Action action, int index)
{
    m_zones.push_back(ClickZone{rect, action, index});
}

const BackpackDialog::ClickZone* BackpackDialog::zoneAt(const QPoint& pos) const
{
    for (const ClickZone& zone : m_zones) {
        if (zone.rect.contains(pos)) return &zone;
    }
    return nullptr;
}

bool BackpackDialog::isHovered(const QRect& rect) const
{
    return rect.contains(m_mousePos);
}

void BackpackDialog::setStatusMessage(const QString& text)
{
    m_statusMessage = text;
}
