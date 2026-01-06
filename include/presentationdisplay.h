#ifndef PRESENTATIONDISPLAY_H
#define PRESENTATIONDISPLAY_H

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <QPoint>

class PresentationDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit PresentationDisplay(QWidget *parent = nullptr);
    void updateSlide(const QImage &slide);
    void setLaserPointer(bool active, const QPoint &pos = QPoint());

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QImage currentSlide;
    bool showLaser;
    QPoint laserPos;
};

#endif // PRESENTATIONDISPLAY_H
