#include "mainwindow.h"
#include <QFileDialog>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentPage(0), showLaser(false)
{
    setMouseTracking(true);
    QString fileName = QFileDialog::getOpenFileName(this, "Open PDF", "", "PDF Files (*.pdf)");
    if (!fileName.isEmpty()) {
        loadPdf(fileName);
        renderCurrentPage();
    }
}

MainWindow::~MainWindow() {}

void MainWindow::loadPdf(const QString &filePath)
{
    pdf.load(filePath);
}

void MainWindow::renderCurrentPage()
{
    if (pdf.pageCount() == 0 || currentPage >= pdf.pageCount()) return;

    QSize pageSize = pdf.pagePointSize(currentPage).toSize();
    currentSlide = pdf.render(currentPage, pageSize);
    update();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_Down:
            if (currentPage < pdf.pageCount() - 1) {
                currentPage++;
                renderCurrentPage();
            }
            break;
        case Qt::Key_Left:
        case Qt::Key_Up:
            if (currentPage > 0) {
                currentPage--;
                renderCurrentPage();
            }
            break;
        case Qt::Key_L:
            showLaser = !showLaser;
            update();
            break;
        case Qt::Key_Escape:
            close();
            break;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    laserPointerPos = event->pos();
    if (showLaser) update();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawImage(rect(), currentSlide);

    if (showLaser) {
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(laserPointerPos, 10, 10);
    }
}
