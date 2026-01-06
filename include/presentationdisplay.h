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
    
    void setLaserPointer(bool active, const QPoint &pos = QPoint());

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderCurrentSlide();

    QPdfDocument *pdf;
    int currentPage;
    bool splitView;
    
    QImage cachedSlide;
    bool showLaser;
    QPoint laserPos;
};

#endif // PRESENTATIONDISPLAY_H
