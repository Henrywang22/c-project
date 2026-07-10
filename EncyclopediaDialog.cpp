#include "EncyclopediaDialog.h"
#include "FileManager.h"
#include "GameConfig.h"

#include <QFontDatabase>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPolygonF>
#include <algorithm>
#include <cmath>

namespace {
constexpr QRect kFrameRect(20, 8, 1240, 704);
constexpr QRect kLeftPageRect(165, 150, 430, 476);
constexpr QRect kRightPageRect(650, 150, 458, 476);
constexpr QRect kProgressRect(210, 166, 300, 34);
constexpr QRect kDetailTitleRect(755, 166, 250, 42);
constexpr QRect kDetailImageRect(690, 218, 378, 142);
constexpr QRect kStatsRect(690, 372, 378, 126);
constexpr QRect kDescriptionRect(690, 506, 378, 82);
constexpr int kRowTop = 210;
constexpr int kRowHeight = 62;
constexpr int kRowGap = 7;
constexpr int kVisibleRows = 6;

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
        QStringLiteral("SimSun"),
        QStringLiteral("宋体")
    });
    QFont font(family, size, weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QFont titleFont(int size, int weight = QFont::Bold)
{
    static const QString family = firstAvailableFont({
        QStringLiteral("STXinwei"),
        QStringLiteral("华文新魏"),
        QStringLiteral("FZShuTi"),
        QStringLiteral("方正舒体"),
        QStringLiteral("STKaiti"),
        QStringLiteral("华文楷体"),
        QStringLiteral("Microsoft YaHei UI")
    });
    QFont font(family, size, weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QPixmap pixmapFromPath(const QString& path)
{
    QPixmap pixmap;
    if (!path.isEmpty()) pixmap.load(path);
    return pixmap;
}

qreal clamp01(qreal value)
{
    return std::clamp(value, 0.0, 1.0);
}

qreal easeOutCubic(qreal value)
{
    const qreal t = clamp01(value);
    return 1.0 - std::pow(1.0 - t, 3.0);
}

qreal easeInCubic(qreal value)
{
    const qreal t = clamp01(value);
    return t * t * t;
}

qreal easeInOut(qreal value)
{
    const qreal t = clamp01(value);
    return t * t * (3.0 - 2.0 * t);
}

qreal segmentProgress(qreal value, qreal start, qreal end)
{
    if (end <= start) return value >= end ? 1.0 : 0.0;
    return clamp01((value - start) / (end - start));
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

void drawCornerRivets(QPainter& p, const QRect& rect, const QColor& brass, const QColor& shadow)
{
    const QVector<QRect> rivets = {
        QRect(rect.left() + 5, rect.top() + 5, 6, 6),
        QRect(rect.right() - 10, rect.top() + 5, 6, 6),
        QRect(rect.left() + 5, rect.bottom() - 10, 6, 6),
        QRect(rect.right() - 10, rect.bottom() - 10, 6, 6)
    };
    for (const QRect& rivet : rivets) {
        p.fillRect(rivet.translated(1, 1), shadow);
        p.fillRect(rivet, brass);
        p.fillRect(rivet.adjusted(1, 1, -3, -3), QColor(255, 228, 122, 190));
    }
}

void drawPixelPanel(QPainter& p, const QRect& rect, const QColor& fill,
                    const QColor& border, bool rivets = true)
{
    if (rect.isEmpty()) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect.translated(4, 5), QColor(43, 24, 8, 74));
    p.fillRect(rect, border);

    const QRect mid = rect.adjusted(3, 3, -3, -3);
    p.fillRect(mid, QColor(87, 52, 22, 180));

    const QRect inner = rect.adjusted(6, 6, -6, -6);
    p.fillRect(inner, fill);

    p.setPen(QPen(QColor(255, 238, 180, 82), 1));
    p.drawLine(inner.left(), inner.top(), inner.right(), inner.top());
    p.drawLine(inner.left(), inner.top(), inner.left(), inner.bottom());
    p.setPen(QPen(QColor(42, 24, 9, 105), 1));
    p.drawLine(inner.left(), inner.bottom(), inner.right(), inner.bottom());
    p.drawLine(inner.right(), inner.top(), inner.right(), inner.bottom());

    if (rivets && rect.width() > 36 && rect.height() > 28) {
        drawCornerRivets(p, rect, QColor(202, 145, 45), QColor(54, 31, 9, 135));
    }

    p.restore();
}

void drawNauticalGrid(QPainter& p, const QRect& rect, const QColor& color)
{
    if (rect.isEmpty()) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setClipRect(rect);

    p.setPen(QPen(color, 1, Qt::DotLine));
    for (int x = rect.left() + 12; x < rect.right(); x += 28) {
        p.drawLine(x, rect.top(), x, rect.bottom());
    }
    for (int y = rect.top() + 12; y < rect.bottom(); y += 28) {
        p.drawLine(rect.left(), y, rect.right(), y);
    }

    p.setPen(QPen(withAlpha(color, std::min(160, color.alpha() + 38)), 1));
    p.drawLine(rect.left() + 10, rect.bottom() - 14, rect.right() - 18, rect.top() + 8);
    p.drawLine(rect.left() + rect.width() / 3, rect.top() + 8,
               rect.right() - 12, rect.bottom() - 12);
    p.restore();
}

void drawCompassRose(QPainter& p, const QPoint& center, int radius, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(color, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    p.setBrush(withAlpha(color, 110));
    QPolygon north;
    north << QPoint(center.x(), center.y() - radius + 2)
          << QPoint(center.x() - 4, center.y())
          << QPoint(center.x() + 4, center.y());
    QPolygon south;
    south << QPoint(center.x(), center.y() + radius - 2)
          << QPoint(center.x() - 4, center.y())
          << QPoint(center.x() + 4, center.y());
    QPolygon west;
    west << QPoint(center.x() - radius + 2, center.y())
         << QPoint(center.x(), center.y() - 4)
         << QPoint(center.x(), center.y() + 4);
    QPolygon east;
    east << QPoint(center.x() + radius - 2, center.y())
         << QPoint(center.x(), center.y() - 4)
         << QPoint(center.x(), center.y() + 4);
    p.drawPolygon(north);
    p.drawPolygon(south);
    p.drawPolygon(west);
    p.drawPolygon(east);
    p.restore();
}

void drawRopeLine(QPainter& p, int x1, int y, int x2)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(84, 55, 25, 150), 4));
    p.drawLine(x1, y + 1, x2, y + 1);
    p.setPen(QPen(QColor(193, 151, 83, 210), 2));
    p.drawLine(x1, y, x2, y);
    p.setPen(QPen(QColor(255, 226, 142, 105), 1));
    for (int x = x1 + 4; x < x2; x += 12) {
        p.drawLine(x, y - 3, x + 7, y + 3);
    }
    p.restore();
}

void drawPagePolygon(QPainter& p, const QPolygonF& polygon, const QColor& fill, const QColor& edge)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(43, 24, 8, 86));
    p.drawPolygon(polygon.translated(5, 7));
    p.setBrush(edge);
    p.drawPolygon(polygon);

    QPainterPath pagePath;
    pagePath.addPolygon(polygon);
    p.setClipPath(pagePath);

    const QRect bounds = polygon.boundingRect().toRect();
    p.fillRect(bounds.adjusted(5, 5, -5, -5), fill);
    drawNauticalGrid(p, bounds.adjusted(24, 24, -24, -24), QColor(113, 79, 35, 42));
    p.restore();
}

void drawNinePatch(QPainter& p, const QPixmap& pixmap, const QRect& target, int margin)
{
    if (pixmap.isNull() || target.isEmpty()) return;

    const int sw = pixmap.width();
    const int sh = pixmap.height();
    const int m = std::min({ margin, sw / 2 - 1, sh / 2 - 1,
                             std::max(1, target.width() / 2 - 1),
                             std::max(1, target.height() / 2 - 1) });
    if (m <= 0) {
        p.drawPixmap(target, pixmap);
        return;
    }

    const QRect src[9] = {
        QRect(0, 0, m, m),
        QRect(m, 0, sw - 2 * m, m),
        QRect(sw - m, 0, m, m),
        QRect(0, m, m, sh - 2 * m),
        QRect(m, m, sw - 2 * m, sh - 2 * m),
        QRect(sw - m, m, m, sh - 2 * m),
        QRect(0, sh - m, m, m),
        QRect(m, sh - m, sw - 2 * m, m),
        QRect(sw - m, sh - m, m, m)
    };
    const QRect dst[9] = {
        QRect(target.left(), target.top(), m, m),
        QRect(target.left() + m, target.top(), target.width() - 2 * m, m),
        QRect(target.right() - m + 1, target.top(), m, m),
        QRect(target.left(), target.top() + m, m, target.height() - 2 * m),
        QRect(target.left() + m, target.top() + m, target.width() - 2 * m, target.height() - 2 * m),
        QRect(target.right() - m + 1, target.top() + m, m, target.height() - 2 * m),
        QRect(target.left(), target.bottom() - m + 1, m, m),
        QRect(target.left() + m, target.bottom() - m + 1, target.width() - 2 * m, m),
        QRect(target.right() - m + 1, target.bottom() - m + 1, m, m)
    };

    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int i = 0; i < 9; ++i) {
        if (!src[i].isEmpty() && !dst[i].isEmpty()) {
            p.drawPixmap(dst[i], pixmap, src[i]);
        }
    }
    p.restore();
}

void drawTexturedPage(QPainter& p, const QPixmap& page, const QPolygonF& polygon)
{
    if (page.isNull() || polygon.isEmpty()) {
        drawPagePolygon(p, polygon, QColor(232, 204, 151, 238), QColor(116, 72, 25));
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(43, 24, 8, 86));
    p.setPen(Qt::NoPen);
    p.drawPolygon(polygon.translated(5, 7));

    QPainterPath path;
    path.addPolygon(polygon);
    p.setClipPath(path);
    p.drawPixmap(polygon.boundingRect().toRect(), page);
    p.setClipping(false);
    p.setPen(QPen(QColor(91, 54, 19, 160), 2));
    p.drawPolygon(polygon);
    p.setPen(QPen(QColor(255, 240, 186, 80), 1));
    p.drawLine(polygon.at(0), polygon.at(1));
    p.restore();
}

void drawInkRule(QPainter& p, int x1, int y, int x2, const QColor& color, int alpha = 120)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(withAlpha(color, alpha), 1));
    p.drawLine(x1, y, x2, y);
    p.setPen(QPen(QColor(255, 242, 194, alpha / 3), 1));
    p.drawLine(x1 + 7, y - 1, x2 - 13, y - 1);
    p.restore();
}

