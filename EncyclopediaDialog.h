#pragma once

#include <QColor>
#include <QDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWheelEvent>

class EncyclopediaDialog : public QDialog
{
public:
    explicit EncyclopediaDialog(int currentStage = 1, QWidget* parent = nullptr);
    ~EncyclopediaDialog() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    int m_currentStage = 1;
    enum class Category {
        Fish,
        Equipment,
        Item,
        Enemy,
        Boss
    };

    struct StatLine {
        QString label;
        QString value;
    };

    struct Entry {
        QString id;
        QString name;
        QString tag;
        QString iconPath;
        QString detailImagePath;
        QVector<StatLine> stats;
        QString description;
        bool discovered = true;
        QColor tagColor = QColor("#2f7a45");
    };

    struct CategoryPage {
        Category category;
        QString title;
        QString icon;
        int discoveredCount = 0;
        int totalCount = 0;
        QVector<Entry> entries;
    };

    void loadAssets();
    void buildCatalog();

    void drawOpeningAnimation(QPainter& p);
    void drawBook(QPainter& p);
    void drawBookContents(QPainter& p);
    void drawTitle(QPainter& p);
    void drawTabs(QPainter& p);
    void drawListPage(QPainter& p);
    void drawDetailPage(QPainter& p);
    void drawEntryRow(QPainter& p, const QRect& rect, const Entry& entry, int index);
    void drawStatLines(QPainter& p, const QRect& rect, const Entry& entry);
    void drawTag(QPainter& p, const QRect& rect, const QString& text, const QColor& color, bool discovered);
    void drawDecoratedPanel(QPainter& p, const QRect& rect, const QColor& fill, const QColor& border);
    void drawTextShadow(QPainter& p, const QRect& rect, const QString& text,
                        const QFont& font, const QColor& color,
                        int flags = Qt::AlignCenter);
    void drawPixmapFit(QPainter& p, const QPixmap& pixmap, const QRect& rect);
    void drawUnknownMark(QPainter& p, const QRect& rect, bool locked);

    const CategoryPage& currentPage() const;
    CategoryPage& currentPage();
    int selectedIndex() const;
    void setSelectedIndex(int index);
    int currentScroll() const;
    void setCurrentScroll(int value);
    int visibleRowCount() const;
    QRect tabRect(int index) const;
    QRect rowRect(int visibleIndex) const;
    int tabAt(const QPoint& pos) const;
    int rowAt(const QPoint& pos) const;
    void switchCategory(int index);
    void updateOpenAnimation();

    QVector<CategoryPage> m_pages;
    QVector<int> m_selectedByCategory;
    QVector<int> m_scrollByCategory;

    int m_categoryIndex = 0;
    int m_hoverTab = -1;
    int m_hoverRow = -1;
    QPoint m_mousePos;

    QTimer m_openTimer;
    qreal m_openProgress = 0.0;

    QPixmap m_bookFrame;
    QPixmap m_fallbackSea;
    QPixmap m_uiPanelParchment;
    QPixmap m_uiPanelSeaChart;
    QPixmap m_uiPanelStat;
    QPixmap m_uiDetailImagePanel;
    QPixmap m_uiIconFrame;
    QPixmap m_uiTabNormal;
    QPixmap m_uiTabHover;
    QPixmap m_uiTabSelected;
    QPixmap m_uiRowNormal;
    QPixmap m_uiRowHover;
    QPixmap m_uiRowSelected;
    QPixmap m_uiTagBadge;
    QPixmap m_uiScrollTrack;
    QPixmap m_uiScrollThumb;
    QPixmap m_uiOpenClosedCover;
    QPixmap m_uiOpenPageLeft;
    QPixmap m_uiOpenPageRight;
    QPixmap m_uiOpenFlipPage;
};
