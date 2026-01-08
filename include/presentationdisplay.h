#ifndef PRESENTATIONDISPLAY_H
#define PRESENTATIONDISPLAY_H

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <QPoint>
#include <QPdfDocument>

class PresentationDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit PresentationDisplay(QWidget *parent = nullptr);
    
    void setDocument(QPdfDocument *doc);
    void setPage(int page);
    void setSplitMode(bool split);
    
    // Explicit update trigger if needed, though setters usually trigger repaint
    void refreshSlide();
    
    void enableLaserPointer(bool active);
    void setLaserSettings(int diameter, int opacity); // Configurable size/opacity
    void setLaserColor(const QColor &color); // NEW: Configurable color

    void enableZoom(bool active);
    void setZoomSettings(float factor, int diameter);
    void setAspectRatioLock(bool locked);

    // Drawing
    void enableDrawing(bool active);
    void setDrawingColor(const QColor &color);
    void setDrawingThickness(int thickness);
    void setDrawingStyle(Qt::PenStyle style);
    void clearDrawings();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderCurrentSlide();
    QCursor createLaserCursor();
    QCursor createPenCursor(); // Helper for pencil cursor

    QPdfDocument *pdf;
    int currentPage;
    bool splitView;
    QImage cachedSlide;
    
    // Laser
    QCursor laserCursor;
    bool laserActive; // Track state for disabling mouse tracking if neither active?
    int laserDiameter;
    int laserOpacity;
    QColor laserColor;

    // Zoom
    bool zoomActive;
    float zoomFactor;
    int zoomDiameter;
    QPoint mousePos;
    
    // Drawing
    struct Stroke {
        QPolygonF points;
        QPen pen;
    };
    QList<Stroke> strokes;
    QPolygonF currentStroke;
    bool drawingActive;
    QColor drawColor;
    int drawThickness;
    Qt::PenStyle drawStyle;
    bool isDrawing; // Track if mouse is down

    // Window Mode
    bool lockedAspectRatio;
    bool isResizing;
};

#endif // PRESENTATIONDISPLAY_H