void drawPageStackEdge(QPainter& p, int spineX, int top, int bottom, int pageWidth, qreal openT)
{
    if (pageWidth <= 24) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    const int leftOuter = spineX - pageWidth;
    const int rightOuter = spineX + pageWidth;
    const int thickness = int(18 + 22 * openT);

    for (int layer = thickness; layer >= 0; layer -= 3) {
        const int shade = 118 + layer * 3;
        const QColor pageEdge(std::min(236, shade + 32), std::min(211, shade + 14), std::min(160, shade), 210);
        const QColor line(90, 55, 22, 80);
        const int yOff = layer;
        const int sideOff = layer / 3;

        QPolygonF leftStack;
        leftStack << QPointF(spineX - 4, top + 12 + yOff / 3)
                  << QPointF(leftOuter - sideOff, top + 22 + yOff / 2)
                  << QPointF(leftOuter - sideOff + 8, bottom + yOff)
                  << QPointF(spineX - 5, bottom + yOff / 2);
        QPolygonF rightStack;
        rightStack << QPointF(spineX + 4, top + 12 + yOff / 3)
                   << QPointF(rightOuter + sideOff, top + 22 + yOff / 2)
                   << QPointF(rightOuter + sideOff - 8, bottom + yOff)
                   << QPointF(spineX + 5, bottom + yOff / 2);

        p.setPen(Qt::NoPen);
        p.setBrush(pageEdge);
        p.drawPolygon(leftStack);
        p.drawPolygon(rightStack);

        p.setPen(QPen(line, 1));
        p.drawLine(QPointF(leftOuter + 18 - sideOff, bottom + yOff - 2),
                   QPointF(spineX - 18, bottom + yOff / 2));
        p.drawLine(QPointF(rightOuter - 18 + sideOff, bottom + yOff - 2),
                   QPointF(spineX + 18, bottom + yOff / 2));
    }

    p.setPen(QPen(QColor(63, 35, 14, 170), 3));
    p.drawLine(QPoint(spineX - pageWidth + 8, bottom + thickness),
               QPoint(spineX - 16, bottom + thickness / 2));
    p.drawLine(QPoint(spineX + pageWidth - 8, bottom + thickness),
               QPoint(spineX + 16, bottom + thickness / 2));
    p.restore();
}

void drawLeatherBoardThickness(QPainter& p, int spineX, int top, int bottom, int pageWidth, qreal openT)
{
    if (pageWidth <= 18) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    const int boardDrop = int(24 + 18 * openT);
    const int side = int(18 * openT);
    QColor leatherDark(42, 22, 8, 210);
    QColor woodEdge(96, 50, 20, 190);

    QPolygonF leftBoard;
    leftBoard << QPointF(spineX - 10, top + 20)
              << QPointF(spineX - pageWidth - side, top + 34)
              << QPointF(spineX - pageWidth - side + 10, bottom + boardDrop)
              << QPointF(spineX - 8, bottom + boardDrop / 2);
    QPolygonF rightBoard;
    rightBoard << QPointF(spineX + 10, top + 20)
               << QPointF(spineX + pageWidth + side, top + 34)
               << QPointF(spineX + pageWidth + side - 10, bottom + boardDrop)
               << QPointF(spineX + 8, bottom + boardDrop / 2);

    p.setPen(Qt::NoPen);
    p.setBrush(leatherDark);
    p.drawPolygon(leftBoard);
    p.drawPolygon(rightBoard);
    p.setPen(QPen(woodEdge, 3));
    p.drawLine(QPointF(spineX - pageWidth - side + 10, bottom + boardDrop),
               QPointF(spineX - 8, bottom + boardDrop / 2));
    p.drawLine(QPointF(spineX + pageWidth + side - 10, bottom + boardDrop),
               QPointF(spineX + 8, bottom + boardDrop / 2));
    p.restore();
}

void drawBookOpeningMass(QPainter& p, int spineX, int top, int bottom, int pageWidth, qreal openT)
{
    drawLeatherBoardThickness(p, spineX, top, bottom, pageWidth, openT);
    drawPageStackEdge(p, spineX, top, bottom, pageWidth, openT);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    const int shadowWidth = int(24 + 18 * openT);
    QLinearGradient spineShadow(spineX - shadowWidth, 0, spineX + shadowWidth, 0);
    spineShadow.setColorAt(0.0, QColor(35, 18, 7, 0));
    spineShadow.setColorAt(0.48, QColor(35, 18, 7, 160));
    spineShadow.setColorAt(0.52, QColor(11, 6, 3, 210));
    spineShadow.setColorAt(1.0, QColor(35, 18, 7, 0));
    p.fillRect(QRect(spineX - shadowWidth, top + 8, shadowWidth * 2, bottom - top + 34), spineShadow);
    p.restore();
}

QPointF curledPagePoint(int spineX, int top, int bottom, int pageWidth, qreal progress,
                        qreal u, bool bottomPoint)
{
    constexpr qreal kPi = 3.14159265358979323846;
    const qreal turn = clamp01(progress);
    const qreal lift = std::sin(kPi * turn);
    const qreal curl = std::sin(kPi * u) * lift;
    const qreal side = (turn < 0.5) ? 1.0 : -1.0;
    const qreal fold = 1.0 - 2.0 * turn;
    const qreal edgeLift = std::sin(kPi * std::sqrt(std::max<qreal>(0.0, u))) * lift;
    const qreal x = spineX + pageWidth * (fold * u + side * (0.30 + 0.14 * turn) * curl);
    const qreal arch = (8.0 + 32.0 * curl) * lift;
    const qreal edgeSag = (8.0 + 7.0 * u) * edgeLift;
    const qreal y = bottomPoint
        ? bottom - arch * 0.32 + edgeSag
        : top + arch * 0.52 - edgeSag * 0.58;
    return QPointF(x, y);
}

QPainterPath curledPagePath(int spineX, int top, int bottom, int pageWidth, qreal progress,
                            int strips, QVector<QPointF>* topCurveOut = nullptr,
                            QVector<QPointF>* bottomCurveOut = nullptr)
{
    QVector<QPointF> topCurve;
    QVector<QPointF> bottomCurve;
    topCurve.reserve(strips + 1);
    bottomCurve.reserve(strips + 1);
    for (int i = 0; i <= strips; ++i) {
        const qreal u = qreal(i) / strips;
        topCurve.push_back(curledPagePoint(spineX, top, bottom, pageWidth, progress, u, false));
        bottomCurve.push_back(curledPagePoint(spineX, top, bottom, pageWidth, progress, u, true));
    }

    QPainterPath path;
    path.moveTo(topCurve.front());
    for (const QPointF& point : topCurve) path.lineTo(point);
    for (int i = bottomCurve.size() - 1; i >= 0; --i) path.lineTo(bottomCurve[i]);
    path.closeSubpath();

    if (topCurveOut) *topCurveOut = topCurve;
    if (bottomCurveOut) *bottomCurveOut = bottomCurve;
    return path;
}

void drawPageTurnGrounding(QPainter& p, int spineX, int top, int bottom,
                           int pageWidth, qreal progress)
{
    constexpr qreal kPi = 3.14159265358979323846;
    const qreal turn = clamp01(progress);
    const qreal lift = std::sin(kPi * turn);
    if (lift <= 0.01) return;

    const qreal freeX = spineX + pageWidth * (1.0 - 2.0 * turn);
    const int alpha = int(72 * lift);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    if (turn < 0.55) {
        const qreal lineX = std::clamp<qreal>(freeX, spineX + 14.0, spineX + pageWidth - 6.0);
        QLinearGradient liftedEdge(lineX - 34.0, 0.0, lineX + 38.0, 0.0);
        liftedEdge.setColorAt(0.0, QColor(48, 24, 7, 0));
        liftedEdge.setColorAt(0.42, QColor(39, 18, 5, alpha));
        liftedEdge.setColorAt(1.0, QColor(255, 238, 184, int(26 * lift)));
        p.fillRect(QRectF(lineX - 36.0, top + 18.0, 76.0, bottom - top - 30.0), liftedEdge);

        const qreal reveal = easeOutCubic(segmentProgress(turn, 0.04, 0.48));
        if (reveal > 0.01) {
            const QRectF revealRect(lineX, top + 26.0,
                                    spineX + pageWidth - lineX - 8.0,
                                    bottom - top - 44.0);
            p.fillRect(revealRect, QColor(255, 239, 187, int(20 * reveal)));
        }
    } else {
        const qreal lineX = std::clamp<qreal>(freeX, spineX - pageWidth + 8.0, spineX - 12.0);
        QLinearGradient landingShadow(lineX - 42.0, 0.0, lineX + 34.0, 0.0);
        landingShadow.setColorAt(0.0, QColor(0, 0, 0, 0));
        landingShadow.setColorAt(0.52, QColor(31, 15, 4, int(68 * lift)));
        landingShadow.setColorAt(1.0, QColor(0, 0, 0, 0));
        p.fillRect(QRectF(lineX - 42.0, top + 28.0, 76.0, bottom - top - 46.0), landingShadow);
    }

    QLinearGradient spineShade(spineX - 38.0, 0.0, spineX + 38.0, 0.0);
    spineShade.setColorAt(0.0, QColor(42, 20, 6, 0));
    spineShade.setColorAt(0.48, QColor(24, 10, 3, int(62 * lift)));
    spineShade.setColorAt(0.52, QColor(24, 10, 3, int(78 * lift)));
    spineShade.setColorAt(1.0, QColor(42, 20, 6, 0));
    p.fillRect(QRectF(spineX - 38.0, top + 16.0, 76.0, bottom - top - 18.0), spineShade);

    p.restore();
}

