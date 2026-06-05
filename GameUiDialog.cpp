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

        const QRect board((width() - 800) / 2, (height() - 640) / 2, 800, 640);
        drawPixmapFit(p, m_board, board);
        drawNoticeText(p, QRect(board.left() + 126, board.top() + 82, board.width() - 252, 50),
                       m_title, 27, QColor(83, 38, 12), true);
        drawNoticeText(p, QRect(board.left() + 142, board.top() + 142, board.width() - 284, 48),
                       m_body, 17, QColor(70, 42, 16), true);

        m_optionRects.clear();
        const int visible = qMin(5, m_options.size());
        const int startY = board.top() + 222;
        for (int i = 0; i < visible; ++i) {
            QRect row(board.left() + 156, startY + i * 72, board.width() - 312, 62);
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

        m_cancelRect = QRect(board.center().x() - 92, board.bottom() - 106, 184, 54);
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
}

namespace GameUi {
void showWoodMessage(QWidget* parent, const QString& title, const QString& body)
{
    WoodMessageDialog dialog(parent, title, body);
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
