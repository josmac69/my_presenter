#include "presentationdisplay.h"
#include <QPainterPath>
#include <QPen>
#include <QWindow>
#include <QGuiApplication>
#include <QScreen>

PresentationDisplay::PresentationDisplay(QWidget *parent)
    : QWidget(parent), pdf(nullptr), currentPage(0), splitView(false),
      laserActive(false), laserDiameter(60), laserOpacity(128), laserColor(Qt::red), zoomActive(false), zoomFactor(2.0f), zoomDiameter(250),
      drawingActive(false), drawColor(Qt::red), drawThickness(5), drawStyle(Qt::SolidLine), isDrawing(false),
      lockedAspectRatio(false), isResizing(false)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
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
    renderCurrentSlide();
    // Default: drawings persist. If we wanted to clear them on slide change:
    // clearDrawings(); 
    // For now, per user request, we might want persistent canvas or one per page. 
    // Standard presentation behavior usually clears drawings reset on page change.
    // Let's implement auto-clear for now to keep it clean.
    strokes.clear();
    currentStroke.clear();
    update();
}

void PresentationDisplay::enableLaserPointer(bool active)
{
    laserActive = active;
    
    // Only update cursor if Zoom is NOT active. 
    // If Zoom or Drawing is active, they might override.
    if (!zoomActive && !drawingActive) {
        if (active) {
            if (laserCursor.bitmap().isNull()) {
                 laserCursor = createLaserCursor();
            }
            setCursor(laserCursor);
        } else {
            unsetCursor();
        }
    }
    update();
}

void PresentationDisplay::enableZoom(bool active)
{
    zoomActive = active;
    
    if (active) {
        // Zoom takes visual precedence
        setCursor(Qt::BlankCursor);
    } else {
        // When deactivating Zoom, fall back to Laser if active, otherwise Default
        if (laserActive) {
            setCursor(laserCursor);
        } else {
            unsetCursor();
        }
    }
    update();
}

void PresentationDisplay::setZoomSettings(float factor, int diameter)
{
    zoomFactor = factor;
    zoomDiameter = diameter;
    if (zoomActive) update();
}

void PresentationDisplay::setLaserSettings(int diameter, int opacity)
{
    laserDiameter = diameter;
    laserOpacity = opacity;
    
    // Regenerate cursor
    laserCursor = createLaserCursor();
    
    // If active and not Zoomed, apply immediately
    if (laserActive && !zoomActive) {
        setCursor(laserCursor);
    }
}

void PresentationDisplay::setLaserColor(const QColor &color)
{
    laserColor = color;
    
    // Regenerate cursor
    laserCursor = createLaserCursor();
    
    // If active and not Zoomed, apply immediately
    if (laserActive && !zoomActive) {
        setCursor(laserCursor);
    }
}

void PresentationDisplay::enableDrawing(bool active)
{
    drawingActive = active;
    if (active) {
        // Drawing takes precedence or similar to laser?
        // Let's say Drawing > Laser
        setCursor(createPenCursor());
    } else {
        // Fallback
        if (laserActive && !zoomActive) {
            setCursor(laserCursor);
        } else if (zoomActive) {
            setCursor(Qt::BlankCursor);
        } else {
            unsetCursor();
        }
    }
    update();
}

void PresentationDisplay::setDrawingColor(const QColor &color)
{
    drawColor = color;
    if (drawingActive) setCursor(createPenCursor()); // Update pen cursor color if we want custom cursor
}

void PresentationDisplay::setDrawingThickness(int thickness)
{
    drawThickness = thickness;
}

void PresentationDisplay::setDrawingStyle(Qt::PenStyle style)
{
    drawStyle = style;
}

void PresentationDisplay::clearDrawings()
{
    strokes.clear();
    currentStroke.clear();
    update();
}

