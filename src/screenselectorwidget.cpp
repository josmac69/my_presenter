#include "screenselectorwidget.h"
#include <QGuiApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

ScreenSelectorWidget::ScreenSelectorWidget(QWidget *parent)
    : QWidget(parent), currentAudienceIndex(1), currentConsoleIndex(0), previewIndex(-1), isDragging(false)
{
    setMinimumSize(200, 80);
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
    return QSize(300, 100);
}

void ScreenSelectorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw Background
    painter.fillRect(rect(), QColor(50, 50, 50));

    if (screens.isEmpty() || virtualRect.isEmpty()) return;

    // Calculate scaling to fit widget
    // Margin of 10px
    QRect drawArea = rect().adjusted(10, 10, -10, -10);
    
    double scaleX = drawArea.width() / virtualRect.width();
    double scaleY = drawArea.height() / virtualRect.height();
    double scale = qMin(scaleX, scaleY);
    
    // Center the cloud of screens
    double scaledW = virtualRect.width() * scale;
    double scaledH = virtualRect.height() * scale;
    double offsetX = drawArea.left() + (drawArea.width() - scaledW) / 2.0;
    double offsetY = drawArea.top() + (drawArea.height() - scaledH) / 2.0;

    mapRects.clear();
    
    // Determine which screen is visually the audience screen right now
    int visualAudienceIndex = isDragging ? previewIndex : currentAudienceIndex;

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

        // Color Logic
        QColor color;
        if (i == visualAudienceIndex) {
            color = QColor("#e74c3c"); // Red for Audience
        } else if (i == currentConsoleIndex) {
            color = QColor("#2ecc71"); // Green for Console
        } else {
             color = QColor("#3498db"); // Blue for Empty
        }

        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(screenRect, 4, 4);
        
        // Draw Text (Resolution)
        painter.setPen(Qt::white);
        QString text = QString("%1\n%2x%3").arg(i).arg(geo.width()).arg(geo.height());
        painter.drawText(screenRect, Qt::AlignCenter, text);
        
        // Icon overlay
        if (i == visualAudienceIndex) {
             painter.setPen(QPen(Qt::white, 2));
             painter.drawEllipse(screenRect.center(), 10, 10);
             painter.drawText(screenRect, Qt::AlignCenter, "A");
        }
        if (i == currentConsoleIndex) {
             // Draw "C" unless it's strictly hidden by A (same index)
             // Even if same index, we might want to hint it's there?
             // But 'A' takes precedence in color. Let's just draw 'C' if not A or if we want to show overlap.
             if (i != visualAudienceIndex) {
                painter.setPen(QPen(Qt::white, 2));
                painter.drawRect(screenRect.center().x() - 10, screenRect.center().y() - 10, 20, 20);
                painter.drawText(screenRect, Qt::AlignCenter, "C");
             }
        }
    }
}

void ScreenSelectorWidget::mousePressEvent(QMouseEvent *event)
{
    for (int i = 0; i < mapRects.size(); ++i) {
        if (mapRects[i].contains(event->position())) {
            isDragging = true;
            previewIndex = i; // Visually jump to clicked screen immediately
            update();
            break;
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
        if (previewIndex != -1 && previewIndex != currentAudienceIndex) {
            // Commit change
            currentAudienceIndex = previewIndex;
            emit audienceScreenChanged(currentAudienceIndex);
        }
        isDragging = false;
        previewIndex = -1;
        update();
    }
}
