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
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderCurrentSlide();
    QCursor createLaserCursor();

    QPdfDocument *pdf;
    int currentPage;
    bool splitView;
    QImage cachedSlide;
    QCursor laserCursor;
};

#endif // PRESENTATIONDISPLAY_H
