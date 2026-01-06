#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QPainter>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QPixmap>
#include <QTimer>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void loadPdf(const QString &filePath);
    void renderCurrentPage();

    QPdfDocument pdf;
    int currentPage;
    QPoint laserPointerPos;
    bool showLaser;
    QImage currentSlide;
};

#endif // MAINWINDOW_H
