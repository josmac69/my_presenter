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
    presentationDisplay->setDocument(pdf);
    presentationDisplay->installEventFilter(this); // Capture keys from audience window

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

    connect(pdf, &QPdfDocument::statusChanged, this, [this](QPdfDocument::Status status){
        if (status == QPdfDocument::Status::Ready) {
            updateViews();
            presentationDisplay->setDocument(pdf);
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
    elapsedLabel = new QLabel("00:00:00");
    
    // Laser Pointer Toggle
    laserCheckBox = new QCheckBox("Laser Pointer (L)");
    connect(laserCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        showLaser = checked;
        if (checked && zoomCheckBox->isChecked()) zoomCheckBox->setChecked(false);
        presentationDisplay->enableLaserPointer(checked);
    });

    QFont timerFont = font();
    timerFont.setPointSize(14);
    timerFont.setBold(true);
    timeLabel->setFont(timerFont);
    elapsedLabel->setFont(timerFont);

    topBar->addWidget(new QLabel("Time:"));
    topBar->addWidget(timeLabel);
    topBar->addSpacing(20);
    topBar->addWidget(laserCheckBox);
    topBar->addStretch();
    topBar->addWidget(new QLabel("Elapsed:"));
    topBar->addWidget(elapsedLabel);
    topBar->addWidget(elapsedLabel);
    // mainLayout->addLayout(topBar); // Add later or keep sequence

    // Zoom Controls Bar
    QHBoxLayout *zoomBar = new QHBoxLayout();
    zoomCheckBox = new QCheckBox("Zoom (Z)");
    connect(zoomCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        // Exclusive with laser? PresentationDisplay handles overrides but UI should sync
        if (checked && laserCheckBox->isChecked()) laserCheckBox->setChecked(false);
        presentationDisplay->enableZoom(checked);
    });
    
    QSlider *sizeSlider = new QSlider(Qt::Horizontal);
    sizeSlider->setRange(250, 1500);
    sizeSlider->setValue(250); // Default
    sizeSlider->setFixedWidth(150);
    
    QSlider *magSlider = new QSlider(Qt::Horizontal);
    magSlider->setRange(2, 5);
    magSlider->setValue(2); // Default
    magSlider->setFixedWidth(100);
    
    // Store pointer for key updates? Or just leave local if logic is purely signal based?
    // We needed member for key press sync.
    zoomSizeSlider = sizeSlider;
    zoomMagSlider = magSlider;
    
    QLabel *sizeLabel = new QLabel("Size: 250px");
    QLabel *magLabel = new QLabel("Mag: 2x");
    
    connect(sizeSlider, &QSlider::valueChanged, this, [this, sizeLabel](int val){
        sizeLabel->setText(QString("Size: %1px").arg(val));
        presentationDisplay->setZoomSettings(zoomMagSlider->value(), val);
        // Force focus back to main window so keys work? Sliders steal focus?
        // setFocus();
        presentationDisplay->setFocus(); // Actually we want MainWindow to keep focus
    });
    
    connect(magSlider, &QSlider::valueChanged, this, [this, magLabel](int val){
        magLabel->setText(QString("Mag: %1x").arg(val));
        presentationDisplay->setZoomSettings(val, zoomSizeSlider->value());
    });

    zoomBar->addWidget(zoomCheckBox);
    zoomBar->addSpacing(15);
    zoomBar->addWidget(new QLabel("Dia:"));
    zoomBar->addWidget(sizeSlider);
    zoomBar->addWidget(sizeLabel);
    zoomBar->addSpacing(15);
    zoomBar->addWidget(new QLabel("Zoom:"));
    zoomBar->addWidget(magSlider);
    zoomBar->addWidget(magLabel);
    zoomBar->addStretch();

    mainLayout->addLayout(topBar);
    mainLayout->addLayout(zoomBar);
    
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
    
    // Help Text
    QLabel *helpLabel = new QLabel(
        "<b>Hotkeys:</b><br>"
        "Right/Space: Next Slide<br>"
        "Left/Back: Prev Slide<br>"
        "Home/End: First/Last Slide<br>"
        "S: Toggle Split Mode (Notes)<br>"
        "L: Laser | Z: Zoom<br>"
        "Q/Esc: Quit"
    );
    helpLabel->setStyleSheet("color: #666; margin-top: 10px;");
    rightLayout->addWidget(helpLabel);
    rightLayout->addStretch();

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
    currentPage = 0;
    pdf->load(filePath);
    // UI update handled by statusChanged signal
}

