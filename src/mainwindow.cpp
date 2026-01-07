#include "mainwindow.h"
#include <QFileDialog>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QWindow>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentPage(0), showLaser(false), useSplitView(false), timerRunning(false)
{
    pdf = new QPdfDocument(this);
    bookmarkModel = new QPdfBookmarkModel(this);
    bookmarkModel->setDocument(pdf);
    presentationDisplay = new PresentationDisplay(nullptr); // Null parent = separate window
    presentationDisplay->setDocument(pdf);
    // presentationDisplay->installEventFilter(this); // REMOVED: Capture keys from audience window

    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateTimers);
    clockTimer->start(1000);

    resizeTimer = new QTimer(this);
    resizeTimer->setSingleShot(true);
    connect(resizeTimer, &QTimer::timeout, this, &MainWindow::updateViews);

    setupUi();

    // Setup shortcuts for both windows
    setupShortcuts(this);
    setupShortcuts(presentationDisplay);

    detectScreens();

    if (windowHandle()) {
         connect(windowHandle(), &QWindow::screenChanged, this, &MainWindow::updateScreenControls);
    }

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

    loadSettings();
}

MainWindow::~MainWindow()
{
    if (presentationDisplay) {
        presentationDisplay->close();
        delete presentationDisplay;
    }
}

void MainWindow::setupShortcuts(QWidget *target)
{
    // Navigation
    new QShortcut(QKeySequence(Qt::Key_Right), target, this, &MainWindow::nextSlide);
    new QShortcut(QKeySequence(Qt::Key_Down), target, this, &MainWindow::nextSlide);
    new QShortcut(QKeySequence(Qt::Key_Space), target, this, &MainWindow::nextSlide);

    new QShortcut(QKeySequence(Qt::Key_Left), target, this, &MainWindow::prevSlide);
    new QShortcut(QKeySequence(Qt::Key_Up), target, this, &MainWindow::prevSlide);
    new QShortcut(QKeySequence(Qt::Key_Backspace), target, this, &MainWindow::prevSlide);

    new QShortcut(QKeySequence(Qt::Key_Home), target, this, &MainWindow::firstSlide);
    new QShortcut(QKeySequence(Qt::Key_End), target, this, &MainWindow::lastSlide);

    // Tools
    new QShortcut(QKeySequence(Qt::Key_L), target, this, &MainWindow::toggleLaser);
    new QShortcut(QKeySequence(Qt::Key_Z), target, this, &MainWindow::toggleZoom);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), target, this, &MainWindow::toggleSplitView);

    // Screen Management
    new QShortcut(QKeySequence(Qt::Key_S), target, this, &MainWindow::switchScreens);

    // System
    new QShortcut(QKeySequence(Qt::Key_Q), target, this, &MainWindow::quitApp);
    new QShortcut(QKeySequence(Qt::Key_Escape), target, this, &MainWindow::quitApp);
}

// Slots for Actions
void MainWindow::nextSlide()
{
    if (currentPage < pdf->pageCount() - 1) {
        currentPage++;
        updateViews();
    }
}

void MainWindow::prevSlide()
{
    if (currentPage > 0) {
        currentPage--;
        updateViews();
    }
}

void MainWindow::firstSlide()
{
    if (currentPage != 0) {
        currentPage = 0;
        updateViews();
    }
}

void MainWindow::lastSlide()
{
    if (currentPage != pdf->pageCount() - 1) {
        currentPage = pdf->pageCount() - 1;
        updateViews();
    }
}

void MainWindow::toggleLaser()
{
    laserCheckBox->setChecked(!laserCheckBox->isChecked());
}

void MainWindow::toggleZoom()
{
    zoomCheckBox->setChecked(!zoomCheckBox->isChecked());
}