void drawCurledPage(QPainter& p, const QPixmap& page, int spineX, int top,
                    int bottom, int pageWidth, qreal progress)
{
    if (page.isNull() || pageWidth <= 0 || bottom <= top) return;

    constexpr qreal kPi = 3.14159265358979323846;
    const qreal turn = clamp01(progress);
    const int strips = 44;
    QVector<QPointF> topCurve;
    QVector<QPointF> bottomCurve;
    QPainterPath shadow = curledPagePath(spineX, top, bottom, pageWidth, turn,
                                         strips, &topCurve, &bottomCurve);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal shadowStrength = std::sin(kPi * turn);
    const qreal shadowOffset = (turn < 0.5 ? 1.0 : -1.0) * (9.0 + 18.0 * shadowStrength);
    p.fillPath(shadow.translated(shadowOffset, 16 + 8 * shadowStrength),
               QColor(20, 9, 3, int(118 * shadowStrength)));
    p.fillPath(shadow.translated(shadowOffset * 0.42, 6 + 5 * shadowStrength),
               QColor(71, 39, 13, int(54 * shadowStrength)));
    p.restore();

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    for (int i = 0; i < strips; ++i) {
        const qreal u0 = qreal(i) / strips;
        const qreal u1 = qreal(i + 1) / strips;
        const QPointF top0 = topCurve[i];
        const QPointF top1 = topCurve[i + 1];
        const QPointF bottom0 = bottomCurve[i];
        const QPointF bottom1 = bottomCurve[i + 1];

        const qreal left = std::min(top0.x(), top1.x());
        const qreal right = std::max(top0.x(), top1.x());
        const qreal targetWidth = std::max<qreal>(1.0, right - left + 1.0);
        const qreal stripTop = std::min(top0.y(), top1.y());
        const qreal stripBottom = std::max(bottom0.y(), bottom1.y());
        QRectF target(left, stripTop, targetWidth, std::max<qreal>(1.0, stripBottom - stripTop));
        QRectF source(page.width() * u0, 0.0,
                      std::max<qreal>(1.0, page.width() * (u1 - u0)), page.height());

        QPainterPath clip;
        clip.moveTo(top0);
        clip.lineTo(top1);
        clip.lineTo(bottom1);
        clip.lineTo(bottom0);
        clip.closeSubpath();

        p.save();
        p.setClipPath(clip);
        p.drawPixmap(target, page, source);

        const qreal u = (u0 + u1) * 0.5;
        const qreal curl = std::sin(kPi * u) * std::sin(kPi * turn);
        const qreal face = std::cos(kPi * turn);
        if (face < 0.0) {
            p.fillRect(target, QColor(115, 82, 44, int(30 + 40 * std::abs(face))));
        }

        const int darkAlpha = int(76 * curl + (face < 0.0 ? 22 * shadowStrength : 0));
        if (darkAlpha > 0) {
            p.fillRect(target, QColor(58, 32, 10, std::clamp(darkAlpha, 0, 138)));
        }
        const qreal ridge = std::exp(-std::pow((u - 0.56) / 0.16, 2.0)) * std::sin(kPi * turn);
        const int lightAlpha = int(118 * ridge);
        if (lightAlpha > 0) {
            p.fillRect(target.adjusted(0, 0, 0, -target.height() * 0.08),
                       QColor(255, 239, 184, std::clamp(lightAlpha, 0, 145)));
        }

        if (i % 6 == 0) {
            p.setPen(QPen(QColor(118, 78, 35, int(18 + 36 * std::sin(kPi * turn))), 1));
            p.drawLine(top0 * 0.82 + bottom0 * 0.18, top1 * 0.82 + bottom1 * 0.18);
            p.drawLine(top0 * 0.58 + bottom0 * 0.42, top1 * 0.58 + bottom1 * 0.42);
        }
        if (i % 12 == 3) {
            p.setPen(QPen(QColor(255, 239, 184, int(16 + 32 * curl)), 1));
            p.drawLine(top0, bottom0);
        }
        p.restore();
    }

    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath outline;
    outline.moveTo(topCurve.front());
    for (const QPointF& point : topCurve) outline.lineTo(point);
    for (int i = bottomCurve.size() - 1; i >= 0; --i) outline.lineTo(bottomCurve[i]);
    outline.closeSubpath();
    p.setPen(QPen(QColor(77, 45, 18, 160), 2));
    p.drawPath(outline);

    const QPointF freeTop = topCurve.back();
    const QPointF freeBottom = bottomCurve.back();
    p.setPen(QPen(QColor(42, 24, 8, 210), 5));
    p.drawLine(freeTop, freeBottom);
    p.setPen(QPen(QColor(128, 84, 36, 180), 3));
    p.drawLine(freeTop + QPointF((turn < 0.5) ? -1 : 1, 0),
               freeBottom + QPointF((turn < 0.5) ? -1 : 1, 0));
    p.setPen(QPen(QColor(255, 238, 181, 170), 1));
    p.drawLine(freeTop + QPointF((turn < 0.5) ? -2 : 2, 0),
               freeBottom + QPointF((turn < 0.5) ? -2 : 2, 0));

    const int ridgeIndex = std::clamp(int(0.56 * strips), 1, strips - 1);
    p.setPen(QPen(QColor(255, 247, 203, int(105 * std::sin(kPi * turn))), 2));
    p.drawLine(topCurve[ridgeIndex], bottomCurve[ridgeIndex]);

    const int contactIndex = std::clamp(int((turn < 0.5 ? 0.92 : 0.12) * strips), 1, strips - 1);
    p.setPen(QPen(QColor(55, 28, 8, int(62 * std::sin(kPi * turn))), 2));
    p.drawLine(topCurve[contactIndex], bottomCurve[contactIndex]);
    p.restore();
}

void drawOpeningCover(QPainter& p, const QPixmap& cover, const QRect& closedRect,
                      int spineX, qreal progress, qreal opacity)
{
    if (cover.isNull() || opacity <= 0.01) return;

    const qreal turn = easeInOut(progress);
    const qreal hingeX = closedRect.left() * (1.0 - turn) + spineX * turn;
    const qreal freeStart = closedRect.right();
    const qreal freeEnd = spineX - 536.0;
    const qreal freeX = freeStart * (1.0 - turn) + freeEnd * turn;
    const qreal targetWidth = std::abs(freeX - hingeX);
    if (targetWidth < 2.0) return;

    const qreal top = closedRect.top() * (1.0 - turn) + 42.0 * turn;
    const qreal height = closedRect.height() * (1.0 - turn) + 596.0 * turn;
    const QRectF target(std::min(freeX, hingeX), top, targetWidth, height);
    const bool showingBack = freeX < hingeX;

    p.save();
    p.setOpacity(opacity);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (showingBack) {
        p.save();
        p.translate(target.left() + target.width(), target.top());
        p.scale(-1.0, 1.0);
        p.drawPixmap(QRectF(0, 0, target.width(), target.height()),
                     cover, QRectF(0, 0, cover.width(), cover.height()));
        p.restore();
        p.fillRect(target, QColor(43, 24, 9, int(86 + 64 * turn)));
    } else {
        p.drawPixmap(target, cover, QRectF(0, 0, cover.width(), cover.height()));
    }

    const int hingeAlpha = int(90 + 82 * std::sin(3.14159265358979323846 * turn));
    p.fillRect(QRectF(hingeX - 3, target.top() + 18, 6, target.height() - 36),
               QColor(30, 14, 5, hingeAlpha));
    p.restore();
}

void drawOpeningBookFrame(QPainter& p, const QPixmap& book, qreal progress)
{
    if (book.isNull()) return;

    const qreal t = clamp01(progress);
    const qreal finalHalfWidth = kFrameRect.width() * 0.5;
    const qreal closedHalfWidth = 186.0;
    const qreal halfWidth = closedHalfWidth + (finalHalfWidth - closedHalfWidth) * t;
    const qreal top = 90.0 * (1.0 - t) + kFrameRect.top() * t;
    const qreal height = 538.0 * (1.0 - t) + kFrameRect.height() * t;
    const qreal spineX = kFrameRect.left() + finalHalfWidth;

    const QRectF leftTarget(spineX - halfWidth, top, halfWidth, height);
    const QRectF rightTarget(spineX, top, halfWidth, height);
    const QRectF leftSource(0, 0, book.width() * 0.5, book.height());
    const QRectF rightSource(book.width() * 0.5, 0, book.width() * 0.5, book.height());

    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    const QRectF shadowRect(spineX - halfWidth + 16, top + height - 20,
                            halfWidth * 2 - 32, 34);
    p.fillRect(shadowRect, QColor(0, 0, 0, int(42 + 46 * (1.0 - t))));

    p.drawPixmap(leftTarget, book, leftSource);
    p.drawPixmap(rightTarget, book, rightSource);

    if (t < 0.995) {
        const int coverAlpha = int(46 * (1.0 - t));
        p.fillRect(leftTarget, QColor(44, 24, 9, coverAlpha));
        p.fillRect(rightTarget, QColor(44, 24, 9, coverAlpha));

        QLinearGradient squeezeShade(spineX - halfWidth, 0, spineX + halfWidth, 0);
        squeezeShade.setColorAt(0.0, QColor(10, 5, 2, int(30 * (1.0 - t))));
        squeezeShade.setColorAt(0.46, QColor(10, 5, 2, int(18 + 42 * (1.0 - t))));
        squeezeShade.setColorAt(0.50, QColor(0, 0, 0, int(78 + 54 * (1.0 - t))));
        squeezeShade.setColorAt(0.54, QColor(10, 5, 2, int(18 + 42 * (1.0 - t))));
        squeezeShade.setColorAt(1.0, QColor(10, 5, 2, int(30 * (1.0 - t))));
        p.fillRect(QRectF(spineX - halfWidth, top, halfWidth * 2, height), squeezeShade);
    }

    p.fillRect(QRectF(spineX - 4, top + 18, 8, height - 36),
               QColor(21, 10, 4, int(92 + 78 * (1.0 - t))));
    p.restore();
}
}