void MainWindow::updateViews()
{
    if (pdf->status() != QPdfDocument::Status::Ready) return;

    // 0. Render Logic (first)
    QImage audienceImg, notesImg;
    {
        // Calculate target size for the Current Slide preview
        QSize targetSize = currentSlideView->size() * currentSlideView->devicePixelRatio();
        if (targetSize.isEmpty()) {
             targetSize = QSize(400, 300) * currentSlideView->devicePixelRatio(); // Fallback
        }

        QSizeF pageSize = pdf->pagePointSize(currentPage);
        
        QSize renderSize(100, 100); // Default safe size
        if (!pageSize.isEmpty()) {
            if (useSplitView) {
                // In split view, the slide is the left half.
                QSizeF slideSize(pageSize.width() / 2.0, pageSize.height());
                
                // Scale factor to fit the half-slide into targetSize
                QSize scaledHalf = slideSize.scaled(targetSize, Qt::KeepAspectRatio).toSize();
                qreal scale = (slideSize.width() > 0) ? ((qreal)scaledHalf.width() / slideSize.width()) : 1.0;
                
                renderSize = QSize(pageSize.width() * scale, pageSize.height() * scale);
            } else {
                renderSize = pageSize.scaled(targetSize, Qt::KeepAspectRatio).toSize();
            }
        }
        
        // Ensure valid render size
        if (renderSize.isEmpty()) renderSize = QSize(100, 100);

        QImage currentFull = pdf->render(currentPage, renderSize);

        if (useSplitView) {
            int w = currentFull.width() / 2;
            int h = currentFull.height();
            if (w > 0 && h > 0) {
                audienceImg = currentFull.copy(0, 0, w, h);
                notesImg = currentFull.copy(w, 0, w, h);
            }
        } else {
            audienceImg = currentFull;
        }
    }

    // 1. Update Notes (Mockup)
    if (useSplitView) {
        notesView->hide();
        notesImageView->show();
        notesImageView->setPixmap(QPixmap::fromImage(notesImg).scaled(notesImageView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        notesImageView->hide();
        notesView->show();
        notesView->setText(QString("Notes for Slide %1").arg(currentPage + 1));
    }

    // 2. Update Audience Display (Metadata only)
    presentationDisplay->setSplitMode(useSplitView);
    presentationDisplay->setPage(currentPage);

    // 3. Update Console View

    currentSlideView->setPixmap(QPixmap::fromImage(audienceImg).scaled(currentSlideView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // presentationDisplay->updateSlide(audienceImg); // REMOVED

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
        case Qt::Key_Home:
            if (currentPage != 0) {
                currentPage = 0;
                updateViews();
            }
            break;
        case Qt::Key_End:
            if (currentPage != pdf->pageCount() - 1) {
                currentPage = pdf->pageCount() - 1;
                updateViews();
            }
            break;
        case Qt::Key_L:
            laserCheckBox->setChecked(!laserCheckBox->isChecked());
            // Signal handler will update showLaser and call enableLaserPointer
             event->accept();
            break;
        case Qt::Key_Z:
            zoomCheckBox->setChecked(!zoomCheckBox->isChecked());
             event->accept();
            break;
        case Qt::Key_S:
            toggleSplitView();
            break;
        case Qt::Key_Q:
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

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == presentationDisplay && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        keyPressEvent(keyEvent); // Forward key event to main window logic
        return true; // Mark handled
    }
    return QMainWindow::eventFilter(watched, event);
}