void MainWindow::quitApp()
{
    close();
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

    // Zoom Controls Bar
    QHBoxLayout *zoomBar = new QHBoxLayout();
    zoomCheckBox = new QCheckBox("Zoom (Z)");
    connect(zoomCheckBox, &QCheckBox::toggled, this, [this](bool checked){
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

    zoomSizeSlider = sizeSlider;
    zoomMagSlider = magSlider;

    QLabel *sizeLabel = new QLabel("Size: 250px");
    QLabel *magLabel = new QLabel("Mag: 2x");

    connect(sizeSlider, &QSlider::valueChanged, this, [this, sizeLabel](int val){
        sizeLabel->setText(QString("Size: %1px").arg(val));
        presentationDisplay->setZoomSettings(zoomMagSlider->value(), val);
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
        "S: Switch Screens<br>"
        "Ctrl+S: Toggle Split Mode<br>"
        "L: Laser | Z: Zoom<br>"
        "Q/Esc: Quit"
    );
    helpLabel->setStyleSheet("color: #666; margin-top: 10px;");
    // rightLayout->addWidget(helpLabel); // Moved to be below splitter
    // rightLayout->addStretch();

    // Assemble Splitters
    mainSplitter = new QSplitter(Qt::Horizontal);
    rightSplitter = new QSplitter(Qt::Vertical); // Declare vertical splitter

    // Style Splitters
    QString splitterStyle =
        "QSplitter::handle { background-color: #dcdcdc; border: 1px solid #ccc; }"
        "QSplitter::handle:horizontal { width: 6px; }"
        "QSplitter::handle:vertical { height: 6px; }";
    mainSplitter->setStyleSheet(splitterStyle);
    rightSplitter->setStyleSheet(splitterStyle);

    connect(mainSplitter, &QSplitter::splitterMoved, this, [this](int, int){ resizeTimer->start(50); });
    connect(rightSplitter, &QSplitter::splitterMoved, this, [this](int, int){ resizeTimer->start(50); });

    // Group 1: Next Slide
    QWidget *nextSlideGroup = new QWidget();
    QVBoxLayout *nextLayout = new QVBoxLayout(nextSlideGroup);
    nextLayout->setContentsMargins(0,0,0,0);
    nextLayout->addWidget(new QLabel("Next Slide:"));
    nextLayout->addWidget(nextSlideView);

    // Group 2: Notes
    QWidget *notesGroup = new QWidget();
    QVBoxLayout *notesLayout = new QVBoxLayout(notesGroup);
    notesLayout->setContentsMargins(0,0,0,0);
    notesLayout->addWidget(new QLabel("Notes:"));
    notesLayout->addWidget(notesView);
    notesLayout->addWidget(notesImageView);

    rightSplitter->addWidget(nextSlideGroup);
    rightSplitter->addWidget(notesGroup);
    rightSplitter->setStretchFactor(1, 1);

    // Add splitter and help text to right container
    rightLayout->addWidget(rightSplitter);
    rightLayout->addWidget(helpLabel);

    // Left: TOC with Title
    QWidget *leftContainer = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *chaptersLabel = new QLabel("Chapters");
    chaptersLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    leftLayout->addWidget(chaptersLabel);
    leftLayout->addWidget(tocView);
    mainSplitter->addWidget(leftContainer);

    // Center: Slide with Title
    QWidget *centerContainer = new QWidget();
    QVBoxLayout *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *currentSlideLabel = new QLabel("Current Slide");
    currentSlideLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    centerLayout->addWidget(currentSlideLabel);
    centerLayout->addWidget(currentSlideView);
    mainSplitter->addWidget(centerContainer);

    mainSplitter->addWidget(rightContainer);

    // Initial sizes
    // tocView->setFixedWidth(200); // FIXED: Removed fixed width constraint
    tocView->setHeaderHidden(true);

    mainSplitter->setStretchFactor(0, 0); // TOC: Minimal initial stretch but not fixed
    mainSplitter->setStretchFactor(1, 4); // Slide: Main focus
    mainSplitter->setStretchFactor(2, 1); // Right Panel

    mainLayout->addWidget(mainSplitter);

    // Controls Layout (Bottom)
    QVBoxLayout *controlsLayout = new QVBoxLayout();
    controlsLayout->addLayout(topBar);
    controlsLayout->addLayout(zoomBar);

    // Screen Management Controls
    QHBoxLayout *screenLayout = new QHBoxLayout();

    switchScreenButton = new QPushButton("Switch Screens (S)");
    connect(switchScreenButton, &QPushButton::clicked, this, &MainWindow::switchScreens);
    switchScreenButton->hide(); // Hidden by default

    screenSelector = new ScreenSelectorWidget(this);
    connect(screenSelector, &ScreenSelectorWidget::audienceScreenChanged, this, &MainWindow::onAudienceScreenSelected);
    connect(screenSelector, &ScreenSelectorWidget::consoleScreenChanged, this, &MainWindow::onConsoleScreenSelected);
    screenSelector->hide(); // Hidden by default

    QLabel *screenHelpLabel = new QLabel("<b>Monitor Manager:</b> Drag the 'A' (Audience) icon to move the presentation screen. 'C' shows your Console location.");
    screenHelpLabel->setWordWrap(true);
    screenHelpLabel->setStyleSheet("QLabel { color: #555; margin-bottom: 5px; }");
    
    screenLayout->addWidget(screenHelpLabel);
    screenLayout->addWidget(switchScreenButton);
    screenLayout->addWidget(screenSelector);
    screenLayout->addStretch();

    screenLayout->addWidget(screenSelector);
    screenLayout->addStretch();
    
    controlsLayout->addLayout(screenLayout);

    // Window Mode Controls
    QHBoxLayout *windowModeLayout = new QHBoxLayout();
    
    consoleFullscreenCheck = new QCheckBox("Console Fullscreen");
    connect(consoleFullscreenCheck, &QCheckBox::toggled, this, &MainWindow::toggleConsoleFullscreen);
    
    audienceFullscreenCheck = new QCheckBox("Audience Fullscreen");
    connect(audienceFullscreenCheck, &QCheckBox::toggled, this, &MainWindow::toggleAudienceFullscreen);
    
    aspectRatioCheck = new QCheckBox("Lock Aspect Ratio");
    connect(aspectRatioCheck, &QCheckBox::toggled, this, &MainWindow::toggleAspectRatioLock);
    
    windowModeLayout->addWidget(consoleFullscreenCheck);
    windowModeLayout->addWidget(audienceFullscreenCheck);
    windowModeLayout->addWidget(aspectRatioCheck);
    windowModeLayout->addStretch();
    
    controlsLayout->addLayout(windowModeLayout);

    mainLayout->addLayout(controlsLayout);

    setCentralWidget(centralWidget);

    setWindowTitle("Presenter Console");
    resize(1200, 800);
}

void MainWindow::detectScreens()
{
    updateScreenControls();

    // Connect to signal for future changes
    connect(qApp, &QGuiApplication::screenAdded, this, &MainWindow::onScreenCountChanged);
    connect(qApp, &QGuiApplication::screenRemoved, this, &MainWindow::onScreenCountChanged);

    // Initial positioning
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.size() > 1) {
        // Move Presentation Window to the second screen by default
        onAudienceScreenSelected(1);
    } else {
        // Single screen mode
        presentationDisplay->resize(800, 600);
        if (audienceFullscreenCheck->isChecked()) {
            presentationDisplay->showFullScreen();
        } else {
            presentationDisplay->show();
        }
    }
}

void MainWindow::onScreenCountChanged()
{
    updateScreenControls();
    screenSelector->refreshScreens();
}

void MainWindow::updateScreenControls()
{
    int count = QGuiApplication::screens().count();

    // Update Console Position in Selector
    if (windowHandle()) {
        QList<QScreen*> screens = QGuiApplication::screens();
        int idx = screens.indexOf(windowHandle()->screen());
        screenSelector->setConsoleScreen(idx);
    }

    if (count <= 1) {
        switchScreenButton->hide();
        screenSelector->hide();
    } else if (count == 2) {
        switchScreenButton->show();
        screenSelector->hide();
    } else {
        switchScreenButton->hide();
        screenSelector->show();
    }
}

void MainWindow::switchScreens()
{
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.count() != 2) return; // Only for 2-monitor mode or explicit button

    // Find current screen of presentation window
    QScreen *current = presentationDisplay->screen();
    int idx = screens.indexOf(current);

    // Swap Audience
    int nextIdx = (idx == 0) ? 1 : 0;

    // Collision check handled in onAudienceScreenSelected
    onAudienceScreenSelected(nextIdx);
}

void MainWindow::onAudienceScreenSelected(int index)
{
    QList<QScreen*> screens = QGuiApplication::screens();
    if (index >= 0 && index < screens.size()) {

        // Check Collision with Console Window
        if (windowHandle() && screens.count() >= 2) {
            QScreen *consoleScreen = windowHandle()->screen();
            int consoleIdx = screens.indexOf(consoleScreen);

            if (index == consoleIdx) {
                // Collision! Swap Console to the *other* available screen.
                // If it was previously at 'oldAudienceIdx', go there.
                // Or just find the first non-colliding screen.

                int targetConsoleIdx = -1;
                // Try to swap with where Audience WAS?
                QScreen *oldAudienceScreen = presentationDisplay->screen();
                int oldAudienceIdx = screens.indexOf(oldAudienceScreen);

                if (oldAudienceIdx != index && oldAudienceIdx != -1) {
                    targetConsoleIdx = oldAudienceIdx;
                } else {
                     // Fallback: Pick any screen that isn't 'index'
                     for (int i=0; i<screens.size(); ++i) {
                         if (i != index) {
                             targetConsoleIdx = i;
                             break;
                         }
                     }
                }

                if (targetConsoleIdx != -1) {
                    // Move Console
                    QScreen *targetConsoleInfo = screens[targetConsoleIdx];
                    windowHandle()->setScreen(targetConsoleInfo);

                    // Move window physically
                    QRect geo = targetConsoleInfo->availableGeometry();
                    // Center it or maximize? Inherit state?
                    // Let's just move it to center for safety, user can maximize
                    setGeometry(geo.x() + 50, geo.y() + 50, 1200, 800);

                    // Update selector
                    screenSelector->setConsoleScreen(targetConsoleIdx);
                }
            }
        }

        QScreen *target = screens[index];
        QRect geo = target->geometry();

        // Ensure handle is created if not already
        if (!presentationDisplay->isVisible()) presentationDisplay->show();

        if (presentationDisplay->windowHandle()) {
            presentationDisplay->windowHandle()->setScreen(target);
        }
        presentationDisplay->setGeometry(geo);
        presentationDisplay->showFullScreen();

        // Update selector state if visible
        screenSelector->setAudienceScreen(index);
    }
}

void MainWindow::onConsoleScreenSelected(int index)
{
    QList<QScreen*> screens = QGuiApplication::screens();
    if (index < 0 || index >= screens.size()) return;

    if (!windowHandle()) {
        createWinId();
    }

    // Move Console Window
    windowHandle()->setScreen(screens[index]);
    
    // Center logic
    QRect screenGeo = screens[index]->availableGeometry();
    QRect windowGeo = geometry();
    windowGeo.moveCenter(screenGeo.center());
    setGeometry(windowGeo);
    
    // Ensure it respects fullscreen
    if (consoleFullscreenCheck->isChecked()) {
        showFullScreen();
    } else {
        showNormal();
    }
    
    // Update Selector
    screenSelector->setConsoleScreen(index);
}

void MainWindow::syncTocWithPage(int page)
{
    if (!bookmarkModel) return;

    QModelIndex bestMatch;
    int bestPage = -1;

    // Helper lambda for recursive search
    std::function<void(const QModelIndex&)> search =
        [&](const QModelIndex &parent) {
        int rowCount = bookmarkModel->rowCount(parent);
        for (int i = 0; i < rowCount; ++i) {
            QModelIndex idx = bookmarkModel->index(i, 0, parent);
            int pageNum = idx.data((int)QPdfBookmarkModel::Role::Page).toInt();

            if (pageNum <= page) {
                if (pageNum > bestPage) {
                    bestPage = pageNum;
                    bestMatch = idx;
                }
            }

            if (bookmarkModel->hasChildren(idx)) {
                search(idx);
            }
        }
    };

    search(QModelIndex());

    if (bestMatch.isValid()) {
        tocView->setCurrentIndex(bestMatch);
        tocView->scrollTo(bestMatch);
    } else {
        tocView->clearSelection();
    }
}

void MainWindow::loadPdf(const QString &filePath)
{
    currentPage = 0;
    pdf->load(filePath);
    
    QFileInfo fi(filePath);
    if (presentationDisplay) {
        presentationDisplay->setWindowTitle("Audience Window - " + fi.fileName());
    }
    
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

    syncTocWithPage(currentPage);
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

void MainWindow::toggleConsoleFullscreen(bool enabled)
{
    if (enabled) showFullScreen();
    else showNormal();
}

void MainWindow::toggleAudienceFullscreen(bool enabled)
{
    if (presentationDisplay) {
        if (enabled) {
            presentationDisplay->showFullScreen();
        } else {
            presentationDisplay->showNormal();
            // Force a small resize or activate to ensure window manager updates decorations
            presentationDisplay->activateWindow();
        }
    }
}

void MainWindow::toggleAspectRatioLock(bool enabled)
{
    if (presentationDisplay) {
        presentationDisplay->setAspectRatioLock(enabled);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    resizeTimer->start(50);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    presentationDisplay->close();
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings(".my_presenter_config.ini", QSettings::IniFormat);

    restoreGeometry(settings.value("window/geometry").toByteArray());

    if (settings.contains("ui/mainSplitter")) {
        mainSplitter->restoreState(settings.value("ui/mainSplitter").toByteArray());
    }

    if (settings.contains("ui/rightSplitter")) {
        rightSplitter->restoreState(settings.value("ui/rightSplitter").toByteArray());
    }

    if (settings.contains("features/laser")) {
        showLaser = settings.value("features/laser").toBool();
        laserCheckBox->setChecked(showLaser);
    }

    if (settings.contains("features/zoom")) {
        bool zoomEnabled = settings.value("features/zoom").toBool();
        zoomCheckBox->setChecked(zoomEnabled);
    }

    if (settings.contains("features/zoomSize")) {
        int size = settings.value("features/zoomSize").toInt();
        zoomSizeSlider->setValue(size);
    }

    if (settings.contains("features/zoomMag")) {
        int mag = settings.value("features/zoomMag").toInt();
        zoomMagSlider->setValue(mag);
    }

    if (settings.contains("window/consoleFullscreen")) {
        bool full = settings.value("window/consoleFullscreen").toBool();
        consoleFullscreenCheck->setChecked(full);
        // Signal connection handles showFullScreen()
    }

    if (settings.contains("window/audienceFullscreen")) {
        bool full = settings.value("window/audienceFullscreen").toBool();
        audienceFullscreenCheck->setChecked(full);
    }
    
    if (settings.contains("window/aspectRatioLock")) {
        bool locked = settings.value("window/aspectRatioLock").toBool();
        aspectRatioCheck->setChecked(locked);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings(".my_presenter_config.ini", QSettings::IniFormat);

    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("ui/mainSplitter", mainSplitter->saveState());
    settings.setValue("ui/rightSplitter", rightSplitter->saveState());

    settings.setValue("features/laser", showLaser);
    settings.setValue("features/zoom", zoomCheckBox->isChecked());
    settings.setValue("features/zoomSize", zoomSizeSlider->value());
    settings.setValue("features/zoomMag", zoomMagSlider->value());
    
    settings.setValue("window/consoleFullscreen", consoleFullscreenCheck->isChecked());
    settings.setValue("window/audienceFullscreen", audienceFullscreenCheck->isChecked());
    settings.setValue("window/aspectRatioLock", aspectRatioCheck->isChecked());
}
