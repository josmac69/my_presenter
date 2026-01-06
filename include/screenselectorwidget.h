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
    
    int currentAudienceIndex;
    int currentConsoleIndex;
    int previewIndex; // For visualizing drag before commit
    bool isDragging;
    QPoint dragStartPos;
};

#endif // SCREENSELECTORWIDGET_H
