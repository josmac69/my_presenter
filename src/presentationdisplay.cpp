#include "presentationdisplay.h"

PresentationDisplay::PresentationDisplay(QWidget *parent)
    : QWidget(parent), showLaser(false)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent); // Optimization
}

void PresentationDisplay::updateSlide(const QImage &slide)
{
    currentSlide = slide;
    update();
}

void PresentationDisplay::setLaserPointer(bool active, const QPoint &pos)
{
    showLaser = active;
    laserPos = pos;
    update();
}

void PresentationDisplay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    
    // Draw black background
    painter.fillRect(rect(), Qt::black);

    if (!currentSlide.isNull()) {
        // Center the slide while maintaining aspect ratio
        QSize slideSize = currentSlide.size().scaled(size(), Qt::KeepAspectRatio);
        QRect slideRect(QPoint(0, 0), slideSize);
        slideRect.moveCenter(rect().center());
        
        painter.drawImage(slideRect, currentSlide);
    }

    if (showLaser) {
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(laserPos, 10, 10);
    }
}

void PresentationDisplay::mouseMoveEvent(QMouseEvent *event)
{
    // If the mouse is moved over this window, update the laser position locally
    // In a real dual-screen setup, the presenter might want to control the laser 
    // from the main window, but this allows direct interaction too.
    if (showLaser) {
        setLaserPointer(true, event->pos());
    }
}
