#include "GameUiDialog.h"

#include <QDialog>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QVector>
#include <QWheelEvent>
#include <QWidget>

namespace {
QFont noticeFont(int pixelSize, bool bold = false)
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(pixelSize);
    font.setWeight(bold ? QFont::Bold : QFont::Normal);
    font.setHintingPreference(QFont::PreferFullHinting);
    return font;
}

void drawNoticeText(QPainter& p, const QRect& rect, const QString& text, int pixelSize,
                    const QColor& color, bool bold = false,
                    int flags = Qt::AlignCenter | Qt::TextWordWrap)
{
    QFont font = noticeFont(pixelSize, bold);
    const int minSize = qMax(12, pixelSize - 7);
    while (font.pixelSize() > minSize) {
        QFontMetrics metrics(font);
        const QRect measured = metrics.boundingRect(rect, flags | Qt::TextWordWrap, text);
        if (measured.width() <= rect.width() && measured.height() <= rect.height()) {
            break;
        }
        font.setPixelSize(font.pixelSize() - 1);
    }

    p.setFont(font);
    p.setPen(color);
    p.drawText(rect, flags | Qt::TextWordWrap, text);
}

void drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& target)
{
    if (pixmap.isNull() || target.isEmpty()) return;
    p.drawPixmap(target, pixmap, pixmap.rect());
}

class WoodMessageDialog final : public QDialog {
public:
    WoodMessageDialog(QWidget* parent, const QString& title, const QString& body)
        : QDialog(parent), m_title(title), m_body(body)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        const QSize overlaySize = parent ? parent->size() : QSize(1280, 720);
        setFixedSize(overlaySize);
        if (parent) {
            move(parent->mapToGlobal(QPoint(0, 0)));
        }
        m_board.load(":/FishingVoyage/ui/common/wood_notice_board.png");
        m_button.load(":/FishingVoyage/ui/common/wood_notice_button.png");
        m_icon.load(":/FishingVoyage/ui/common/notice_icon_info.png");
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.fillRect(rect(), QColor(0, 0, 0, 92));

        const QRect board((width() - 640) / 2, (height() - 540) / 2, 640, 540);
        drawPixmapFit(p, m_board, board);

        drawNoticeText(p, QRect(board.left() + 106, board.top() + 92, board.width() - 212, 54),
                       m_title, 27, QColor(83, 38, 12), true, Qt::AlignCenter);
        drawNoticeText(p, QRect(board.left() + 118, board.top() + 174, board.width() - 236, 118),
                       m_body, 18, QColor(72, 43, 17), true, Qt::AlignCenter | Qt::TextWordWrap);

        m_okRect = QRect(board.center().x() - 96, board.bottom() - 126, 192, 58);
        drawPixmapFit(p, m_button, m_okRect);
        drawNoticeText(p, m_okRect.adjusted(16, 0, -16, -3),
                       QStringLiteral("\u786e\u5b9a"), 20, QColor(255, 232, 170), true);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        setCursor(m_okRect.contains(event->position().toPoint()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && m_okRect.contains(event->position().toPoint())) {
            accept();
        }
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
            event->key() == Qt::Key_Space || event->key() == Qt::Key_Escape) {
            accept();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    QString m_title;
    QString m_body;
    QPixmap m_board;
    QPixmap m_button;
    QPixmap m_icon;
    QRect m_okRect;
};

class WoodChoiceDialog final : public QDialog {
public:
    WoodChoiceDialog(QWidget* parent, const QString& title, const QString& body, const QStringList& options)
        : QDialog(parent), m_title(title), m_body(body), m_options(options)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        const QSize overlaySize = parent ? parent->size() : QSize(1280, 720);
        setFixedSize(overlaySize);
        setMouseTracking(true);
        if (parent) {
            move(parent->mapToGlobal(QPoint(0, 0)));
        }
        m_board.load(":/FishingVoyage/ui/common/wood_notice_board.png");
        m_button.load(":/FishingVoyage/ui/common/wood_notice_button.png");
    }

    int selectedIndex() const { return m_selectedIndex; }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.fillRect(rect(), QColor(0, 0, 0, 92));