EncyclopediaDialog::EncyclopediaDialog(int currentStage, QWidget* parent,
                                       bool currentStageBossEncountered)
    : QDialog(parent),
      m_currentStage(qBound(1, currentStage, Config::GameConfig::STAGE_COUNT)),
      m_currentStageBossEncountered(currentStageBossEncountered)
{
    setWindowTitle(QStringLiteral("航海图鉴"));
    setFixedSize(1280, 720);
    setMouseTracking(true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    loadAssets();
    buildCatalog();

    m_selectedByCategory.resize(m_pages.size());
    m_scrollByCategory.resize(m_pages.size());
    std::fill(m_selectedByCategory.begin(), m_selectedByCategory.end(), 0);
    std::fill(m_scrollByCategory.begin(), m_scrollByCategory.end(), 0);

    m_openTimer.setInterval(16);
    connect(&m_openTimer, &QTimer::timeout, this, [this]() { updateOpenAnimation(); });
    m_openTimer.start();
}

void EncyclopediaDialog::loadAssets()
{
    m_bookFrame.load(QStringLiteral(":/FishingVoyage/encyclopedia/book_frame.png"));
    m_fallbackSea.load(QStringLiteral(":/FishingVoyage/backgrounds/sea.png"));
    m_uiPanelParchment.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/panel_parchment.png"));
    m_uiPanelSeaChart.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/panel_sea_chart.png"));
    m_uiPanelStat.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/panel_stat.png"));
    m_uiDetailImagePanel.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/detail_image_panel.png"));
    m_uiIconFrame.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/icon_frame.png"));
    m_uiTabNormal.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/tab_normal.png"));
    m_uiTabHover.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/tab_hover.png"));
    m_uiTabSelected.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/tab_selected.png"));
    m_uiRowNormal.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/row_normal.png"));
    m_uiRowHover.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/row_hover.png"));
    m_uiRowSelected.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/row_selected.png"));
    m_uiTagBadge.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/tag_badge.png"));
    m_uiScrollTrack.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/scroll_track.png"));
    m_uiScrollThumb.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/scroll_thumb.png"));
    m_uiOpenClosedCover.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/open_closed_cover.png"));
    m_uiOpenPageLeft.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/open_page_left.png"));
    m_uiOpenPageRight.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/open_page_right.png"));
    m_uiOpenFlipPage.load(QStringLiteral(":/FishingVoyage/encyclopedia/ui/open_flip_page.png"));
}

void EncyclopediaDialog::buildCatalog()
{
    auto stat = [](const QString& label, const QString& value) {
        return StatLine{ label, value };
    };
    const int enemyStage = qBound(1, m_currentStage, Config::GameConfig::STAGE_COUNT);
    constexpr qreal kEnemyHpGrowth = 0.15;
    constexpr qreal kEnemyAttackGrowth = 0.12;
    constexpr qreal kEnemySpeedGrowth = 0.025;
    constexpr qreal kEnemyRewardGrowth = 0.15;
    constexpr int kFishCatalogTotal = 12;
    constexpr int kEquipmentCatalogTotal = 5;
    constexpr int kItemCatalogTotal = 5;
    constexpr int kEnemyCatalogTotal = 5;
    constexpr int kBossCatalogTotal = 2;
    auto scaledEnemyValue = [enemyStage](int base, qreal growth) {
        return QString::number(qRound(base * (1.0 + growth * (enemyStage - 1))));
    };
    auto scaledEnemySpeed = [enemyStage](qreal base) {
        return QString::number(base * (1.0 + kEnemySpeedGrowth * (enemyStage - 1)), 'f', 2);
    };
    auto configValue = [](int value) {
        return QString::number(value);
    };
    auto unknown = [](const QString& id, const QString& tagText = QStringLiteral("未发现")) {
        Entry entry;
        entry.id = id;
        entry.name = QStringLiteral("？？？");
        entry.tag = tagText;
        entry.discovered = false;
        entry.tagColor = QColor("#8a6a42");
        entry.description = QStringLiteral("这条记录还没有被写入航海日志。后续接入解锁系统后，可由存档数据控制显示。");
        return entry;
    };
    FileManager discoveryLog;
    auto setEntryDiscovery = [](Entry& entry, bool discovered) {
        entry.discovered = discovered;
        entry.tag = discovered
            ? QStringLiteral("已发现")
            : QStringLiteral("未发现");
        if (!discovered) {
            entry.tagColor = QColor("#8a6a42");
        }
    };
    auto setEntryDiscoveryAt = [&](CategoryPage& page, int index, bool discovered) {
        if (index < 0 || index >= page.entries.size()) return;
        setEntryDiscovery(page.entries[index], discovered);
    };
    auto syncPageCount = [](CategoryPage& page) {
        page.totalCount = page.entries.size();
        page.discoveredCount = 0;
        for (const Entry& entry : page.entries) {
            if (entry.discovered) {
                ++page.discoveredCount;
            }
        }
    };
    auto finalizeCatalogPage = [&](CategoryPage& page, int totalCount,
                                   const QVector<QString>& excludedIds = {}) {
        auto isExcluded = [&](const QString& id) {
            for (const QString& excludedId : excludedIds) {
                if (id == excludedId) return true;
            }
            return false;
        };

        QVector<Entry> catalogEntries;
        catalogEntries.reserve(page.entries.size());
        int discoveredCount = 0;
        for (const Entry& entry : page.entries) {
            const bool placeholderOnly =
                entry.stats.isEmpty() && entry.iconPath.isEmpty() && entry.detailImagePath.isEmpty();
            if (placeholderOnly || isExcluded(entry.id)) {
                continue;
            }
            catalogEntries.push_back(entry);
            if (entry.discovered) {
                ++discoveredCount;
            }
        }
        page.entries = catalogEntries;
        page.totalCount = totalCount;
        page.discoveredCount = qBound(0, discoveredCount, totalCount);
    };

    CategoryPage fish;
    fish.category = Category::Fish;
    fish.title = QStringLiteral("鱼类");
    fish.icon = QStringLiteral("◆");
    fish.discoveredCount = 18;
    fish.totalCount = 62;
    fish.entries = {
        {QStringLiteral("001"), QStringLiteral("沙丁鱼"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_sardine.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_sardine.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("沙丁鱼")),
           stat(QStringLiteral("类型"), QStringLiteral("小型鱼类")),
           stat(QStringLiteral("价值"), QStringLiteral("5 - 15")),
           stat(QStringLiteral("稀有度"), QStringLiteral("★☆☆☆☆")),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 1 关起")),
           stat(QStringLiteral("捕获方式"), QStringLiteral("鱼竿 / 渔网")),
           stat(QStringLiteral("捕获难度"), QStringLiteral("3 次 / 3 秒"))},
         QStringLiteral("常见的小型洄游鱼类，成群活动于近海水域。肉质鲜美，容易上钩，是水手们最熟悉的伙伴。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("002"), QStringLiteral("金枪鱼"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_tuna.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_tuna.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("金枪鱼")),
           stat(QStringLiteral("类型"), QStringLiteral("中型鱼类")),
           stat(QStringLiteral("价值"), QStringLiteral("25 - 55")),
           stat(QStringLiteral("稀有度"), QStringLiteral("★★☆☆☆")),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 1 关起")),
           stat(QStringLiteral("捕获方式"), QStringLiteral("鱼竿 / 渔网")),
           stat(QStringLiteral("捕获难度"), QStringLiteral("3 次 / 3 秒"))},
         QStringLiteral("速度稳定、价值不错的远海鱼类。掌握基础捕鱼节奏后，它会成为重要收入来源。"),
         true, QColor("#2f7a45")},
        unknown(QStringLiteral("003")),
        {QStringLiteral("004"), QStringLiteral("黄金鱼"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_golden.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_golden.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("黄金鱼")),
          stat(QStringLiteral("类型"), QStringLiteral("稀有鱼类")),
           stat(QStringLiteral("价值"), QStringLiteral("150 - 250")),
           stat(QStringLiteral("稀有度"), QStringLiteral("★★★★★")),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 3 关起")),
           stat(QStringLiteral("捕获方式"), QStringLiteral("高耐久钓具")),
           stat(QStringLiteral("捕获难度"), QStringLiteral("10 次 / 1.25 秒"))},
         QStringLiteral("传说会在阳光穿透浪面时出现。价值极高，但警觉性强，稍慢一步就会逃离。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("005"), QStringLiteral("深海鳗"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_eel.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_eel.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("深海鳗")),
          stat(QStringLiteral("类型"), QStringLiteral("稀有鱼类")),
           stat(QStringLiteral("价值"), QStringLiteral("80 - 140")),
           stat(QStringLiteral("稀有度"), QStringLiteral("★★★★☆")),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 2 关起")),
           stat(QStringLiteral("捕获方式"), QStringLiteral("鱼叉 / 渔网")),
           stat(QStringLiteral("捕获难度"), QStringLiteral("8 次 / 2 秒"))},
         QStringLiteral("栖息于暗流之下，动作突然且难以预判。捕获它需要更短的反应时间。"),
         true, QColor("#2f7a45")},
        unknown(QStringLiteral("006")),
        {QStringLiteral("007"), QStringLiteral("\u94f6\u9cca\u9c7c"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_anchovy.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_anchovy.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u94f6\u9cca\u9c7c")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("8 - 18")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 1 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("3 \u6b21 / 3.2 \u79d2"))},
         QStringLiteral("\u8fd1\u6d77\u5c0f\u578b\u9c7c\uff0c\u6e38\u52a8\u8f7b\u5feb\uff0c\u9002\u5408\u65b0\u624b\u5728\u7b2c\u4e00\u5173\u7a33\u5b9a\u83b7\u53d6\u6536\u5165\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("008"), QStringLiteral("\u5c0f\u4e11\u9c7c"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_clownfish.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_clownfish.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u5c0f\u4e11\u9c7c")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("12 - 24")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 2 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("4 \u6b21 / 3 \u79d2"))},
         QStringLiteral("\u8272\u5f69\u9192\u76ee\u7684\u73ca\u745a\u533a\u9c7c\u7c7b\uff0c\u4ef7\u503c\u7565\u9ad8\uff0c\u4f46\u8b66\u89c9\u6027\u4ecd\u53ef\u63a7\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("009"), QStringLiteral("\u84dd\u9cb5"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_mackerel.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_mackerel.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u84dd\u9cb5")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("35 - 65")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 1 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("4 \u6b21 / 2.75 \u79d2"))},
         QStringLiteral("\u5916\u6d77\u5e38\u89c1\u7684\u4e2d\u4ef7\u9c7c\uff0c\u901f\u5ea6\u548c\u6536\u76ca\u90fd\u6bd4\u91d1\u67aa\u9c7c\u66f4\u7a33\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("010"), QStringLiteral("\u771f\u9cb7"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_sea_bream.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_sea_bream.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u771f\u9cb7")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("45 - 80")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 1 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("5 \u6b21 / 2.6 \u79d2"))},
         QStringLiteral("\u8089\u8d28\u548c\u4ef7\u503c\u90fd\u4e0d\u9519\u7684\u4e2d\u578b\u9c7c\uff0c\u66f4\u9002\u5408\u6709\u4e00\u5b9a\u6355\u9c7c\u8282\u594f\u540e\u8ffd\u6355\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("011"), QStringLiteral("\u706f\u7b3c\u9c7c"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_lanternfish.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_lanternfish.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u706f\u7b3c\u9c7c")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("95 - 160")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 2 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("7 \u6b21 / 1.75 \u79d2"))},
         QStringLiteral("\u6697\u6d41\u6d77\u57df\u4e2d\u4f1a\u95ea\u5149\u7684\u7a00\u6709\u9c7c\uff0c\u6536\u76ca\u9ad8\uff0c\u4f46\u6355\u83b7\u7a97\u53e3\u660e\u663e\u66f4\u7d27\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("012"), QStringLiteral("\u77f3\u6591\u9c7c"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_grouper.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_grouper.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u77f3\u6591\u9c7c")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("110 - 190")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 2 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("7 \u6b21 / 2 \u79d2"))},
         QStringLiteral("\u559c\u6b22\u9760\u8fd1\u7901\u77f3\u7684\u539a\u91cd\u9c7c\u7c7b\uff0c\u5355\u6761\u6536\u76ca\u5f88\u53ef\u89c2\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("013"), QStringLiteral("\u9526\u9ca4"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_koi.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_koi.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u9526\u9ca4")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("180 - 280")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 3 \u5173\u8d77")),
          stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("9 \u6b21 / 1.5 \u79d2"))},
         QStringLiteral("\u7f55\u89c1\u7684\u9ad8\u4ef7\u9c7c\uff0c\u4f1a\u5728\u5929\u6c14\u548c\u6d6a\u52bf\u4e0d\u7a33\u7684\u6d77\u57df\u51fa\u73b0\u3002"),
         true, QColor("#2f7a45")},
        {QStringLiteral("014"), QStringLiteral("\u6676\u9cde\u9c7c"), QStringLiteral("\u5df2\u53d1\u73b0"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_crystal_fish.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/fish_crystal_fish.png"),
         {stat(QStringLiteral("\u540d\u79f0"), QStringLiteral("\u6676\u9cde\u9c7c")),
          stat(QStringLiteral("\u4ef7\u503c"), QStringLiteral("240 - 380")),
           stat(QStringLiteral("\u51fa\u73b0\u5173\u5361"), QStringLiteral("\u7b2c 3 \u5173\u8d77")),
           stat(QStringLiteral("\u6355\u83b7\u96be\u5ea6"), QStringLiteral("11 \u6b21 / 1.37 \u79d2"))},
          QStringLiteral("\u7ec8\u6bb5\u6d77\u57df\u4e2d\u7684\u73cd\u7a00\u9c7c\uff0c\u4ef7\u503c\u6781\u9ad8\uff0c\u9700\u8981\u66f4\u597d\u7684\u88c5\u5907\u548c\u4f53\u529b\u7ba1\u7406\u3002"),
         true, QColor("#2f7a45")}
    };
    setEntryDiscoveryAt(fish, 0, discoveryLog.isFishDiscovered(0));
    setEntryDiscoveryAt(fish, 1, discoveryLog.isFishDiscovered(1));
    setEntryDiscoveryAt(fish, 3, discoveryLog.isFishDiscovered(3));
    setEntryDiscoveryAt(fish, 4, discoveryLog.isFishDiscovered(2));
    setEntryDiscoveryAt(fish, 6, discoveryLog.isFishDiscovered(4));
    setEntryDiscoveryAt(fish, 7, discoveryLog.isFishDiscovered(5));
    setEntryDiscoveryAt(fish, 8, discoveryLog.isFishDiscovered(6));
    setEntryDiscoveryAt(fish, 9, discoveryLog.isFishDiscovered(7));
    setEntryDiscoveryAt(fish, 10, discoveryLog.isFishDiscovered(8));
    setEntryDiscoveryAt(fish, 11, discoveryLog.isFishDiscovered(9));
    setEntryDiscoveryAt(fish, 12, discoveryLog.isFishDiscovered(10));
    setEntryDiscoveryAt(fish, 13, discoveryLog.isFishDiscovered(11));
    finalizeCatalogPage(fish, kFishCatalogTotal);

    CategoryPage equipment;
    equipment.category = Category::Equipment;
    equipment.title = QStringLiteral("装备");
    equipment.icon = QStringLiteral("⚔");
    equipment.discoveredCount = 12;
    equipment.totalCount = 25;
    equipment.entries = {
        {QStringLiteral("001"), QStringLiteral("鱼竿"), QStringLiteral("工具"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_rod.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_rod.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("鱼竿")),
          stat(QStringLiteral("类型"), QStringLiteral("工具")),
           stat(QStringLiteral("用途"), QStringLiteral("钓鱼")),
           stat(QStringLiteral("钓鱼方式"), QStringLiteral("QTE")),
           stat(QStringLiteral("攻击伤害"), QStringLiteral("—")),
            stat(QStringLiteral("捕捞范围"), configValue(Config::RANGE_ROD)),
            stat(QStringLiteral("最大耐久"), configValue(Config::DUR_ROD_T1))},
         QStringLiteral("最基础的钓鱼工具，操作简单，适合初学者使用。虽然朴素，但能陪伴你度过漫长的航海时光。"),
         true, QColor("#2f6f9f")},
        {QStringLiteral("002"), QStringLiteral("渔网"), QStringLiteral("工具"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_net.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_net.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("渔网")),
          stat(QStringLiteral("类型"), QStringLiteral("工具")),
           stat(QStringLiteral("用途"), QStringLiteral("范围捕鱼")),
           stat(QStringLiteral("钓鱼方式"), QStringLiteral("校准")),
           stat(QStringLiteral("攻击伤害"), QStringLiteral("—")),
            stat(QStringLiteral("捕捞范围"), configValue(Config::RANGE_NET)),
            stat(QStringLiteral("最大耐久"), configValue(Config::DUR_NET_T1))},
         QStringLiteral("适合稳定收获小型鱼群，范围更宽，但耐久消耗更明显。"),
         true, QColor("#2f6f9f")},
        {QStringLiteral("003"), QStringLiteral("鱼叉"), QStringLiteral("武器"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_harpoon.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_harpoon.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("鱼叉")),
          stat(QStringLiteral("类型"), QStringLiteral("双用装备")),
           stat(QStringLiteral("用途"), QStringLiteral("捕鱼 / 攻击")),
           stat(QStringLiteral("钓鱼方式"), QStringLiteral("校准")),
            stat(QStringLiteral("攻击伤害"), configValue(Config::DMG_HARPOON_T1)),
            stat(QStringLiteral("作用范围"), configValue(Config::RANGE_HARPOON)),
            stat(QStringLiteral("最大耐久"), configValue(Config::DUR_HARPOON_T1))},
         QStringLiteral("近距离捕鱼与自卫兼备。遇到海中威胁时，它比普通钓具更可靠。"),
         true, QColor("#8a3e2e")},
        {QStringLiteral("004"), QStringLiteral("手枪"), QStringLiteral("武器"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_pistol.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_pistol.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("手枪")),
          stat(QStringLiteral("类型"), QStringLiteral("远程武器")),
           stat(QStringLiteral("用途"), QStringLiteral("攻击")),
           stat(QStringLiteral("钓鱼方式"), QStringLiteral("—")),
            stat(QStringLiteral("攻击伤害"), configValue(Config::DMG_PISTOL_T1)),
            stat(QStringLiteral("射程"), configValue(Config::RANGE_PISTOL)),
            stat(QStringLiteral("最大耐久"), configValue(Config::DUR_PISTOL_T1))},
         QStringLiteral("远距离自卫武器，射程优秀，适合在敌人接近前削弱威胁。"),
         true, QColor("#8a3e2e")},
        {QStringLiteral("005"), QStringLiteral("猎枪"), QStringLiteral("武器"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_shotgun.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/weapon_shotgun.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("猎枪")),
          stat(QStringLiteral("类型"), QStringLiteral("近距武器")),
           stat(QStringLiteral("用途"), QStringLiteral("高伤害攻击")),
           stat(QStringLiteral("钓鱼方式"), QStringLiteral("—")),
            stat(QStringLiteral("攻击伤害"), configValue(Config::DMG_SHOTGUN_T1)),
            stat(QStringLiteral("射程"), configValue(Config::RANGE_SHOTGUN)),
            stat(QStringLiteral("最大耐久"), configValue(Config::DUR_SHOTGUN_T1))},
         QStringLiteral("爆发力强但耐久有限。适合在危险距离内快速解决敌人。"),
         true, QColor("#8a3e2e")}
    };
    setEntryDiscoveryAt(equipment, 0, discoveryLog.isEquipmentDiscovered(0));
    setEntryDiscoveryAt(equipment, 1, discoveryLog.isEquipmentDiscovered(1));
    setEntryDiscoveryAt(equipment, 2, discoveryLog.isEquipmentDiscovered(2));
    setEntryDiscoveryAt(equipment, 3, discoveryLog.isEquipmentDiscovered(3));
    setEntryDiscoveryAt(equipment, 4, discoveryLog.isEquipmentDiscovered(4));
    finalizeCatalogPage(equipment, kEquipmentCatalogTotal);

    CategoryPage item;
    item.category = Category::Item;
    item.title = QStringLiteral("道具");
    item.icon = QStringLiteral("●");
    item.discoveredCount = 34;
    item.totalCount = 78;
    item.entries = {
        {QStringLiteral("001"), QStringLiteral("航海干粮"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_food.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_food.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("航海干粮")),
          stat(QStringLiteral("类型"), QStringLiteral("消耗品")),
          stat(QStringLiteral("目标"), QStringLiteral("使用者")),
           stat(QStringLiteral("效果"), QStringLiteral("恢复 %1 点体力").arg(Config::HEAL_FOOD_RATION)),
          stat(QStringLiteral("是否可叠加"), QStringLiteral("是（最多 99）")),
          stat(QStringLiteral("使用场景"), QStringLiteral("航行 / 探索 / 战斗"))},
         QStringLiteral("经过烘烤的硬质饼干，便于长期保存。虽然味道平淡，但能有效补充航海所需的体力。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("002"), QStringLiteral("初级船体修理包"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_repair_t1.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_repair_t1.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("初级船体修理包")),
          stat(QStringLiteral("类型"), QStringLiteral("消耗品")),
          stat(QStringLiteral("目标"), QStringLiteral("船体")),
           stat(QStringLiteral("效果"), QStringLiteral("恢复 %1 点耐久").arg(Config::HEAL_REPAIR_T1)),
          stat(QStringLiteral("是否可叠加"), QStringLiteral("是")),
          stat(QStringLiteral("使用场景"), QStringLiteral("航行 / 战斗"))},
         QStringLiteral("基础船板与补漏材料，适合处理轻微撞击和浅层破损。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("003"), QStringLiteral("中级船体修理包"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_repair_t2.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_repair_t2.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("中级船体修理包")),
          stat(QStringLiteral("类型"), QStringLiteral("消耗品")),
          stat(QStringLiteral("目标"), QStringLiteral("船体")),
           stat(QStringLiteral("效果"), QStringLiteral("恢复 %1 点耐久").arg(Config::HEAL_REPAIR_T2)),
          stat(QStringLiteral("是否可叠加"), QStringLiteral("是")),
          stat(QStringLiteral("使用场景"), QStringLiteral("航行 / 战斗"))},
         QStringLiteral("更结实的船体材料，能修复较严重损伤，是中后段航程的常备补给。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("004"), QStringLiteral("高级船体修理包"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_repair_t3.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_repair_t3.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("高级船体修理包")),
          stat(QStringLiteral("类型"), QStringLiteral("消耗品")),
          stat(QStringLiteral("目标"), QStringLiteral("船体")),
           stat(QStringLiteral("效果"), QStringLiteral("恢复 %1 点耐久").arg(Config::HEAL_REPAIR_T3)),
          stat(QStringLiteral("是否可叠加"), QStringLiteral("是")),
          stat(QStringLiteral("使用场景"), QStringLiteral("深海决战"))},
         QStringLiteral("专业级修理材料，关键时刻能把几乎散架的船重新拉回航线。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("005"), QStringLiteral("紧急装备修理工具"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_emergency_repair.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/item_emergency_repair.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("紧急装备修理工具")),
          stat(QStringLiteral("类型"), QStringLiteral("消耗品")),
          stat(QStringLiteral("目标"), QStringLiteral("当前装备")),
           stat(QStringLiteral("效果"), QStringLiteral("恢复 %1% 最大耐久").arg(Config::EMERGENCY_WEAPON_REPAIR_PERCENT)),
          stat(QStringLiteral("是否可叠加"), QStringLiteral("是")),
          stat(QStringLiteral("使用场景"), QStringLiteral("战斗中救急"))},
         QStringLiteral("小型工具组，可在航行中快速修复当前装备，适合应对连续战斗。"),
         true, QColor("#2f7a45")},
        unknown(QStringLiteral("006"))
    };
    setEntryDiscoveryAt(item, 0, discoveryLog.isItemDiscovered(0));
    setEntryDiscoveryAt(item, 1, discoveryLog.isItemDiscovered(1));
    setEntryDiscoveryAt(item, 2, discoveryLog.isItemDiscovered(2));
    setEntryDiscoveryAt(item, 3, discoveryLog.isItemDiscovered(3));
    setEntryDiscoveryAt(item, 4, discoveryLog.isItemDiscovered(4));
    finalizeCatalogPage(item, kItemCatalogTotal);

    CategoryPage enemy;
    enemy.category = Category::Enemy;
    enemy.title = QStringLiteral("敌人");
    enemy.icon = QStringLiteral("☠");
    enemy.discoveredCount = 21;
    enemy.totalCount = 58;
    enemy.entries = {
        {QStringLiteral("001"), QStringLiteral("鲨鱼"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_shark.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_shark.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("鲨鱼")),
           stat(QStringLiteral("HP"), scaledEnemyValue(100, kEnemyHpGrowth)),
           stat(QStringLiteral("攻击"), scaledEnemyValue(10, kEnemyAttackGrowth)),
           stat(QStringLiteral("速度"), scaledEnemySpeed(2.0)),
           stat(QStringLiteral("当前关卡"), QStringLiteral("第 %1 关").arg(enemyStage)),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 1 关起")),
           stat(QStringLiteral("攻击方式"), QStringLiteral("追踪撕咬")),
           stat(QStringLiteral("击败收益"), scaledEnemyValue(30, kEnemyRewardGrowth))},
          QStringLiteral("近海中最常见的掠食者，会持续追踪船只，咬合后短暂后撤再寻找下一次机会。"),
          true, QColor("#2f7a45")},
        {QStringLiteral("002"), QStringLiteral("剑鱼"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_swordfish.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_swordfish.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("剑鱼")),
           stat(QStringLiteral("HP"), scaledEnemyValue(80, kEnemyHpGrowth)),
           stat(QStringLiteral("攻击"), scaledEnemyValue(25, kEnemyAttackGrowth)),
           stat(QStringLiteral("速度"), QStringLiteral("%1 / 冲刺 8.0")
               .arg(scaledEnemySpeed(1.5))),
          stat(QStringLiteral("当前关卡"), QStringLiteral("第 %1 关").arg(enemyStage)),
          stat(QStringLiteral("出现关卡"), QStringLiteral("外海航道")),
          stat(QStringLiteral("攻击方式"), QStringLiteral("蓄力冲刺")),
           stat(QStringLiteral("击败收益"), scaledEnemyValue(50, kEnemyRewardGrowth))},
         QStringLiteral("平时巡游，发现船只后会短暂蓄力并高速冲刺。提前观察它的朝向是关键。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("003"), QStringLiteral("墨鱼"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_octopus.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_octopus.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("墨鱼")),
           stat(QStringLiteral("HP"), scaledEnemyValue(60, kEnemyHpGrowth)),
           stat(QStringLiteral("攻击"), QStringLiteral("0 / 喷墨干扰")),
           stat(QStringLiteral("速度"), scaledEnemySpeed(1.2)),
           stat(QStringLiteral("当前关卡"), QStringLiteral("第 %1 关").arg(enemyStage)),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 2 关起")),
           stat(QStringLiteral("攻击方式"), QStringLiteral("隐身 / 喷墨遮挡")),
           stat(QStringLiteral("击败收益"), scaledEnemyValue(40, kEnemyRewardGrowth))},
          QStringLiteral("会周期性隐身并靠近船只。喷墨弹命中后会在屏幕四周和中央留下墨迹，并短暂拖慢航速。"),
          true, QColor("#2f7a45")},
        {QStringLiteral("004"), QStringLiteral("电鳐"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_electric_ray.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_electric_ray.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("电鳐")),
           stat(QStringLiteral("HP"), scaledEnemyValue(120, kEnemyHpGrowth)),
           stat(QStringLiteral("攻击"), scaledEnemyValue(12, kEnemyAttackGrowth)),
           stat(QStringLiteral("速度"), scaledEnemySpeed(1.35)),
          stat(QStringLiteral("当前关卡"), QStringLiteral("第 %1 关").arg(enemyStage)),
          stat(QStringLiteral("出现关卡"), QStringLiteral("第 2 关起")),
          stat(QStringLiteral("攻击方式"), QStringLiteral("蓄电范围脉冲")),
          stat(QStringLiteral("附加效果"), QStringLiteral("短暂眩晕")),
           stat(QStringLiteral("击败收益"), scaledEnemyValue(65, kEnemyRewardGrowth))},
         QStringLiteral("靠近船只后会停下蓄电，电光环与实际脉冲范围一致。离开光环或利用蓄力时间穿过去。"),
         true, QColor("#2f7a45")},
        {QStringLiteral("005"), QStringLiteral("毒刺水母"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_poison_jellyfish.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/enemy_poison_jellyfish.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("毒刺水母")),
           stat(QStringLiteral("HP"), scaledEnemyValue(75, kEnemyHpGrowth)),
           stat(QStringLiteral("攻击"), scaledEnemyValue(5, kEnemyAttackGrowth)),
           stat(QStringLiteral("速度"), scaledEnemySpeed(1.0)),
          stat(QStringLiteral("当前关卡"), QStringLiteral("第 %1 关").arg(enemyStage)),
          stat(QStringLiteral("出现关卡"), QStringLiteral("第 3 关起")),
          stat(QStringLiteral("攻击方式"), QStringLiteral("蓄力触须突刺")),
          stat(QStringLiteral("附加效果"), QStringLiteral("持续中毒")),
           stat(QStringLiteral("击败收益"), scaledEnemyValue(55, kEnemyRewardGrowth))},
         QStringLiteral("靠近船只后会停下蓄力，随后沿当前朝向伸出有毒触须。攻击有明显前摇，命中后会中毒，水母也会立即后撤。"),
         true, QColor("#2f7a45")},
        unknown(QStringLiteral("006"))
    };
    setEntryDiscoveryAt(enemy, 0, discoveryLog.isEnemyDiscovered(0));
    setEntryDiscoveryAt(enemy, 1, discoveryLog.isEnemyDiscovered(1));
    setEntryDiscoveryAt(enemy, 2, discoveryLog.isEnemyDiscovered(2));
    setEntryDiscoveryAt(enemy, 3, discoveryLog.isEnemyDiscovered(3));
    setEntryDiscoveryAt(enemy, 4, discoveryLog.isEnemyDiscovered(4));
    finalizeCatalogPage(enemy, kEnemyCatalogTotal);

    CategoryPage boss;
    boss.category = Category::Boss;
    boss.title = QStringLiteral("Boss");
    boss.icon = QStringLiteral("♛");
    boss.discoveredCount = 18;
    boss.totalCount = 62;
    boss.entries = {
        {QStringLiteral("B01"), QStringLiteral("夺命五头鲨"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/boss_five_head_shark.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/boss_five_head_shark.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("夺命五头鲨")),
           stat(QStringLiteral("HP（初始）"), QStringLiteral("2,900")),
           stat(QStringLiteral("阶段数"), QStringLiteral("2 阶段")),
           stat(QStringLiteral("出现关卡"), QStringLiteral("第 4 关 Boss")),
           stat(QStringLiteral("危险等级"), QStringLiteral("极高")),
           stat(QStringLiteral("击败收益"), QStringLiteral("800"))},
          QStringLiteral("深海中孕育的变异巨鲨，五个头颅各自拥有独立意识。海员传说中，它的出现常伴随碎浪与鲨影。"),
          true, QColor("#9d3737")},
        {QStringLiteral("B02"), QStringLiteral("塔利海怪"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/boss_tali_monster.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/boss_tali_monster.png"),
         {stat(QStringLiteral("名称"), QStringLiteral("塔利海怪")),
           stat(QStringLiteral("HP（初始）"), QStringLiteral("2,250")),
           stat(QStringLiteral("阶段数"), QStringLiteral("2 阶段")),
           stat(QStringLiteral("出现关卡"), QStringLiteral("档案预留")),
           stat(QStringLiteral("危险等级"), QStringLiteral("未知")),
           stat(QStringLiteral("击败收益"), QStringLiteral("800"))},
          QStringLiteral("旧航海记录中反复出现的深海怪物，外壳覆盖珊瑚与沉船残片。当前航线暂未开放遭遇。"),
          true, QColor("#9d3737")},
        {QStringLiteral("B03"), QStringLiteral("塞壬女妖"), QStringLiteral("已发现"),
         QStringLiteral(":/FishingVoyage/encyclopedia/boss_siren.png"),
         QStringLiteral(":/FishingVoyage/encyclopedia/boss_siren.png"),
           {stat(QStringLiteral("名称"), QStringLiteral("塞壬女妖")),
            stat(QStringLiteral("HP（初始）"), QStringLiteral("4,200")),
            stat(QStringLiteral("阶段数"), QStringLiteral("2 阶段")),
            stat(QStringLiteral("出现关卡"), QStringLiteral("第 9 关最终 Boss")),
            stat(QStringLiteral("危险等级"), QStringLiteral("终局")),
            stat(QStringLiteral("击败收益"), QStringLiteral("1,500"))},
           QStringLiteral("月色下吟唱的深海女王。她的歌声会让航线和时间都变得模糊，许多船只只留下被潮水磨平的桅杆。"),
           true, QColor("#9d3737")},
        unknown(QStringLiteral("B04"), QStringLiteral("未解锁")),
        unknown(QStringLiteral("B05"), QStringLiteral("未解锁")),
        unknown(QStringLiteral("B06"), QStringLiteral("未解锁"))
    };
    const bool fiveHeadEncountered =
        discoveryLog.isBossDiscovered(0) &&
        (m_currentStage > 4 || (m_currentStage == 4 && m_currentStageBossEncountered));
    const bool sirenEncountered =
        discoveryLog.isBossDiscovered(2) &&
        (m_currentStage > 9 || (m_currentStage == 9 && m_currentStageBossEncountered));
    setEntryDiscoveryAt(boss, 0, fiveHeadEncountered);
    setEntryDiscoveryAt(boss, 1, false);
    setEntryDiscoveryAt(boss, 2, sirenEncountered);
    finalizeCatalogPage(boss, kBossCatalogTotal, { QStringLiteral("B02") });

    m_pages = { fish, equipment, item, enemy, boss };
}

void EncyclopediaDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    if (!m_fallbackSea.isNull()) {
        p.drawPixmap(rect(), m_fallbackSea);
    } else {
        p.fillRect(rect(), QColor(18, 83, 120));
    }
    p.fillRect(rect(), QColor(0, 12, 24, 170));

    if (m_openProgress < 1.0) {
        drawOpeningAnimation(p);
        return;
    }

    drawBook(p);
}

void EncyclopediaDialog::drawOpeningAnimation(QPainter& p)
{
    const qreal t = clamp01(m_openProgress);
    const qreal spreadT = easeOutCubic(segmentProgress(t, 0.0, 0.18));
    const qreal flipRaw = segmentProgress(t, 0.10, 0.74);
    const qreal flipT = easeInOut(flipRaw);
    const qreal flipOpacity = 1.0 - easeInCubic(segmentProgress(t, 0.82, 0.94));
    const qreal contentAlpha = easeOutCubic(segmentProgress(t, 0.62, 0.86));

    p.save();

    const int spineX = width() / 2;
    if (!m_bookFrame.isNull()) {
        drawOpeningBookFrame(p, m_bookFrame, spreadT);
    } else {
        const int pageTop = 74;
        const int pageBottom = 650;
        const int pageWidth = 548;
        QPolygonF leftPage;
        leftPage << QPointF(spineX, pageTop)
                 << QPointF(spineX - pageWidth, pageTop)
                 << QPointF(spineX - pageWidth, pageBottom)
                 << QPointF(spineX, pageBottom);
        QPolygonF rightPage;
        rightPage << QPointF(spineX, pageTop)
                  << QPointF(spineX + pageWidth, pageTop)
                  << QPointF(spineX + pageWidth, pageBottom)
                  << QPointF(spineX, pageBottom);
        drawTexturedPage(p, m_uiOpenPageLeft, leftPage);
        drawTexturedPage(p, m_uiOpenPageRight, rightPage);
    }

    if (spreadT > 0.96 && flipOpacity > 0.01) {
        drawPageTurnGrounding(p, spineX, 56, 620, 482, flipT);
    }

    if (contentAlpha > 0.01) {
        p.save();
        p.setOpacity(contentAlpha);
        drawBookContents(p);
        p.restore();
    }

    if (spreadT > 0.72 && flipOpacity > 0.01) {
        p.save();
        p.setOpacity(flipOpacity);
        drawCurledPage(p, m_uiOpenFlipPage, spineX, 56, 620, 482, flipT);
        p.restore();
    }

    p.restore();
}

