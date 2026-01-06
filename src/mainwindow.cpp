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
    : QMainWindow(parent), currentPage(0), showLaser(false), useSplitView(false), timerRunning(false)
{
    pdf = new QPdfDocument(this);
    bookmarkModel = new QPdfBookmarkModel(this);
    bookmarkModel->setDocument(pdf);
    presentationDisplay = new PresentationDisplay(nullptr); // Null parent = separate window

    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateTimers);
    clockTimer->start(1000);

    setupUi();
    detectScreens();

    // Auto-open for convenience
    QTimer::singleShot(0, this, [this](){
        QString fileName = QFileDialog::getOpenFileName(this, "Open PDF", "", "PDF Files (*.pdf)");
        if (!fileName.isEmpty()) {
            loadPdf(fileName);
            startTime = QTime::currentTime();
            timerRunning = true;
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
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Top Bar (Timers)
    QHBoxLayout *topBar = new QHBoxLayout();
    timeLabel = new QLabel("00:00:00");
    elapsedLabel = new QLabel("00:00:00"); // Presentation timer
    QFont timerFont = font();
    timerFont.setPointSize(14);
    timerFont.setBold(true);
    timeLabel->setFont(timerFont);
    elapsedLabel->setFont(timerFont);

    topBar->addWidget(new QLabel("Time:"));
    topBar->addWidget(timeLabel);
    topBar->addStretch();
    topBar->addWidget(new QLabel("Elapsed:"));
    topBar->addWidget(elapsedLabel);
    mainLayout->addLayout(topBar);
    
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
    
    // Notes: Text or Image (Split View)
    notesView = new QTextEdit(this);
    notesView->setPlaceholderText("Notes for this slide...");
    
    notesImageView = new QLabel("Notes View"); // For Beamer notes
    notesImageView->setAlignment(Qt::AlignCenter);
    notesImageView->setStyleSheet("background: white; border: 1px solid #ccc;");
    notesImageView->hide(); // Hidden by default

    rightLayout->addWidget(new QLabel("Next Slide:"));
    rightLayout->addWidget(nextSlideView);
    rightLayout->addWidget(new QLabel("Notes:"));
    rightLayout->addWidget(notesView);
    rightLayout->addWidget(notesImageView);

    // Assemble Splitters
    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(tocView);
    mainSplitter->addWidget(currentSlideView);
    mainSplitter->addWidget(rightContainer);
    
    // Initial sizes
    mainSplitter->setStretchFactor(1, 4); // Give priority to current slide
    mainSplitter->setStretchFactor(2, 1);

    mainLayout->addWidget(mainSplitter);
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
    // For SplitView, we need the Left Half for Audience/Console, Right Half for Notes
    // For SplitView, we need the Left Half for Audience/Console, Right Half for Notes
    
    // We render the full page first, then crop logic handle it
    // Note: Rendering full resolution then cropping is easier for now
    QSize renderSize = pdf->pagePointSize(currentPage).toSize() * 2; // 2x scale for quality
    
    QImage currentFull = pdf->render(currentPage, renderSize);
    QImage audienceImg, notesImg;

    if (useSplitView) {
        int w = currentFull.width() / 2;
        int h = currentFull.height();
        audienceImg = currentFull.copy(0, 0, w, h);
        notesImg = currentFull.copy(w, 0, w, h);
    } else {
        audienceImg = currentFull;
        notesImg = QImage(); // No image notes
    }

    currentSlideView->setPixmap(QPixmap::fromImage(audienceImg).scaled(currentSlideView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    presentationDisplay->updateSlide(audienceImg);

    // Beamer Notes Update
    if (useSplitView) {
        notesView->hide();
        notesImageView->show();
        notesImageView->setPixmap(QPixmap::fromImage(notesImg).scaled(notesImageView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        notesImageView->hide();
        notesView->show();
        notesView->setText(QString("Notes for Slide %1").arg(currentPage + 1));
    }

    // 3. Render Next Slide Preview
    if (currentPage + 1 < pdf->pageCount()) {
        QSize nextRenderSize = pdf->pagePointSize(currentPage + 1).toSize(); 
        QImage nextFull = pdf->render(currentPage + 1, nextRenderSize);
        QImage nextPreview;
        
        if (useSplitView) {
            nextPreview = nextFull.copy(0, 0, nextFull.width() / 2, nextFull.height());
        } else {
            nextPreview = nextFull;
        }
        nextSlideView->setPixmap(QPixmap::fromImage(nextPreview).scaled(nextSlideView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

void MainWindow::updateTimers()
{
    timeLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
    if (timerRunning) {
        int secs = startTime.secsTo(QTime::currentTime());
        int h = secs / 3600;
        int m = (secs % 3600) / 60;
        int s = secs % 60;
        elapsedLabel->setText(QString("%1:%2:%3")
                              .arg(h, 2, 10, QChar('0'))
                              .arg(m, 2, 10, QChar('0'))
                              .arg(s, 2, 10, QChar('0')));
    }
}

void MainWindow::toggleSplitView()
{
    useSplitView = !useSplitView;
    updateViews();
    QMessageBox::information(this, "Mode Changed", 
                             useSplitView ? "Split Mode Enabled (Left=Slide, Right=Notes)" 
                                          : "Standard Mode Enabled");
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
             event->accept();
            break;
        case Qt::Key_S:
            toggleSplitView();
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
