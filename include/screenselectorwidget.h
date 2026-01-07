#ifndef SCREENSELECTORWIDGET_H
#define SCREENSELECTORWIDGET_H

#include <QWidget>
#include <QList>
#include <QScreen>
#include <QRect>

class ScreenSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenSelectorWidget(QWidget *parent = nullptr);
    void refreshScreens();
    void setAudienceScreen(int index);
    void setConsoleScreen(int index);
    int getAudienceScreenIndex() const;

signals:
    void audienceScreenChanged(int index);
    void consoleScreenChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;

private:
    QList<QScreen*> screens;
    QRectF virtualRect;       // Normalized bounding box of all screens
    QList<QRectF> mapRects;   // Normalized rects for painting
    
    // Store exact rects of icons for hit testing
    QList<QRectF> audienceIconRects; 
    QList<QRectF> consoleIconRects;

    int currentAudienceIndex;
    int currentConsoleIndex;
    int previewIndex; // For visualizing drag before commit
    
    enum class DragTarget { None, Audience, Console };
    DragTarget currentDragTarget;
    bool isDragging;
};

#endif // SCREENSELECTORWIDGET_H