void EncyclopediaDialog::drawBook(QPainter& p)
{
    if (!m_bookFrame.isNull()) {
        p.drawPixmap(kFrameRect, m_bookFrame);
    } else {
        drawDecoratedPanel(p, kFrameRect, QColor(88, 48, 23), QColor(220, 163, 60));
        drawDecoratedPanel(p, kLeftPageRect, QColor(230, 204, 151), QColor(126, 80, 32));
        drawDecoratedPanel(p, kRightPageRect, QColor(232, 207, 157), QColor(126, 80, 32));
    }

    drawBookContents(p);
}

void EncyclopediaDialog::drawBookContents(QPainter& p)
{
    drawNauticalGrid(p, kLeftPageRect.adjusted(24, 26, -28, -24), QColor(112, 78, 36, 32));
    drawNauticalGrid(p, kRightPageRect.adjusted(24, 26, -28, -24), QColor(112, 78, 36, 32));
    drawCompassRose(p, QPoint(kLeftPageRect.right() - 46, kLeftPageRect.bottom() - 44),
                    18, QColor(109, 73, 31, 72));
    drawCompassRose(p, QPoint(kRightPageRect.left() + 46, kRightPageRect.bottom() - 44),
                    18, QColor(109, 73, 31, 72));

    drawTitle(p);
    drawTabs(p);
    drawListPage(p);
    drawDetailPage(p);

    drawTextShadow(p, QRect(0, 674, width(), 28),
                   QStringLiteral("鼠标切换分类 / 滚轮翻阅条目 / Esc 返回"),
                   uiFont(10, QFont::Bold), QColor(221, 188, 128));
}

