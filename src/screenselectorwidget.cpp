#include "screenselectorwidget.h"
#include <QGuiApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

ScreenSelectorWidget::ScreenSelectorWidget(QWidget *parent)
    : QWidget(parent), currentAudienceIndex(1), currentConsoleIndex(0), previewIndex(-1), 
      currentDragTarget(DragTarget::None), isDragging(false)
{
    setMinimumSize(400, 200);
    refreshScreens();
}

void ScreenSelectorWidget::refreshScreens()
{
    screens = QGuiApplication::screens();
    if (screens.isEmpty()) return;

    // Calculate virtual geometry
    QRect fullBound;
    for (QScreen *s : screens) {
        fullBound = fullBound.united(s->geometry());
    }

    // Store virtual rect for normalization
    virtualRect = fullBound;
    update();
}

void ScreenSelectorWidget::setAudienceScreen(int index)
{
    if (index >= 0 && index < screens.size()) {
        currentAudienceIndex = index;
        update();
    }
}

void ScreenSelectorWidget::setConsoleScreen(int index)
{
    if (index >= 0 && index < screens.size()) {
        currentConsoleIndex = index;
        update();
    }
}

int ScreenSelectorWidget::getAudienceScreenIndex() const
{
    return currentAudienceIndex;
}

QSize ScreenSelectorWidget::sizeHint() const
{
    return QSize(600, 300);
}

void ScreenSelectorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw Background
    painter.fillRect(rect(), QColor(50, 50, 50));

    if (screens.isEmpty() || virtualRect.isEmpty()) return;

    // Calculate scaling to fit widget
    // Margin of 20px
    QRect drawArea = rect().adjusted(20, 20, -20, -20);
    
    double scaleX = drawArea.width() / virtualRect.width();
    double scaleY = drawArea.height() / virtualRect.height();
    double scale = qMin(scaleX, scaleY);
    
    // Center the cloud of screens
    double scaledW = virtualRect.width() * scale;
    double scaledH = virtualRect.height() * scale;
    double offsetX = drawArea.left() + (drawArea.width() - scaledW) / 2.0;
    double offsetY = drawArea.top() + (drawArea.height() - scaledH) / 2.0;

    mapRects.clear();
    audienceIconRects.clear();
    consoleIconRects.clear();
    
    // Determine visuals based on drag state
    int visualAudienceIndex = (isDragging && currentDragTarget == DragTarget::Audience) ? previewIndex : currentAudienceIndex;
    int visualConsoleIndex = (isDragging && currentDragTarget == DragTarget::Console) ? previewIndex : currentConsoleIndex;

    // Setup fonts
    QFont resFont = font();
    resFont.setPointSize(12);
    
    QFont iconFont = font();
    iconFont.setPointSize(16);
    iconFont.setBold(true);

    for (int i = 0; i < screens.size(); ++i) {
        QScreen *s = screens[i];
        QRect geo = s->geometry();

        // Normalize relative to virtualRect top-left
        double x = (geo.x() - virtualRect.x()) * scale + offsetX;
        double y = (geo.y() - virtualRect.y()) * scale + offsetY;
        double w = geo.width() * scale;
        double h = geo.height() * scale;
        
        QRectF screenRect(x, y, w, h);
        mapRects.append(screenRect);
        
        // --- Calculate Icon Positions ---
        // Default center
        QPointF center = screenRect.center();
        int iconSize = 40;
        int spacing = 10;
        
        QRectF audRect, conRect;
        
        bool hasAudience = (i == visualAudienceIndex);
        bool hasConsole = (i == visualConsoleIndex);
        
        if (hasAudience && hasConsole) {
            // Overlapping: Draw side by side
            // Audience Left, Console Right
            audRect = QRectF(center.x() - iconSize - spacing/2, center.y() - iconSize/2, iconSize, iconSize);
            conRect = QRectF(center.x() + spacing/2, center.y() - iconSize/2, iconSize, iconSize);
        } else {
            // Centered if single
            if (hasAudience) {
                audRect = QRectF(center.x() - iconSize/2, center.y() - iconSize/2, iconSize, iconSize);
            }
            if (hasConsole) {
                conRect = QRectF(center.x() - iconSize/2, center.y() - iconSize/2, iconSize, iconSize);
            }
        }
        
        // Store hit rects if valid for this screen (even if empty/null)
        if (hasAudience) audienceIconRects.append(audRect);
        else audienceIconRects.append(QRectF()); // Placeholder
        
        if (hasConsole) consoleIconRects.append(conRect);
        else consoleIconRects.append(QRectF()); // Placeholder

        // --- Drawing ---
        
        // Color Logic (Screen Background)
        QColor color = QColor("#3498db"); // Blue for Empty
        if (hasAudience) color = QColor("#e74c3c"); // Red
        else if (hasConsole) color = QColor("#2ecc71"); // Green
        
        if (hasAudience && hasConsole) {
             // Mixed color or just one? Let's stick to Audience Red as base, or maybe Purple?
             color = QColor("#9b59b6"); // Purple for overlap
        }

        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(screenRect, 8, 8);
        
        // Draw Text (Resolution) - Shift down if icons take center? 
        // Or draw at bottom
        painter.setPen(Qt::white);
        painter.setFont(resFont);
        QString text = QString("%1\n%2x%3").arg(i).arg(geo.width()).arg(geo.height());
        // Align bottom
        painter.drawText(screenRect.adjusted(0, 0, 0, -5), Qt::AlignBottom | Qt::AlignHCenter, text);
        
        // Draw Icons
        painter.setFont(iconFont);
        
        if (hasAudience) {
             painter.setPen(QPen(Qt::white, 3));
             painter.setBrush(Qt::NoBrush);
             painter.drawEllipse(audRect);
             painter.drawText(audRect, Qt::AlignCenter, "A");
        }
        
        if (hasConsole) {
             painter.setPen(QPen(Qt::white, 3));
             painter.setBrush(Qt::NoBrush);
             painter.drawRect(conRect);
             painter.drawText(conRect, Qt::AlignCenter, "C");
        }
    }
}