        const bool compact = m_options.size() <= 2;
        const QSize boardSize = compact ? QSize(640, 512) : QSize(800, 640);
        const QRect board((width() - boardSize.width()) / 2,
                          (height() - boardSize.height()) / 2,
                          boardSize.width(), boardSize.height());
        drawPixmapFit(p, m_board, board);
        drawNoticeText(p, QRect(board.left() + 104, board.top() + (compact ? 66 : 82),
                                board.width() - 208, 50),
                       m_title, compact ? 25 : 27, QColor(83, 38, 12), true);
        drawNoticeText(p, QRect(board.left() + 118, board.top() + (compact ? 126 : 142),
                                board.width() - 236, compact ? 70 : 48),
                       m_body, compact ? 16 : 17, QColor(70, 42, 16), true);

        m_optionRects.clear();
        const int visible = qMin(6, m_options.size());
        for (int i = 0; i < visible; ++i) {
            QRect row;
            if (compact) {
                const int buttonWidth = 214;
                const int buttonY = board.bottom() - (visible == 1 ? 142 : 194);
                row = QRect(board.center().x() - 228 + i * 242, buttonY, buttonWidth, 58);
            }
            else {
                const int rowHeight = visible > 5 ? 52 : 62;
                const int rowStep = visible > 5 ? 56 : 72;
                const int startY = board.top() + (visible > 5 ? 194 : 222);
                row = QRect(board.left() + 156, startY + i * rowStep,
                            board.width() - 312, rowHeight);
            }
            m_optionRects.append(row);
            const bool hovered = i == m_hoverIndex;
            if (hovered) {
                p.save();
                p.setOpacity(0.96);
                drawPixmapFit(p, m_button, row.adjusted(-10, -6, 10, 6));
                p.restore();
            }
            drawPixmapFit(p, m_button, row);
            const QString option = m_options[i];
            if (compact) {
                drawNoticeText(p, row.adjusted(18, 0, -18, -3), option, 17,
                               hovered ? QColor(255, 246, 190) : QColor(255, 232, 170),
                               true, Qt::AlignCenter);
                continue;
            }

            QString titleLine = option;
            QString statLine;
            const int firstSep = option.indexOf(QStringLiteral("  "));
            const int secondSep = firstSep >= 0 ? option.indexOf(QStringLiteral("  "), firstSep + 2) : -1;
            if (secondSep > 0) {
                titleLine = option.left(secondSep).trimmed();
                statLine = option.mid(secondSep + 2).trimmed();
            }

            const QFont titleFont = noticeFont(15, true);
            const QFont statFont = noticeFont(12, true);
            const QString titleLabel = QFontMetrics(titleFont)
                .elidedText(titleLine, Qt::ElideRight, row.width() - 58);
            const QString statLabel = QFontMetrics(statFont)
                .elidedText(statLine, Qt::ElideRight, row.width() - 58);
            drawNoticeText(p, QRect(row.left() + 28, row.top() + 7, row.width() - 56, 22), titleLabel, 15,
                           hovered ? QColor(255, 246, 190) : QColor(255, 232, 170),
                           true, Qt::AlignLeft | Qt::AlignVCenter);
            if (!statLabel.isEmpty()) {
                drawNoticeText(p, QRect(row.left() + 28, row.top() + 31, row.width() - 56, 18), statLabel, 12,
                               hovered ? QColor(255, 236, 158) : QColor(238, 205, 134),
                               true, Qt::AlignLeft | Qt::AlignVCenter);
            }
        }