void EncyclopediaDialog::drawTitle(QPainter& p)
{
    drawTextShadow(p, QRect(0, 26, width(), 54), QStringLiteral("航海图鉴"),
                   titleFont(32, QFont::Bold), QColor(255, 229, 151));
    drawTextShadow(p, QRect(0, 76, width(), 22), QStringLiteral("VOYAGE ENCYCLOPEDIA"),
                   uiFont(9, QFont::Bold), QColor(220, 168, 79));
}

void EncyclopediaDialog::drawTabs(QPainter& p)
{
    for (int i = 0; i < m_pages.size(); ++i) {
        const QRect r = tabRect(i);
        const bool selected = (i == m_categoryIndex);
        const bool hovered = (i == m_hoverTab);

        const QPixmap& tabImage = selected ? m_uiTabSelected
                                 : hovered ? m_uiTabHover
                                           : m_uiTabNormal;
        if (!tabImage.isNull()) {
            p.save();
            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            p.drawPixmap(r, tabImage);
            p.restore();
        } else {
            QColor fill = selected ? QColor(28, 64, 92, 235)
                        : hovered ? QColor(222, 192, 139, 235)
                                  : QColor(203, 174, 126, 220);
            QColor border = selected ? QColor(232, 179, 65) : QColor(133, 84, 35);
            drawPixelPanel(p, r, fill, border, true);
        }

        const QColor textColor = selected ? QColor(255, 240, 188) : QColor(71, 42, 17);
        drawTextShadow(p, QRect(r.left() + 14, r.top() + 6, 30, r.height() - 12),
                       m_pages[i].icon, uiFont(15, QFont::Bold), textColor);
        drawTextShadow(p, QRect(r.left() + 44, r.top() + 4, r.width() - 52, r.height() - 8),
                       m_pages[i].title, titleFont(16, QFont::Bold), textColor,
                       Qt::AlignLeft | Qt::AlignVCenter);
    }
}

void EncyclopediaDialog::drawListPage(QPainter& p)
{
    const auto& page = currentPage();
    QRect header(kLeftPageRect.left() + 44, 162, kLeftPageRect.width() - 92, 36);
    drawCompassRose(p, QPoint(header.left() - 18, header.center().y()),
                    12, QColor(89, 58, 26, 95));
    drawTextShadow(p, header,
                   QStringLiteral("图鉴进度： %1 / %2").arg(page.discoveredCount).arg(page.totalCount),
                   uiFont(16, QFont::Bold), QColor(74, 42, 16),
                   Qt::AlignLeft | Qt::AlignVCenter);
    drawInkRule(p, header.left(), header.bottom() - 2, header.right(), QColor(102, 67, 30), 132);

    QRect listArea(kLeftPageRect.left() + 28, kRowTop - 8, kLeftPageRect.width() - 58, 424);
    drawNauticalGrid(p, listArea.adjusted(14, 18, -18, -20), QColor(101, 69, 31, 22));
    p.save();
    p.setPen(QPen(QColor(120, 82, 37, 55), 1, Qt::DashLine));
    p.drawLine(listArea.left() + 24, listArea.top(), listArea.left() + 24, listArea.bottom());
    p.restore();

    const int start = currentScroll();
    for (int i = 0; i < visibleRowCount(); ++i) {
        const int entryIndex = start + i;
        if (entryIndex >= page.entries.size()) break;
        drawEntryRow(p, rowRect(i), page.entries[entryIndex], entryIndex);
    }

    const int entryCount = static_cast<int>(page.entries.size());
    const int maxScroll = std::max(0, entryCount - visibleRowCount());
    QRect rail(kLeftPageRect.right() - 22, kRowTop - 4, 8, 396);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(100, 66, 28, 100), 1, Qt::DashLine));
    p.drawLine(rail.center().x(), rail.top(), rail.center().x(), rail.bottom());
    if (maxScroll > 0) {
        const int thumbH = std::max(58, rail.height() * visibleRowCount() / entryCount);
        const int track = rail.height() - thumbH;
        const int thumbY = rail.top() + currentScroll() * track / maxScroll;
        QRect thumb(rail.left() - 3, thumbY, 14, thumbH);
        p.fillRect(thumb, QColor(128, 81, 28, 110));
        p.fillRect(thumb.adjusted(3, 4, -3, -4), QColor(71, 44, 18, 120));
    }
    p.restore();
}

void EncyclopediaDialog::drawDetailPage(QPainter& p)
{
    const auto& page = currentPage();
    if (page.entries.isEmpty()) return;

    const Entry& entry = page.entries[selectedIndex()];
    QRect detailHeader(kRightPageRect.left() + 58, 164, kRightPageRect.width() - 112, 42);
    drawTextShadow(p, detailHeader, entry.discovered ? entry.name : QStringLiteral("未发现记录"),
                   titleFont(22, QFont::Bold), QColor(43, 24, 8),
                   Qt::AlignCenter);
    drawInkRule(p, detailHeader.left() + 20, detailHeader.bottom() - 2,
                detailHeader.right() - 20, QColor(98, 61, 24), 132);

    QRect imageArea = kDetailImageRect.adjusted(-2, 2, 2, 6);
    drawNauticalGrid(p, imageArea.adjusted(14, 12, -14, -12), QColor(70, 104, 111, 28));
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(90, 58, 24, 110), 2));
    p.drawLine(imageArea.left() + 12, imageArea.top() + 8, imageArea.left() + 44, imageArea.top() + 8);
    p.drawLine(imageArea.left() + 12, imageArea.top() + 8, imageArea.left() + 12, imageArea.top() + 34);
    p.drawLine(imageArea.right() - 12, imageArea.bottom() - 8, imageArea.right() - 44, imageArea.bottom() - 8);
    p.drawLine(imageArea.right() - 12, imageArea.bottom() - 8, imageArea.right() - 12, imageArea.bottom() - 34);
    p.restore();
    QPixmap image = pixmapFromPath(entry.discovered ? (entry.detailImagePath.isEmpty() ? entry.iconPath : entry.detailImagePath) : QString());
    if (!image.isNull()) {
        p.save();
        p.setOpacity(0.22);
        drawPixmapFit(p, image, imageArea.adjusted(18, 18, -18, -12).translated(3, 5));
        p.setOpacity(1.0);
        drawPixmapFit(p, image, imageArea.adjusted(18, 18, -18, -12));
        p.restore();
    } else {
        drawUnknownMark(p, imageArea.adjusted(32, 24, -32, -24), !entry.discovered);
    }

    drawInkRule(p, kStatsRect.left(), kStatsRect.top() - 8, kStatsRect.right(), QColor(101, 66, 31), 105);
    if (entry.discovered) {
        drawStatLines(p, kStatsRect, entry);
    } else {
        drawTextShadow(p, kStatsRect.adjusted(8, 10, -8, -10),
                       QStringLiteral("记录尚未解锁\n后续可由关卡进度、击败次数、捕获次数或存档标记来更新。"),
                       uiFont(13, QFont::Bold), QColor(88, 61, 31),
                       Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap);
    }

    drawInkRule(p, kDescriptionRect.left(), kDescriptionRect.top(),
                kDescriptionRect.right(), QColor(101, 66, 31), 112);
    drawTextShadow(p, QRect(kDescriptionRect.left() + 12, kDescriptionRect.top() + 6,
                            kDescriptionRect.width() - 24, 20),
                   QStringLiteral("描述"), uiFont(11, QFont::Bold), QColor(68, 38, 13),
                   Qt::AlignLeft | Qt::AlignVCenter);
    drawTextShadow(p, kDescriptionRect.adjusted(12, 27, -12, -6),
                   entry.description, uiFont(10, QFont::Bold), QColor(62, 38, 17),
                   Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap);
}

