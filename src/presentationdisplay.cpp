#include "presentationdisplay.h"
#include <QPainterPath>
#include <QPen>

PresentationDisplay::PresentationDisplay(QWidget *parent)
    : QWidget(parent), pdf(nullptr), currentPage(0), splitView(false),
      laserActive(false), zoomActive(false), zoomFactor(2.0f), zoomDiameter(250)
{
    setMouseTracking(true);
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
    laserActive = active;
    if (active) {
        if (zoomActive) enableZoom(false); 
        
        if (laserCursor.bitmap().isNull()) {
             laserCursor = createLaserCursor();
        }
        setCursor(laserCursor);
    } else {
        unsetCursor();
    }
    update();
}

void PresentationDisplay::enableZoom(bool active)
{
    zoomActive = active;
    if (active) {
        if (laserActive) enableLaserPointer(false);
        setCursor(Qt::BlankCursor);
    } else {
        unsetCursor();
    }
    update();
}

void PresentationDisplay::setZoomSettings(float factor, int diameter)
{
    zoomFactor = factor;
    zoomDiameter = diameter;
    if (zoomActive) update();
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

void PresentationDisplay::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    if (zoomActive) {
        update();
    }
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

    if (cachedSlide.isNull()) return;

    // Center the slide while maintaining aspect ratio
    QSize slideSize = cachedSlide.size(); // Logical size due to DPR
    slideSize.scale(size(), Qt::KeepAspectRatio);
    
    QRect slideRect(QPoint(0, 0), slideSize);
    slideRect.moveCenter(rect().center());
    
    painter.drawImage(slideRect, cachedSlide);

    // Draw Magnifier
    if (zoomActive) {
        painter.save();
        
        int r = zoomDiameter / 2;
        QPoint center = mousePos;
        
        QPainterPath path;
        path.addEllipse(center, r, r);
        painter.setClipPath(path);
        
        // Source Rect
        float srcR = r / zoomFactor;
        
        // We need to map screen coordinates to image coordinates
        double scaleX = (double)cachedSlide.width() / slideRect.width(); 
        double scaleY = (double)cachedSlide.height() / slideRect.height();
        
        QRectF sourceRect(center.x() - srcR, center.y() - srcR, srcR * 2, srcR * 2);
        
        double imgSrcX = (sourceRect.x() - slideRect.x()) * scaleX;
        double imgSrcY = (sourceRect.y() - slideRect.y()) * scaleY;
        double imgSrcW = sourceRect.width() * scaleX;
        double imgSrcH = sourceRect.height() * scaleY;
        
        painter.drawImage(QRect(center.x() - r, center.y() - r, zoomDiameter, zoomDiameter), 
                          cachedSlide, 
                          QRectF(imgSrcX, imgSrcY, imgSrcW, imgSrcH));
                          
        painter.setClipping(false);
        painter.setPen(QPen(Qt::darkGray, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, r, r);
        
        painter.restore();
    }
}