        if (compact && visible == 1) {
            m_cancelRect = QRect(board.center().x() + 14, board.bottom() - 142, 214, 58);
        }
        else {
            m_cancelRect = QRect(board.center().x() - 92, board.bottom() - 106, 184, 54);
        }
        if (m_cancelHover) {
            p.save();
            p.setOpacity(0.96);
            drawPixmapFit(p, m_button, m_cancelRect.adjusted(-8, -5, 8, 5));
            p.restore();
        }
        drawPixmapFit(p, m_button, m_cancelRect);
        drawNoticeText(p, m_cancelRect.adjusted(12, 0, -12, -3),
                       QStringLiteral("\u53d6\u6d88"), 18,
                       m_cancelHover ? QColor(255, 246, 190) : QColor(255, 232, 170), true);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const QPoint pos = event->position().toPoint();
        int nextHover = -1;
        for (int i = 0; i < m_optionRects.size(); ++i) {
            if (m_optionRects[i].contains(pos)) {
                nextHover = i;
                break;
            }
        }
        const bool nextCancelHover = m_cancelRect.contains(pos);
        if (nextHover != m_hoverIndex || nextCancelHover != m_cancelHover) {
            m_hoverIndex = nextHover;
            m_cancelHover = nextCancelHover;
            update();
        }
        setCursor((m_hoverIndex >= 0 || m_cancelHover) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void leaveEvent(QEvent*) override
    {
        if (m_hoverIndex != -1 || m_cancelHover) {
            m_hoverIndex = -1;
            m_cancelHover = false;
            update();
        }
        setCursor(Qt::ArrowCursor);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) return;
        const QPoint pos = event->position().toPoint();
        for (int i = 0; i < m_optionRects.size(); ++i) {
            if (m_optionRects[i].contains(pos)) {
                m_selectedIndex = i;
                accept();
                return;
            }
        }
        if (m_cancelRect.contains(pos)) {
            reject();
        }
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Escape) {
            reject();
            return;
        }
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !m_options.isEmpty()) {
            m_selectedIndex = 0;
            accept();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    QString m_title;
    QString m_body;
    QStringList m_options;
    QPixmap m_board;
    QPixmap m_button;
    QVector<QRect> m_optionRects;
    QRect m_cancelRect;
    int m_hoverIndex = -1;
    bool m_cancelHover = false;
    int m_selectedIndex = -1;
};

struct GuideSection {
    QString title;
    QString body;
};

class WoodGuideDialog final : public QDialog {
public:
    WoodGuideDialog(QWidget* parent, const QVector<GuideSection>& sections)
        : QDialog(parent), m_sections(sections)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        const QSize overlaySize = parent ? parent->size() : QSize(1280, 720);
        setFixedSize(overlaySize);
        if (parent) {
            move(parent->mapToGlobal(QPoint(0, 0)));
        }
        m_board.load(":/FishingVoyage/ui/common/wood_notice_board.png");
        m_button.load(":/FishingVoyage/ui/common/wood_notice_button.png");
        m_scrollTrack.load(":/FishingVoyage/encyclopedia/ui/scroll_track.png");
        m_scrollThumb.load(":/FishingVoyage/encyclopedia/ui/scroll_thumb.png");
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.fillRect(rect(), QColor(0, 0, 0, 104));

        const QRect board = boardRect();
        drawPixmapFit(p, m_board, board);

        drawNoticeText(p, QRect(board.left() + 112, board.top() + 62,
                                board.width() - 224, 54),
                       QStringLiteral("\u64cd\u4f5c\u8bf4\u660e"), 28,
                       QColor(83, 38, 12), true);

        const QRect viewport = contentRect(board);
        m_contentHeight = calculateContentHeight(viewport.width());
        m_scrollOffset = qBound(0, m_scrollOffset, maxScroll(viewport));

        p.save();
        p.setClipRect(viewport);
        p.translate(0, -m_scrollOffset);

        int y = viewport.top() + 4;
        const QFont sectionFont = noticeFont(19, true);
        const QFont bodyFont = noticeFont(15, false);
        const QFontMetrics bodyMetrics(bodyFont);
        const QColor titleColor(103, 50, 15);
        const QColor bodyColor(67, 43, 21);

        for (const GuideSection& section : m_sections) {
            const QRect titleRect(viewport.left() + 4, y, viewport.width() - 12, 30);
            p.setFont(sectionFont);
            p.setPen(titleColor);
            p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, section.title);
            y += 34;

