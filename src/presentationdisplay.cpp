#include "presentationdisplay.h"

PresentationDisplay::PresentationDisplay(QWidget *parent)
    : QWidget(parent), pdf(nullptr), currentPage(0), splitView(false), showLaser(false)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void PresentationDisplay::setDocument(QPdfDocument *doc)
{
    pdf = doc;
    refreshSlide();
}

void PresentationDisplay::setPage(int page)
{
    if (currentPage != page) {
        currentPage = page;
        refreshSlide();
    }
}

void PresentationDisplay::setSplitMode(bool split)
{
    if (splitView != split) {
        splitView = split;
        refreshSlide();
    }
}

void PresentationDisplay::refreshSlide()
{
    renderCurrentSlide();
    update();
}

void PresentationDisplay::setLaserPointer(bool active, const QPoint &pos)
{
    showLaser = active;
    laserPos = pos;
    update();
}

void PresentationDisplay::resizeEvent(QResizeEvent *)
{
    // Re-render on resize to maintain crisp quality
    refreshSlide();
}

void PresentationDisplay::renderCurrentSlide()
{
    if (!pdf || pdf->status() != QPdfDocument::Status::Ready) {
        cachedSlide = QImage();
        return;
    }

    // Determine target size in physical pixels
    QSize targetSize = size() * devicePixelRatio();
    QSizeF pageSize = pdf->pagePointSize(currentPage);

    if (splitView) {
        // Logical slide size is half the page width
        QSizeF slideSize(pageSize.width() / 2.0, pageSize.height());
        
        // Calculate aspect-ratio preserving scale to fit targetSize
        QSize scaledSize = slideSize.scaled(targetSize, Qt::KeepAspectRatio).toSize();
        
        // Calculate the full render size based on this scale
        // Explicitly calculate scale factor to apply to full page
        qreal scale = (qreal)scaledSize.width() / slideSize.width();
        
        QSize fullRenderSize(pageSize.width() * scale, pageSize.height() * scale);
        
        QImage fullImg = pdf->render(currentPage, fullRenderSize);
        cachedSlide = fullImg.copy(0, 0, fullImg.width() / 2, fullImg.height());
    } else {
        // Standard mode: fit page to target size keeping aspect ratio
        QSize renderSize = pageSize.scaled(targetSize, Qt::KeepAspectRatio).toSize();
        cachedSlide = pdf->render(currentPage, renderSize);
    }
    
    cachedSlide.setDevicePixelRatio(devicePixelRatio());
}

void PresentationDisplay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    
    // Draw black background
    painter.fillRect(rect(), Qt::black);

    if (!cachedSlide.isNull()) {
        // Center the slide while maintaining aspect ratio
        QSize slideSize = cachedSlide.size(); // Logical size due to DPR
        slideSize.scale(size(), Qt::KeepAspectRatio);
        
        QRect slideRect(QPoint(0, 0), slideSize);
        slideRect.moveCenter(rect().center());
        
        painter.drawImage(slideRect, cachedSlide);
    }

    if (showLaser) {
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        // Laser pointer needs to be visible, size independent of DPI?
        // 10px logical size is usually fine.
        painter.drawEllipse(laserPos, 10, 10);
    }
}

void PresentationDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (showLaser) {
        setLaserPointer(true, event->pos());
    }
}