void EncyclopediaDialog::drawEntryRow(QPainter& p, const QRect& rect, const Entry& entry, int index)
{
    const bool selected = (index == selectedIndex());
    const bool hovered = (index == m_hoverRow);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    if (selected || hovered) {
        const QColor wash = selected ? QColor(33, 91, 106, 26) : QColor(126, 84, 31, 18);
        p.fillRect(rect.adjusted(8, 7, -10, -9), wash);
    }
    drawInkRule(p, rect.left() + 8, rect.bottom() - 4, rect.right() - 10,
                QColor(104, 69, 31), selected ? 130 : 80);
    if (selected) {
        p.fillRect(QRect(rect.left() + 2, rect.top() + 13, 5, rect.height() - 26),
                   QColor(37, 82, 96, 135));
        p.setPen(QPen(QColor(182, 123, 42, 150), 1));
        p.drawLine(rect.left() + 10, rect.top() + 9, rect.left() + 10, rect.bottom() - 11);
    }
    p.restore();

    drawTextShadow(p, QRect(rect.left() + 20, rect.top() + 8, 38, rect.height() - 16),
                   entry.id, uiFont(10, QFont::Bold), QColor(74, 43, 17));

    QRect iconFrame(rect.left() + 68, rect.top() + 8, 76, rect.height() - 14);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(92, 58, 22, 72), 1));
    p.drawLine(iconFrame.left() - 4, iconFrame.top() + 3, iconFrame.left() + 20, iconFrame.top() + 3);
    p.drawLine(iconFrame.left() - 4, iconFrame.top() + 3, iconFrame.left() - 4, iconFrame.top() + 22);
    p.drawLine(iconFrame.right() + 4, iconFrame.bottom() - 3, iconFrame.right() - 20, iconFrame.bottom() - 3);
    p.drawLine(iconFrame.right() + 4, iconFrame.bottom() - 3, iconFrame.right() + 4, iconFrame.bottom() - 22);
    p.restore();
    QPixmap icon = pixmapFromPath(entry.discovered ? entry.iconPath : QString());
    if (!icon.isNull()) {
        p.save();
        p.setOpacity(0.18);
        drawPixmapFit(p, icon, iconFrame.adjusted(4, 2, -4, -2).translated(2, 3));
        p.setOpacity(1.0);
        drawPixmapFit(p, icon, iconFrame.adjusted(4, 2, -4, -2));
        p.restore();
    } else {
        drawUnknownMark(p, iconFrame.adjusted(8, 4, -8, -4), !entry.discovered);
    }

    const bool hideDiscoveredItemTag = (currentPage().category == Category::Item && entry.discovered);
    const bool showTag = !hideDiscoveredItemTag;
    const int nameRight = showTag ? rect.right() - 112 : rect.right() - 16;
    drawTextShadow(p, QRect(rect.left() + 154, rect.top() + 6,
                            nameRight - (rect.left() + 154), rect.height() - 12),
                   entry.discovered ? entry.name : QStringLiteral("？？？"),
                   titleFont(15, QFont::Bold), QColor(42, 25, 9),
                   Qt::AlignLeft | Qt::AlignVCenter);

    if (showTag) {
        drawTag(p, QRect(rect.right() - 98, rect.top() + 18, 82, 28),
                entry.tag, entry.tagColor, entry.discovered);
    }
}

void EncyclopediaDialog::drawStatLines(QPainter& p, const QRect& rect, const Entry& entry)
{
    p.save();
    p.setFont(uiFont(11, QFont::Bold));
    int y = rect.top();
    for (const StatLine& line : entry.stats) {
        QRect lineRect(rect.left(), y, rect.width(), 22);
        p.setPen(QPen(QColor(143, 92, 33, 85), 1));
        p.drawLine(lineRect.left(), lineRect.bottom(), lineRect.right(), lineRect.bottom());

        drawTextShadow(p, QRect(lineRect.left() + 6, lineRect.top(), 92, lineRect.height()),
                       line.label, uiFont(10, QFont::Bold), QColor(77, 48, 22),
                       Qt::AlignLeft | Qt::AlignVCenter);
        drawTextShadow(p, QRect(lineRect.left() + 112, lineRect.top(), lineRect.width() - 118, lineRect.height()),
                       line.value, uiFont(10, QFont::Bold), QColor(38, 26, 11),
                       Qt::AlignLeft | Qt::AlignVCenter);
        y += 22;
        if (y > rect.bottom() - 12) break;
    }
    p.restore();
}

void EncyclopediaDialog::drawTag(QPainter& p, const QRect& rect, const QString& text, const QColor& color, bool discovered)
{
    QColor tagBorder = discovered ? color : QColor(126, 98, 63);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    QPolygon stamp;
    stamp << QPoint(rect.left() + 8, rect.top())
          << QPoint(rect.right() - 8, rect.top())
          << QPoint(rect.right(), rect.top() + 8)
          << QPoint(rect.right(), rect.bottom() - 8)
          << QPoint(rect.right() - 8, rect.bottom())
          << QPoint(rect.left() + 8, rect.bottom())
          << QPoint(rect.left(), rect.bottom() - 8)
          << QPoint(rect.left(), rect.top() + 8);
    p.setPen(QPen(withAlpha(tagBorder, discovered ? 180 : 130), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolygon(stamp);
    p.setPen(QPen(withAlpha(tagBorder, discovered ? 80 : 55), 1));
    p.drawPolygon(stamp.translated(2, 1));
    p.restore();
    drawTextShadow(p, rect, text, uiFont(10, QFont::Bold),
                   discovered ? color : QColor(91, 70, 42));
}

void EncyclopediaDialog::drawDecoratedPanel(QPainter& p, const QRect& rect, const QColor& fill, const QColor& border)
{
    if (!m_uiPanelParchment.isNull()) {
        drawNinePatch(p, m_uiPanelParchment, rect, 28);
    } else {
        drawPixelPanel(p, rect, fill, border, true);
    }
}

void EncyclopediaDialog::drawTextShadow(QPainter& p, const QRect& rect, const QString& text,
                                        const QFont& font, const QColor& color, int flags)
{
    p.save();
    p.setFont(font);
    p.setPen(QColor(43, 24, 8, 90));
    p.drawText(rect.translated(1, 1), flags, text);
    p.setPen(color);
    p.drawText(rect, flags, text);
    p.restore();
}

void EncyclopediaDialog::drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& rect)
{
    if (pixmap.isNull() || rect.isEmpty()) return;
    QSize size = pixmap.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    QRect target(QPoint(0, 0), size);
    target.moveCenter(rect.center());
    p.drawPixmap(target, pixmap);
}

void EncyclopediaDialog::drawUnknownMark(QPainter& p, const QRect& rect, bool locked)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(82, 55, 27, 52), 1, Qt::DashLine));
    for (int x = rect.left() + 4; x < rect.right(); x += 14) {
        p.drawLine(x, rect.top() + 4, x + 18, rect.bottom() - 4);
    }
    p.setPen(QPen(QColor(91, 60, 28, 72), 1));
    p.drawLine(rect.left() + 8, rect.top() + 4, rect.right() - 8, rect.top() + 4);
    p.drawLine(rect.left() + 8, rect.bottom() - 4, rect.right() - 8, rect.bottom() - 4);
    drawTextShadow(p, rect, locked ? QStringLiteral("?") : QStringLiteral("？"),
                   titleFont(rect.height() > 70 ? 44 : 28, QFont::Bold),
                   QColor(91, 60, 28));
    p.restore();
}

const EncyclopediaDialog::CategoryPage& EncyclopediaDialog::currentPage() const
{
    return m_pages[m_categoryIndex];
}

EncyclopediaDialog::CategoryPage& EncyclopediaDialog::currentPage()
{
    return m_pages[m_categoryIndex];
}

int EncyclopediaDialog::selectedIndex() const
{
    const auto& page = currentPage();
    if (page.entries.isEmpty()) return 0;
    const int entryCount = static_cast<int>(page.entries.size());
    return std::clamp(m_selectedByCategory[m_categoryIndex], 0, entryCount - 1);
}

void EncyclopediaDialog::setSelectedIndex(int index)
{
    const auto& page = currentPage();
    if (page.entries.isEmpty()) return;
    const int entryCount = static_cast<int>(page.entries.size());
    m_selectedByCategory[m_categoryIndex] = std::clamp(index, 0, entryCount - 1);
}

int EncyclopediaDialog::currentScroll() const
{
    const auto& page = currentPage();
    const int entryCount = static_cast<int>(page.entries.size());
    const int maxScroll = std::max(0, entryCount - visibleRowCount());
    return std::clamp(m_scrollByCategory[m_categoryIndex], 0, maxScroll);
}

void EncyclopediaDialog::setCurrentScroll(int value)
{
    const auto& page = currentPage();
    const int entryCount = static_cast<int>(page.entries.size());
    const int maxScroll = std::max(0, entryCount - visibleRowCount());
    m_scrollByCategory[m_categoryIndex] = std::clamp(value, 0, maxScroll);
}

int EncyclopediaDialog::visibleRowCount() const
{
    return kVisibleRows;
}

QRect EncyclopediaDialog::tabRect(int index) const
{
    return QRect(222 + index * 168, 112, 150, 50);
}

QRect EncyclopediaDialog::rowRect(int visibleIndex) const
{
    return QRect(kLeftPageRect.left() + 30,
                 kRowTop + visibleIndex * (kRowHeight + kRowGap),
                 kLeftPageRect.width() - 72,
                 kRowHeight);
}

int EncyclopediaDialog::tabAt(const QPoint& pos) const
{
    for (int i = 0; i < m_pages.size(); ++i) {
        if (tabRect(i).contains(pos)) return i;
    }
    return -1;
}

int EncyclopediaDialog::rowAt(const QPoint& pos) const
{
    const auto& page = currentPage();
    const int scroll = currentScroll();
    for (int i = 0; i < visibleRowCount(); ++i) {
        const int index = scroll + i;
        if (index >= page.entries.size()) break;
        if (rowRect(i).contains(pos)) return index;
    }
    return -1;
}

void EncyclopediaDialog::switchCategory(int index)
{
    if (index < 0 || index >= m_pages.size() || index == m_categoryIndex) return;
    m_categoryIndex = index;
    m_hoverRow = -1;
    setSelectedIndex(selectedIndex());
    setCurrentScroll(currentScroll());
    update();
}

void EncyclopediaDialog::updateOpenAnimation()
{
    m_openProgress += 0.034;
    if (m_openProgress >= 1.0) {
        m_openProgress = 1.0;
        m_openTimer.stop();
    }
    update();
}

void EncyclopediaDialog::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->pos();
    if (m_openProgress < 1.0) return;

    const int newHoverTab = tabAt(m_mousePos);
    const int newHoverRow = rowAt(m_mousePos);
    if (newHoverTab != m_hoverTab || newHoverRow != m_hoverRow) {
        m_hoverTab = newHoverTab;
        m_hoverRow = newHoverRow;
        setCursor((m_hoverTab >= 0 || m_hoverRow >= 0) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void EncyclopediaDialog::leaveEvent(QEvent*)
{
    m_hoverTab = -1;
    m_hoverRow = -1;
    setCursor(Qt::ArrowCursor);
    update();
}

void EncyclopediaDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || m_openProgress < 1.0) return;

    const int tab = tabAt(event->pos());
    if (tab >= 0) {
        switchCategory(tab);
        return;
    }

    const int row = rowAt(event->pos());
    if (row >= 0) {
        setSelectedIndex(row);
        update();
    }
}

void EncyclopediaDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_H) {
        close();
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        switchCategory((m_categoryIndex + m_pages.size() - 1) % m_pages.size());
        return;
    }
    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        switchCategory((m_categoryIndex + 1) % m_pages.size());
        return;
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
        const int next = selectedIndex() - 1;
        setSelectedIndex(next);
        if (selectedIndex() < currentScroll()) setCurrentScroll(selectedIndex());
        update();
        return;
    }
    if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
        const int next = selectedIndex() + 1;
        setSelectedIndex(next);
        if (selectedIndex() >= currentScroll() + visibleRowCount()) {
            setCurrentScroll(selectedIndex() - visibleRowCount() + 1);
        }
        update();
        return;
    }

    QDialog::keyPressEvent(event);
}

void EncyclopediaDialog::wheelEvent(QWheelEvent* event)
{
    if (m_openProgress < 1.0) return;
    const int delta = event->angleDelta().y();
    if (delta == 0) return;
    setCurrentScroll(currentScroll() + (delta < 0 ? 1 : -1));
    update();
}