            const QRect measured = bodyMetrics.boundingRect(
                QRect(0, 0, viewport.width() - 20, 4000),
                Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                section.body);
            const QRect bodyRect(viewport.left() + 12, y,
                                 viewport.width() - 20, measured.height() + 4);
            p.setFont(bodyFont);
            p.setPen(bodyColor);
            p.drawText(bodyRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                       section.body);
            y += measured.height() + 22;
        }
        p.restore();

        drawScrollBar(p, viewport);

        m_okRect = QRect(board.center().x() - 102, board.bottom() - 91, 204, 58);
        if (m_okHover) {
            p.save();
            p.setOpacity(0.96);
            drawPixmapFit(p, m_button, m_okRect.adjusted(-8, -5, 8, 5));
            p.restore();
        }
        drawPixmapFit(p, m_button, m_okRect);
        drawNoticeText(p, m_okRect.adjusted(16, 0, -16, -3),
                       QStringLiteral("\u786e\u5b9a"), 20,
                       m_okHover ? QColor(255, 246, 190) : QColor(255, 232, 170),
                       true);
    }

    void wheelEvent(QWheelEvent* event) override
    {
        const QRect viewport = contentRect(boardRect());
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            setScrollOffset(m_scrollOffset - delta / 2, viewport);
            event->accept();
            return;
        }
        QDialog::wheelEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const bool nextHover = m_okRect.contains(event->position().toPoint());
        if (nextHover != m_okHover) {
            m_okHover = nextHover;
            update();
        }
        setCursor(m_okHover ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void leaveEvent(QEvent*) override
    {
        if (m_okHover) {
            m_okHover = false;
            update();
        }
        setCursor(Qt::ArrowCursor);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && m_okRect.contains(event->position().toPoint())) {
            accept();
            return;
        }

        if (event->button() == Qt::LeftButton) {
            const QRect viewport = contentRect(boardRect());
            if (m_scrollTrackRect.contains(event->position().toPoint())) {
                const int direction = event->position().y() < m_scrollThumbRect.top() ? -1 : 1;
                setScrollOffset(m_scrollOffset + direction * viewport.height(), viewport);
            }
        }
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        const QRect viewport = contentRect(boardRect());
        switch (event->key()) {
        case Qt::Key_Escape:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            accept();
            return;
        case Qt::Key_Up:
            setScrollOffset(m_scrollOffset - 42, viewport);
            return;
        case Qt::Key_Down:
            setScrollOffset(m_scrollOffset + 42, viewport);
            return;
        case Qt::Key_PageUp:
            setScrollOffset(m_scrollOffset - viewport.height(), viewport);
            return;
        case Qt::Key_PageDown:
        case Qt::Key_Space:
            setScrollOffset(m_scrollOffset + viewport.height(), viewport);
            return;
        case Qt::Key_Home:
            setScrollOffset(0, viewport);
            return;
        case Qt::Key_End:
            setScrollOffset(maxScroll(viewport), viewport);
            return;
        default:
            break;
        }
        QDialog::keyPressEvent(event);
    }

private:
    QRect boardRect() const
    {
        const int boardWidth = qMin(820, width() - 36);
        const int boardHeight = qMin(680, height() - 24);
        return QRect((width() - boardWidth) / 2, (height() - boardHeight) / 2,
                     boardWidth, boardHeight);
    }

    QRect contentRect(const QRect& board) const
    {
        return QRect(board.left() + 116, board.top() + 128,
                     board.width() - 264, board.height() - 250);
    }

    int calculateContentHeight(int width) const
    {
        const QFontMetrics bodyMetrics(noticeFont(15, false));
        int height = 4;
        for (const GuideSection& section : m_sections) {
            const QRect measured = bodyMetrics.boundingRect(
                QRect(0, 0, width - 20, 4000),
                Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                section.body);
            height += 34 + measured.height() + 22;
        }
        return height;
    }

    int maxScroll(const QRect& viewport) const
    {
        return qMax(0, m_contentHeight - viewport.height());
    }

    void setScrollOffset(int offset, const QRect& viewport)
    {
        const int next = qBound(0, offset, maxScroll(viewport));
        if (next != m_scrollOffset) {
            m_scrollOffset = next;
            update();
        }
    }

    void drawScrollBar(QPainter& p, const QRect& viewport)
    {
        m_scrollTrackRect = QRect(viewport.right() + 16, viewport.top(), 18, viewport.height());
        drawPixmapFit(p, m_scrollTrack, m_scrollTrackRect);

        const int maximum = maxScroll(viewport);
        const int thumbHeight = maximum > 0
            ? qMax(46, viewport.height() * viewport.height() / qMax(1, m_contentHeight))
            : viewport.height();
        const int travel = viewport.height() - thumbHeight;
        const int thumbY = maximum > 0
            ? viewport.top() + qRound(travel * (static_cast<qreal>(m_scrollOffset) / maximum))
            : viewport.top();
        m_scrollThumbRect = QRect(m_scrollTrackRect.left() - 4, thumbY,
                                  m_scrollTrackRect.width() + 8, thumbHeight);
        drawPixmapFit(p, m_scrollThumb, m_scrollThumbRect);
    }

    QVector<GuideSection> m_sections;
    QPixmap m_board;
    QPixmap m_button;
    QPixmap m_scrollTrack;
    QPixmap m_scrollThumb;
    QRect m_okRect;
    QRect m_scrollTrackRect;
    QRect m_scrollThumbRect;
    int m_scrollOffset = 0;
    int m_contentHeight = 0;
    bool m_okHover = false;
};
}