void ScreenSelectorWidget::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = event->position();
    
    // Hit test icons first
    
    // Check Audience Icons
    for (int i = 0; i < audienceIconRects.size(); ++i) {
        if (!audienceIconRects[i].isEmpty() && audienceIconRects[i].contains(pos)) {
            currentDragTarget = DragTarget::Audience;
            previewIndex = i;
            isDragging = true;
            update();
            return;
        }
    }
    
    // Check Console Icons
    for (int i = 0; i < consoleIconRects.size(); ++i) {
        if (!consoleIconRects[i].isEmpty() && consoleIconRects[i].contains(pos)) {
            currentDragTarget = DragTarget::Console;
            previewIndex = i;
            isDragging = true;
            update();
            return;
        }
    }
    
    // Fallback: Click on screen rect
    // If screen has ONLY one item, drag that.
    // If screen has both, requires icon click (which we missed above).
    // If empty -> ignore? Or maybe move A by default? Let's ignore for safety/clarity.
    for (int i = 0; i < mapRects.size(); ++i) {
        if (mapRects[i].contains(pos)) {
            bool hasA = (i == currentAudienceIndex);
            bool hasC = (i == currentConsoleIndex);
            
            if (hasA && !hasC) {
                currentDragTarget = DragTarget::Audience;
                previewIndex = i;
                isDragging = true;
                update();
                return;
            }
            if (hasC && !hasA) {
                currentDragTarget = DragTarget::Console;
                previewIndex = i;
                isDragging = true;
                update();
                return;
            }
             if (hasC && hasA) {
                // Ambiguous click on background of shared screen. 
                // Do nothing, force user to pick icon.
                return;
            }
             // Empty screen? Maybe just move A?
             // Let's allow moving A to empty screen by clicking empty screen?
             // No, that's creating A. We are moving.
             // Assume click on empty screen means nothing.
        }
    }
}

void ScreenSelectorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        // Find which screen we are hovering
        for (int i = 0; i < mapRects.size(); ++i) {
            if (mapRects[i].contains(event->position())) {
                if (i != previewIndex) {
                    previewIndex = i;
                    update(); // Just visual update
                }
                break;
            }
        }
    }
}

void ScreenSelectorWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (isDragging) {
        if (previewIndex != -1) {
            if (currentDragTarget == DragTarget::Audience && previewIndex != currentAudienceIndex) {
                currentAudienceIndex = previewIndex;
                emit audienceScreenChanged(currentAudienceIndex);
            }
            else if (currentDragTarget == DragTarget::Console && previewIndex != currentConsoleIndex) {
                currentConsoleIndex = previewIndex;
                emit consoleScreenChanged(currentConsoleIndex);
            }
        }
        
        isDragging = false;
        currentDragTarget = DragTarget::None;
        previewIndex = -1;
        update();
    }
}