QCursor PresentationDisplay::createPenCursor()
{
    // Simple pencil or crosshair. Let's draw a pencil icon or just use CrossCursor
    // Larger pencil with border
    int size = 36;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Draw pencil tip (Polygon)
    // Scale points from 24x24 to 36x36 roughly 1.5x
    QVector<QPointF> points;
    points << QPointF(3, 33) << QPointF(12, 33) << QPointF(33, 12) << QPointF(24, 3);
    
    // Black border
    QPen borderPen(Qt::black);
    borderPen.setWidth(2);
    p.setPen(borderPen);
    p.setBrush(drawColor); 
    p.drawPolygon(QPolygonF(points));
    
    // Hotspot bottom-left (0, height-1)
    return QCursor(pix, 0, 35);
}

QCursor PresentationDisplay::createLaserCursor()
{
    int size = laserDiameter; 
    // Ensure size is valid
    if (size < 10) size = 10;
    
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Radial gradient for "blurry borders"
    QRadialGradient gradient(size/2, size/2, size/2);
    // Center opacity user-defined
    // Center opacity user-defined
    QColor c1 = laserColor; c1.setAlpha(laserOpacity);
    gradient.setColorAt(0.0, c1); 
    // Midpoint opacity slightly less
    QColor c2 = laserColor; c2.setAlpha(static_cast<int>(laserOpacity * 0.8));
    gradient.setColorAt(0.5, c2); 
    // Edge transparent
    QColor c3 = laserColor; c3.setAlpha(0);
    gradient.setColorAt(1.0, c3);   
    
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);

    return QCursor(pixmap);
}

void PresentationDisplay::resizeEvent(QResizeEvent *)
{
    // If in Fullscreen mode, do NOT resize the window. 
    // The paintEvent handles centering and black bars.
    if (isFullScreen()) {
        refreshSlide();
        return;
    }

    if (lockedAspectRatio && !isResizing && pdf && pdf->pageCount() > 0) {
        
        QSizeF pageSize = pdf->pagePointSize(currentPage);
        if (!pageSize.isEmpty() && pageSize.height() > 0) {
            double aspect = pageSize.width() / pageSize.height();
            if (splitView) aspect /= 2.0;

            // Get available screen geometry
            QScreen *scr = screen();
            if (!scr && windowHandle()) scr = windowHandle()->screen();
            if (!scr) scr = QGuiApplication::primaryScreen();
            
            QRect avail = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);
            
            int targetW = width();
            int targetH = static_cast<int>(targetW / aspect);

            // Constrain to screen bounds
            if (targetW > avail.width()) {
                targetW = avail.width();
                targetH = static_cast<int>(targetW / aspect);
            }
            
            if (targetH > avail.height()) {
                targetH = avail.height();
                targetW = static_cast<int>(targetH * aspect);
            }
            
            // Check delta to avoid infinite loops due to integer rounding
            if (abs(width() - targetW) > 2 || abs(height() - targetH) > 2) {
                isResizing = true;
                resize(targetW, targetH);
                isResizing = false;
            }
        }
    }
    // Re-render on resize to maintain crisp quality
    refreshSlide();
}

void PresentationDisplay::setAspectRatioLock(bool locked)
{
    lockedAspectRatio = locked;
    if (locked) {
        // Trigger resize logic
        resizeEvent(nullptr);
    }
}

void PresentationDisplay::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    if (zoomActive) {
        update();
    }
    
    if (drawingActive && isDrawing) {
        currentStroke << event->pos();
        update();
    }
}

void PresentationDisplay::mousePressEvent(QMouseEvent *event)
{
    if (drawingActive && event->button() == Qt::LeftButton) {
        isDrawing = true;
        currentStroke.clear();
        currentStroke << event->pos();
    }
}

void PresentationDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (drawingActive && event->button() == Qt::LeftButton && isDrawing) {
        isDrawing = false;
        if (!currentStroke.isEmpty()) {
            Stroke s;
            s.points = currentStroke;
            s.pen = QPen(drawColor, drawThickness, drawStyle, Qt::RoundCap, Qt::RoundJoin);
            strokes.append(s);
            currentStroke.clear();
        }
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

    // Draw Strokes
    painter.setRenderHint(QPainter::Antialiasing);
    for (const Stroke &s : strokes) {
        painter.setPen(s.pen);
        painter.drawPolyline(s.points);
    }
    
    if (!currentStroke.isEmpty()) {
        QPen pen(drawColor, drawThickness, drawStyle, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawPolyline(currentStroke);
    }

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