namespace GameUi {
void showWoodMessage(QWidget* parent, const QString& title, const QString& body)
{
    WoodMessageDialog dialog(parent, title, body);
    dialog.exec();
}

void showOperationGuide(QWidget* parent)
{
    const QVector<GuideSection> sections = {
        {
            QStringLiteral("\u57fa\u7840\u822a\u884c"),
            QStringLiteral(
                "\u00b7 W / A / S / D\uff1a\u9a7e\u9a76\u8239\u53ea\u79fb\u52a8\u3002\n"
                "\u00b7 Shift\uff1a\u79fb\u52a8\u65f6\u6301\u7eed\u52a0\u901f\uff0c\u4f1a\u6d88\u8017\u4f53\u529b\u3002\n"
                "\u00b7 Space\uff1a\u671d\u5f53\u524d\u65b9\u5411\u51b2\u523a\uff0c\u9002\u5408\u8eb2\u907f\u654c\u4eba\u548c\u5371\u9669\u5730\u5f62\u3002\n"
                "\u00b7 Esc\uff1a\u6682\u505c\u6216\u7ee7\u7eed\u6e38\u620f\uff1bQ\uff1a\u4fdd\u5b58\u5e76\u9000\u51fa\u3002")
        },
        {
            QStringLiteral("\u6355\u9c7c\u4e0e\u6218\u6597"),
            QStringLiteral(
                "\u00b7 \u9f20\u6807\u5de6\u952e\uff1a\u4f18\u5148\u653b\u51fb\u70b9\u51fb\u4f4d\u7f6e\u7684\u654c\u4eba\uff1b\u672a\u547d\u4e2d\u654c\u4eba\u65f6\uff0c\u6355\u9c7c\u88c5\u5907\u4f1a\u9501\u5b9a\u8303\u56f4\u5185\u6700\u8fd1\u7684\u9c7c\u3002\n"
                "\u00b7 \u8fde\u70b9\u578b\u6355\u9c7c\uff1a\u5728\u9650\u65f6\u5185\u8fbe\u5230\u8981\u6c42\u6b21\u6570\uff0c\u8d8a\u5feb\u5b8c\u6210\u7ed3\u679c\u8d8a\u597d\u3002\n"
                "\u00b7 \u6821\u51c6\u578b\u6355\u9c7c\uff1a\u6307\u9488\u8fdb\u5165\u968f\u673a\u76ee\u6807\u533a\u65f6\u70b9\u51fb\uff1b\u7a00\u6709\u9c7c\u7684\u6307\u9488\u66f4\u5feb\u3002\n"
                "\u00b7 \u6821\u51c6\u5931\u8d25\u540e\u9c7c\u4f1a\u9003\u79bb\u4e00\u6bb5\u8ddd\u79bb\uff0c\u4e0d\u4f1a\u51ed\u7a7a\u6d88\u5931\u3002\n"
                "\u00b7 E\uff1a\u9707\u8361\u6ce2\u5145\u80fd\u5b8c\u6210\u540e\u53ef\u9707\u5f00\u8fd1\u8eab\u654c\u4eba\u3002")
        },
        {
            QStringLiteral("\u88c5\u5907\u4e0e\u9053\u5177"),
            QStringLiteral(
                "\u00b7 1 - 6\uff1a\u9009\u62e9\u5feb\u6377\u680f\u6b66\u5668\uff1b\u65e0\u6b66\u5668\u65f6\u4f7f\u7528\u8be5\u683c\u9053\u5177\u3002\n"
                "\u00b7 B\uff1a\u6253\u5f00\u8239\u8231\u80cc\u5305\uff0c\u53ef\u88c5\u5907\u5230\u4efb\u610f\u5feb\u6377\u4f4d\u3001\u4fee\u7406\u6216\u4e22\u5f03\u6b66\u5668\u3002\n"
                "\u00b7 \u521d\u59cb\u57fa\u7840\u9c7c\u7aff\u548c\u94c1\u5236\u9c7c\u53c9\u4e3a\u65e0\u9650\u8010\u4e45\u3002\u5176\u4ed6\u6b66\u5668\u635f\u574f\u540e\u5fc5\u987b\u5148\u4fee\u7406\u3002\n"
                "\u00b7 \u8239\u4f53\u4fee\u7406\u5305\u53ea\u6062\u590d\u8239\u4f53\u8010\u4e45\uff1b\u7d27\u6025\u88c5\u5907\u4fee\u7406\u7528\u4e8e\u5f53\u524d\u9009\u4e2d\u6b66\u5668\u3002")
        },
        {
            QStringLiteral("\u80cc\u5305\u3001\u56fe\u9274\u4e0e\u5546\u5e97"),
            QStringLiteral(
                "\u00b7 H\uff1a\u6253\u5f00\u822a\u6d77\u56fe\u9274\uff1b\u6355\u9c7c\u8fdb\u884c\u4e2d\u4e0d\u80fd\u6253\u5f00\u80cc\u5305\u6216\u56fe\u9274\u3002\n"
                "\u00b7 \u9c7c\u7c7b\u4f1a\u5728\u9996\u6b21\u6355\u83b7\u540e\u8bb0\u5f55\uff0c\u654c\u4eba\u4f1a\u5728\u9996\u6b21\u9047\u89c1\u65f6\u8bb0\u5f55\u3002\n"
                "\u00b7 O\uff1a\u5f00\u542f\u6216\u5173\u95ed\u6d4b\u8bd5\u6a21\u5f0f\uff1b\u5f00\u542f\u540e\u4f1a\u6062\u590d\u72b6\u6001\u5e76\u4fdd\u8bc1\u8db3\u591f\u7684\u6d4b\u8bd5\u91d1\u5e01\u3002\n"
                "\u00b7 P\uff1a\u6d4b\u8bd5\u6a21\u5f0f\u4e0b\u968f\u65f6\u6253\u5f00\u5546\u5e97\u3002")
        },
        {
            QStringLiteral("\u6d77\u51b5\u4e0e\u5173\u5361"),
            QStringLiteral(
                "\u00b7 \u987a\u6d6a\u4f1a\u63a8\u52a8\u8239\u53ea\u52a0\u901f\uff0c\u9006\u6d6a\u4f1a\u964d\u901f\uff0c\u6548\u679c\u53d6\u51b3\u4e8e\u8239\u53ea\u4e0e\u6d77\u6d6a\u7684\u76f8\u5bf9\u65b9\u5411\u3002\n"
                "\u00b7 \u5929\u6c14\u4f1a\u9010\u6e10\u8fc7\u6e21\u3002\u6d77\u96fe\u4f1a\u7f29\u5c0f\u89c6\u91ce\uff0c\u66b4\u98ce\u96e8\u4e2d\u9700\u6ce8\u610f\u843d\u96f7\u9884\u8b66\u3002\n"
                "\u00b7 \u5c9b\u5c7f\u548c\u5927\u578b\u5730\u5f62\u6709\u5b9e\u4f53\u4f46\u4e0d\u4f24\u8239\uff1b\u7901\u77f3\u4f1a\u9020\u6210\u78b0\u649e\u4f24\u5bb3\uff0c\u6f29\u6da1\u4f1a\u5e72\u6270\u822a\u884c\u3002\n"
                "\u00b7 \u6bcf 3 \u5173\u51fa\u73b0\u4e00\u573a Boss \u6218\uff0c\u5230\u8fbe\u822a\u7a0b\u7ec8\u70b9\u5e76\u5b8c\u6210\u672c\u5173\u76ee\u6807\u5373\u53ef\u8fc7\u5173\u3002")
        }
    };

    WoodGuideDialog dialog(parent, sections);
    dialog.exec();
}

int selectWoodOption(QWidget* parent, const QString& title, const QString& body, const QStringList& options)
{
    if (options.isEmpty()) return -1;
    WoodChoiceDialog dialog(parent, title, body, options);
    if (dialog.exec() != QDialog::Accepted) return -1;
    return dialog.selectedIndex();
}
}
