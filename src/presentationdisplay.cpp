#include "presentationdisplay.h"

PresentationDisplay::PresentationDisplay(QWidget *parent)
    : QWidget(parent), pdf(nullptr), currentPage(0), splitView(false)
{
    // setMouseTracking(true); // Redundant with QCursor
    setAttribute(Qt::WA_OpaquePaintEvent);
    laserCursor = createLaserCursor();
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

void PresentationDisplay::enableLaserPointer(bool active)
{
    if (active) {
        if (laserCursor.bitmap().isNull()) {
             laserCursor = createLaserCursor();
        }
        setCursor(laserCursor);
    } else {
        unsetCursor();
    }
}

QCursor PresentationDisplay::createLaserCursor()
{
    int size = 60; // 60px wide (2x bigger)
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Radial gradient for "blurry borders"
    QRadialGradient gradient(size/2, size/2, size/2);
    // Center opacity ~50% (128/255)
    gradient.setColorAt(0.0, QColor(255, 0, 0, 128)); 
    // Midpoint opacity ~40% (100/255)
    gradient.setColorAt(0.5, QColor(255, 0, 0, 100)); 
    // Edge transparent
    gradient.setColorAt(1.0, QColor(255, 0, 0, 0));   
    
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);

    return QCursor(pixmap);
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

    if (!cachedSlide.isNull()) {
        // Center the slide while maintaining aspect ratio
        QSize slideSize = cachedSlide.size(); // Logical size due to DPR
        slideSize.scale(size(), Qt::KeepAspectRatio);
        
        QRect slideRect(QPoint(0, 0), slideSize);
        slideRect.moveCenter(rect().center());
        
        painter.drawImage(slideRect, cachedSlide);
    }
}
// mouseMoveEvent removed as we use QCursor now
