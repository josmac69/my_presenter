#include "mainwindow.h"
#include <QFileDialog>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QWindow>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentPage(0), showLaser(false)
{
    pdf = new QPdfDocument(this);
    bookmarkModel = new QPdfBookmarkModel(this);
    bookmarkModel->setDocument(pdf);
    presentationDisplay = new PresentationDisplay(nullptr); // Null parent = separate window

    setupUi();
    detectScreens();

    // Auto-open for convenience
    QTimer::singleShot(0, this, [this](){
        QString fileName = QFileDialog::getOpenFileName(this, "Open PDF", "", "PDF Files (*.pdf)");
        if (!fileName.isEmpty()) {
            loadPdf(fileName);
        }
    });
}

MainWindow::~MainWindow()
{
    if (presentationDisplay) {
        presentationDisplay->close();
        delete presentationDisplay;
    }
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(centralWidget);
    
    // TOC View (Left)
    tocView = new QTreeView(this);
    tocView->setModel(bookmarkModel);
    tocView->setHeaderHidden(true);
    tocView->setFixedWidth(200);
    connect(tocView, &QTreeView::activated, this, &MainWindow::onBookmarkActivated);
    connect(tocView, &QTreeView::clicked, this, &MainWindow::onBookmarkActivated);

    // Center Area (Current Slide)
    currentSlideView = new QLabel("Current Slide");
    currentSlideView->setAlignment(Qt::AlignCenter);
    currentSlideView->setStyleSheet("background: #dddddd; border: 1px solid #999;");
    currentSlideView->setMinimumSize(400, 300);

    // Right Area (Preview + Notes)
    QWidget *rightContainer = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);

    nextSlideView = new QLabel("Next Slide");
    nextSlideView->setAlignment(Qt::AlignCenter);
    nextSlideView->setStyleSheet("background: #eeeeee; border: 1px dashed #aaa;");
    nextSlideView->setFixedHeight(200);
    
    notesView = new QTextEdit(this);
    notesView->setPlaceholderText("Notes for this slide...");

    rightLayout->addWidget(new QLabel("Next Slide:"));
    rightLayout->addWidget(nextSlideView);
    rightLayout->addWidget(new QLabel("Notes:"));
    rightLayout->addWidget(notesView);

    // Assemble Splitters
    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(tocView);
    mainSplitter->addWidget(currentSlideView);
    mainSplitter->addWidget(rightContainer);
    
    // Initial sizes
    mainSplitter->setStretchFactor(1, 4); // Give priority to current slide
    mainSplitter->setStretchFactor(2, 1);

    layout->addWidget(mainSplitter);
    setCentralWidget(centralWidget);
    
    setWindowTitle("Presenter Console");
    resize(1200, 800);
}

void MainWindow::detectScreens()
{
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.size() > 1) {
        // Move Presentation Window to the second screen
        QRect screenGeo = screens[1]->geometry();
        presentationDisplay->move(screenGeo.topLeft());
        presentationDisplay->resize(screenGeo.size());
        presentationDisplay->showFullScreen();
        
        // Optional: Ensure it's associated with the correct screen if handle exists
        if (presentationDisplay->windowHandle()) {
            presentationDisplay->windowHandle()->setScreen(screens[1]);
        }
    } else {
        // Single screen mode: Just show it as a normal window
        presentationDisplay->resize(800, 600);
        presentationDisplay->show();
    }
}

void MainWindow::loadPdf(const QString &filePath)
{
    pdf->load(filePath);
    if (pdf->status() == QPdfDocument::Status::Ready) {
        updateViews();
    } else {
        QMessageBox::critical(this, "Error", "Failed to load PDF.");
    }
}

void MainWindow::updateViews()
{
    if (pdf->status() != QPdfDocument::Status::Ready) return;

    // 1. Render Current Side
    QSize currentSize = currentSlideView->size();
    QImage currentImg = pdf->render(currentPage, currentSize);
    currentSlideView->setPixmap(QPixmap::fromImage(currentImg));
    
    // 2. Update Audience Display
    // We render at high res for the audience display (e.g., 1920x1080 or detection based)
    // For simplicity, we assume a reasonable 1080p target buffer or scale 
    QSize fullScreenSize(1920, 1080); 
    QImage fullImg = pdf->render(currentPage, fullScreenSize);
    presentationDisplay->updateSlide(fullImg);

    // 3. Render Next Slide Preview
    if (currentPage + 1 < pdf->pageCount()) {
        QSize paramPreview = nextSlideView->size();
        QImage nextImg = pdf->render(currentPage + 1, paramPreview);
        nextSlideView->setPixmap(QPixmap::fromImage(nextImg));
    } else {
        nextSlideView->setText("End of Presentation");
        nextSlideView->clear(); 
    }
    
    // 4. Update Notes (Mockup)
    notesView->setText(QString("Notes for Slide %1").arg(currentPage + 1));
}

void MainWindow::onBookmarkActivated(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int page = index.data((int)QPdfBookmarkModel::Role::Page).toInt();
    if (page >= 0 && page < pdf->pageCount()) {
        currentPage = page;
        updateViews();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_Down:
        case Qt::Key_Space:
            if (currentPage < pdf->pageCount() - 1) {
                currentPage++;
                updateViews();
            }
            break;
        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_Backspace:
            if (currentPage > 0) {
                currentPage--;
                updateViews();
            }
            break;
        case Qt::Key_L:
            showLaser = !showLaser;
            // TODO: Implement laser logic for PresentationDisplay
             // For now, toggle a visual indicator on the console
             event->accept();
            break;
        case Qt::Key_Escape:
            close();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    presentationDisplay->close();
    QMainWindow::closeEvent(event);
}
