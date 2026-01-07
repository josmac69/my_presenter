#include "mainwindow.h"
#include "flowlayout.h"
#include <QFileDialog>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QWindow>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QStackedLayout>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentPage(0), showLaser(false), useSplitView(false), timerRunning(false), timerHasStarted(false)
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

    // Timer starts manually via button/hotkey or first slide change
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
    // Tools
    new QShortcut(QKeySequence(Qt::Key_L), target, this, &MainWindow::activateLaser);
    new QShortcut(QKeySequence(Qt::Key_N), target, this, &MainWindow::resetCursor);
    QShortcut *zoomShortcut = new QShortcut(QKeySequence(Qt::Key_Z), target);
    connect(zoomShortcut, &QShortcut::activated, this, &MainWindow::toggleZoom);

    QShortcut *timerShortcut = new QShortcut(QKeySequence(Qt::Key_P), target);
    connect(timerShortcut, &QShortcut::activated, this, &MainWindow::toggleTimer);
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
    if (!timerRunning) toggleTimer();

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
    laserCheckBox->setChecked(!laserCheckBox->isChecked());
}

void MainWindow::activateLaser()
{
    // Force switch to Laser:
    // 1. Disable Zoom if active
    if (zoomCheckBox->isChecked()) {
        zoomCheckBox->setChecked(false);
    }
    // 2. Enable Laser if not active
    if (!laserCheckBox->isChecked()) {
        laserCheckBox->setChecked(true);
    }
}

void MainWindow::resetCursor()
{
    // Switch to Normal:
    // Disable both
    if (zoomCheckBox->isChecked()) zoomCheckBox->setChecked(false);
    if (laserCheckBox->isChecked()) laserCheckBox->setChecked(false);
}

void MainWindow::toggleZoom()
{
    zoomCheckBox->setChecked(!zoomCheckBox->isChecked());
}

void MainWindow::toggleTimer()
{
    if (timerRunning) {
        // Pause
        pauseStartTime = QTime::currentTime();
        timerRunning = false;
        timerButton->setText("Start timer");
    } else {
        // Start or Resume
        if (!timerHasStarted) {
            startTime = QTime::currentTime();
            timerHasStarted = true;
        } else {
            // Resume: Shift start time forward by the paused duration
            int pauseDuration = pauseStartTime.secsTo(QTime::currentTime());
            startTime = startTime.addSecs(pauseDuration);
        }
        timerRunning = true;
         timerButton->setText("Pause timer");
    }
    updateTimers(); // Force immediate update
}

void MainWindow::quitApp()
{
    close();
}

void MainWindow::resetLayout()
{
}

void MainWindow::setupUi()
{
    // Central Widget with Fixed Layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0); // Optional: tighter look? Or default spacing. User said "divide... into 3 parts".

    // Define 3 Columns
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QVBoxLayout *middleLayout = new QVBoxLayout();
    QVBoxLayout *rightLayout = new QVBoxLayout();

    // --- LEFT COLUMN (25%) ---
    // Contains: Chapters (Top), Monitor Manager (Bottom)?
    // In original: Chapters (LeftDock), ScreenDock (LeftDock).
    // Let's put them vertically in the left column.

    // 1. Chapters
    QWidget *tocContainer = new QWidget();
    tocContainer->setStyleSheet("background-color: palette(base); color: palette(text);");
    QVBoxLayout *tocInnerLayout = new QVBoxLayout(tocContainer);
    tocInnerLayout->setContentsMargins(0, 0, 0, 0);
    tocInnerLayout->setSpacing(0);

    QPushButton *startNavBtn = new QPushButton("--- start ---");
    startNavBtn->setFlat(true);
    startNavBtn->setStyleSheet("text-align: left; padding: 5px; border: none;");
    startNavBtn->setCursor(Qt::PointingHandCursor);
    connect(startNavBtn, &QPushButton::clicked, this, &MainWindow::firstSlide);

    tocView = new QTreeView();
    tocView->setModel(bookmarkModel);
    tocView->setHeaderHidden(true);
    tocView->setFrameShape(QFrame::NoFrame);
    connect(tocView, &QTreeView::activated, this, &MainWindow::onBookmarkActivated);
    connect(tocView, &QTreeView::clicked, this, &MainWindow::onBookmarkActivated);

    QPushButton *endNavBtn = new QPushButton("--- end ---");
    endNavBtn->setFlat(true);
    endNavBtn->setStyleSheet("text-align: left; padding: 5px; border: none;");
    endNavBtn->setCursor(Qt::PointingHandCursor);
    connect(endNavBtn, &QPushButton::clicked, this, &MainWindow::lastSlide);

    tocInnerLayout->addWidget(startNavBtn);
    tocInnerLayout->addWidget(tocView);
    tocInnerLayout->addWidget(endNavBtn);

    // 2. Monitor Manager
    QWidget *screenContainer = new QWidget();
    QVBoxLayout *scrLayout = new QVBoxLayout(screenContainer);

    switchScreenButton = new QPushButton("Switch Screens (S)");
    connect(switchScreenButton, &QPushButton::clicked, this, &MainWindow::switchScreens);

    screenSelector = new ScreenSelectorWidget(this);
    connect(screenSelector, &ScreenSelectorWidget::audienceScreenChanged, this, &MainWindow::onAudienceScreenSelected);
    connect(screenSelector, &ScreenSelectorWidget::consoleScreenChanged, this, &MainWindow::onConsoleScreenSelected);

    QLabel *helpLabel = new QLabel(
        "<b>Hotkeys:</b><br>"
        "Right/Space: Next Slide<br>"
        "Left/Back: Prev Slide<br>"
        "Home/End: First/Last<br>"
        "S: Switch Screens<br>"
        "L: Laser | Z: Zoom<br>"
        "P: Timer | Q: Quit"
    );
    helpLabel->setStyleSheet("margin-top: 10px; color: #333;");
    helpLabel->setWordWrap(true);

    scrLayout->addWidget(switchScreenButton);
    scrLayout->addWidget(screenSelector);
    scrLayout->addWidget(helpLabel);
    scrLayout->addStretch();

    // Assemble Left Column
    // Add titles? Previously docks had titles.
    // "Chapters", "Monitor Manager".
    QLabel *tocTitle = new QLabel("Chapters");
    tocTitle->setStyleSheet("font-weight: bold; background: #ddd; padding: 4px;");
    QLabel *screenTitle = new QLabel("Monitor Manager");
    screenTitle->setStyleSheet("font-weight: bold; background: #ddd; padding: 4px;");

    leftLayout->addWidget(tocTitle);
    leftLayout->addWidget(tocContainer, 1); // Expand TOC
    leftLayout->addWidget(screenTitle);
    leftLayout->addWidget(screenContainer, 0); // Fixed size for screens?

    // --- MIDDLE COLUMN (50%) ---
    // Contains: Current Slide (Top), Notes (Bottom)

    // 1. Check currentSlideView
    QLabel *currentSlideTitle = new QLabel("Current Slide");
    currentSlideTitle->setStyleSheet("font-weight: bold; padding: 5px;");
    currentSlideTitle->setAlignment(Qt::AlignCenter);

    currentSlideView = new QLabel("Current Slide");
    currentSlideView->setAlignment(Qt::AlignCenter);
    currentSlideView->setStyleSheet("background: #dddddd; border: 1px solid #999;");
    currentSlideView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    currentSlideView->setMinimumSize(50, 50);
    currentSlideView->installEventFilter(this);

    // 2. Notes
    // Previously in notesDock
    QLabel *notesTitle = new QLabel("Notes"); // Was dock title
    notesTitle->setStyleSheet("font-weight: bold; background: #ddd; padding: 4px;");

    notesView = new QTextEdit();
    notesView->setPlaceholderText("Notes for this slide...");
    // notesView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); // Maybe preferred now?

    notesImageView = new QLabel("Notes View");
    notesImageView->setAlignment(Qt::AlignCenter);
    notesImageView->setStyleSheet("background: white; border: 1px solid #ccc;");
    notesImageView->hide();

    middleLayout->addWidget(currentSlideTitle);
    middleLayout->addWidget(currentSlideView, 2); // Slide takes more space
    middleLayout->addWidget(notesTitle);

    // Notes Container (Stacking text/image)
    QStackedLayout *notesStack = new QStackedLayout();
    notesStack->addWidget(notesView);
    notesStack->addWidget(notesImageView);
    QWidget *notesWidget = new QWidget();
    notesWidget->setLayout(notesStack);

    middleLayout->addWidget(notesWidget, 1); // Notes take less space than slide


    // --- RIGHT COLUMN (25%) ---
    // Contains: Next Slide (Top), Control Center (Bottom)

    // 1. Next Slide
    QLabel *nextSlideTitle = new QLabel("Next Slide");
    nextSlideTitle->setStyleSheet("font-weight: bold; padding: 5px;");
    nextSlideTitle->setAlignment(Qt::AlignCenter);

    nextSlideView = new QLabel("Next Slide");
    nextSlideView->setAlignment(Qt::AlignCenter);
    nextSlideView->setStyleSheet("background: #eeeeee; border: 1px dashed #aaa;");
    nextSlideView->setMinimumHeight(150);
    nextSlideView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // 2. Control Center
    QLabel *controlsTitle = new QLabel("Control Center");
    controlsTitle->setStyleSheet("font-weight: bold; background: #ddd; padding: 4px;");

    QWidget *centerContainer = new QWidget();
    FlowLayout *centerLayout = new FlowLayout(centerContainer);
    centerLayout->setContentsMargins(5, 5, 5, 5);

    // ... Controls Logic (Copied from original) ...
    QFrame *controlsFrame = new QFrame();
    controlsFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    controlsFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsFrame);
    controlsLayout->setContentsMargins(5, 5, 5, 5);

    QVBoxLayout *controlsLeft = new QVBoxLayout();
    consoleFullscreenCheck = new QCheckBox("Console Fullscreen");
    connect(consoleFullscreenCheck, &QCheckBox::toggled, this, &MainWindow::toggleConsoleFullscreen);
    audienceFullscreenCheck = new QCheckBox("Audience Fullscreen");
    connect(audienceFullscreenCheck, &QCheckBox::toggled, this, &MainWindow::toggleAudienceFullscreen);
    aspectRatioCheck = new QCheckBox("Lock Aspect Ratio");
    connect(aspectRatioCheck, &QCheckBox::toggled, this, &MainWindow::toggleAspectRatioLock);
    controlsLeft->addWidget(consoleFullscreenCheck);
    controlsLeft->addWidget(audienceFullscreenCheck);
    controlsLeft->addWidget(aspectRatioCheck);
    controlsLeft->addStretch();

    QVBoxLayout *controlsRight = new QVBoxLayout();

    // -- Features Section --
    // We can use a GridLayout to align everything nicely or just VBoxes.
    // Let's use a GridLayout for parameters to align labels/sliders.

    QGridLayout *featuresGrid = new QGridLayout();
    featuresGrid->setContentsMargins(0, 0, 0, 0);
    featuresGrid->setVerticalSpacing(8);

    // Row 0: Laser Checkbox
    laserCheckBox = new QCheckBox("Laser (L)");
    connect(laserCheckBox, &QCheckBox::toggled, this, [this](bool checked){ showLaser = checked; presentationDisplay->enableLaserPointer(checked); });
    featuresGrid->addWidget(laserCheckBox, 0, 0, 1, 3);

    // Row 1: Laser Param 1 (Size)
    QLabel *lSizeLbl = new QLabel("Size");
    laserSizeSlider = new QSlider(Qt::Horizontal);
    laserSizeSlider->setRange(10, 200); laserSizeSlider->setValue(60);
    QLabel *lSizeVal = new QLabel("60px");
    connect(laserSizeSlider, &QSlider::valueChanged, this, [this, lSizeVal](int val){
        lSizeVal->setText(QString("%1px").arg(val));
        presentationDisplay->setLaserSettings(val, laserOpacitySlider->value());
    });
    featuresGrid->addWidget(lSizeLbl, 1, 0);
    featuresGrid->addWidget(laserSizeSlider, 1, 1);
    featuresGrid->addWidget(lSizeVal, 1, 2);

    // Row 2: Laser Param 2 (Opacity)
    QLabel *lOpLbl = new QLabel("Alpha");
    laserOpacitySlider = new QSlider(Qt::Horizontal);
    laserOpacitySlider->setRange(20, 255); laserOpacitySlider->setValue(128);
    QLabel *lOpVal = new QLabel("128");
    connect(laserOpacitySlider, &QSlider::valueChanged, this, [this, lOpVal](int val){
        lOpVal->setText(QString::number(val));
        presentationDisplay->setLaserSettings(laserSizeSlider->value(), val);
    });
    featuresGrid->addWidget(lOpLbl, 2, 0);
    featuresGrid->addWidget(laserOpacitySlider, 2, 1);
    featuresGrid->addWidget(lOpVal, 2, 2);

    // Row 3: Spacer/Separator?
    // Just a small gap handled by spacing

    // Row 4: Zoom Checkbox
    zoomCheckBox = new QCheckBox("Zoom (Z)");
    connect(zoomCheckBox, &QCheckBox::toggled, this, [this](bool checked){ presentationDisplay->enableZoom(checked); });
    featuresGrid->addWidget(zoomCheckBox, 4, 0, 1, 3);

    // Row 5: Zoom Param 1 (Size)
    QLabel *zSizeLbl = new QLabel("Size");
    zoomSizeSlider = new QSlider(Qt::Horizontal);
    zoomSizeSlider->setRange(250, 1500); zoomSizeSlider->setValue(250);
    QLabel *zSizeVal = new QLabel("250px");

    // Row 6: Zoom Param 2 (Mag)
    QLabel *zMagLbl = new QLabel("Mag");
    zoomMagSlider = new QSlider(Qt::Horizontal);
    zoomMagSlider->setRange(2, 5); zoomMagSlider->setValue(2);
    QLabel *zMagVal = new QLabel("2x");

    connect(zoomSizeSlider, &QSlider::valueChanged, this, [this, zSizeVal](int val){
        zSizeVal->setText(QString("%1px").arg(val));
        presentationDisplay->setZoomSettings(zoomMagSlider->value(), val);
    });
    connect(zoomMagSlider, &QSlider::valueChanged, this, [this, zMagVal](int val){
        zMagVal->setText(QString("%1x").arg(val));
        presentationDisplay->setZoomSettings(val, zoomSizeSlider->value());
    });

    featuresGrid->addWidget(zSizeLbl, 5, 0);
    featuresGrid->addWidget(zoomSizeSlider, 5, 1);
    featuresGrid->addWidget(zSizeVal, 5, 2); // reuse zSizeVal pointer? logic above captures it.

    featuresGrid->addWidget(zMagLbl, 6, 0);
    featuresGrid->addWidget(zoomMagSlider, 6, 1);
    featuresGrid->addWidget(zMagVal, 6, 2);

    controlsRight->addLayout(featuresGrid);

    // Help Text
    QLabel *shortcutsHelpLabel = new QLabel("Shortcuts: L=Laser, N=Normal, Z=Zoom");
    shortcutsHelpLabel->setStyleSheet("color: #555; font-size: 10px; font-style: italic; margin-top: 5px;");
    shortcutsHelpLabel->setAlignment(Qt::AlignCenter);
    controlsRight->addWidget(shortcutsHelpLabel);

    // -- Bottom: Buttons --
    QHBoxLayout *buttonRow = new QHBoxLayout();
    closeButton = new QPushButton("Close Presenter");
    closeButton->setStyleSheet("background-color: #ffcccc; padding: 5px;");
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);

    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

    controlsRight->addStretch();
    controlsRight->addLayout(buttonRow);

    controlsLayout->addLayout(controlsLeft);
    controlsLayout->addSpacing(10);
    controlsLayout->addLayout(controlsRight);
    controlsLayout->addStretch();

    // Timer Frame
    QFrame *timerFrame = new QFrame();
    timerFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QVBoxLayout *elapsedLayout = new QVBoxLayout(timerFrame);

    QFont defaultTimerFont("Nimbus Sans", 14, QFont::Bold); defaultTimerFont.setBold(true);
    elapsedLabel = new QLabel("00:00:00");
    elapsedLabel->setFont(defaultTimerFont);
    elapsedLabel->setAlignment(Qt::AlignCenter);

    // Horizontal Controls Row
    QHBoxLayout *timerControls = new QHBoxLayout();

    timerFontSlider = new QSlider(Qt::Horizontal);
    timerFontSlider->setRange(10, 72); timerFontSlider->setValue(14);
    // timerFontSlider->setFixedWidth(100); // Let it expand slightly or fix?
    connect(timerFontSlider, &QSlider::valueChanged, this, [this](int val){ QFont f = elapsedLabel->font(); f.setPointSize(val); elapsedLabel->setFont(f); });

    timerColorButton = new QPushButton("Color");
    connect(timerColorButton, &QPushButton::clicked, this, [this](){
        QColor color = QColorDialog::getColor(Qt::black, this, "Select Timer Color");
        if (color.isValid()) { elapsedLabel->setStyleSheet(QString("color: %1").arg(color.name())); elapsedLabel->setProperty("customColor", color.name()); }
    });

    timerFontButton = new QPushButton("Font");
    connect(timerFontButton, &QPushButton::clicked, this, [this](){
        bool ok; QFont font = QFontDialog::getFont(&ok, elapsedLabel->font(), this, "Select Timer Font");
        if (ok) { timerFontSlider->setValue(font.pointSize()); elapsedLabel->setFont(font); update(); }
    });

    timerControls->addWidget(timerFontSlider);
    timerControls->addSpacing(10);
    timerControls->addWidget(timerColorButton);
    timerControls->addSpacing(10);
    timerControls->addWidget(timerFontButton);

    timerButton = new QPushButton("Start timer");
    connect(timerButton, &QPushButton::clicked, this, &MainWindow::toggleTimer);

    QHBoxLayout *timerHeader = new QHBoxLayout();
    timerHeader->addWidget(new QLabel("Timer:"));
    timerHeader->addStretch();
    timerHeader->addWidget(timerButton);

    elapsedLayout->addLayout(timerHeader);
    elapsedLayout->addWidget(elapsedLabel);
    elapsedLayout->addLayout(timerControls); // Use horizontal layout
    elapsedLayout->addStretch();

    // Clock Frame
    QFrame *clockFrame = new QFrame();
    clockFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QVBoxLayout *clockLayout = new QVBoxLayout(clockFrame);

    timeLabel = new QLabel("00:00:00");
    timeLabel->setFont(defaultTimerFont);
    timeLabel->setAlignment(Qt::AlignCenter);

    // Horizontal Controls Row
    QHBoxLayout *clockControls = new QHBoxLayout();

    clockFontSlider = new QSlider(Qt::Horizontal);
    clockFontSlider->setRange(10, 72); clockFontSlider->setValue(14);
    connect(clockFontSlider, &QSlider::valueChanged, this, [this](int val){ QFont f = timeLabel->font(); f.setPointSize(val); timeLabel->setFont(f); });

    clockColorButton = new QPushButton("Color");
    connect(clockColorButton, &QPushButton::clicked, this, [this](){
        QColor color = QColorDialog::getColor(Qt::black, this, "Select Clock Color");
        if (color.isValid()) { timeLabel->setStyleSheet(QString("color: %1").arg(color.name())); timeLabel->setProperty("customColor", color.name()); }
    });

    clockFontButton = new QPushButton("Font");
    connect(clockFontButton, &QPushButton::clicked, this, [this](){
        bool ok; QFont font = QFontDialog::getFont(&ok, timeLabel->font(), this, "Select Clock Font");
        if (ok) { clockFontSlider->setValue(font.pointSize()); timeLabel->setFont(font); update(); }
    });

    clockControls->addWidget(clockFontSlider);
    clockControls->addSpacing(10);
    clockControls->addWidget(clockColorButton);
    clockControls->addSpacing(10);
    clockControls->addWidget(clockFontButton);

    clockLayout->addWidget(new QLabel("Current Time:"));
    clockLayout->addWidget(timeLabel);
    clockLayout->addLayout(clockControls);
    clockLayout->addStretch();

    // Add to FlowLayout
    centerLayout->addWidget(controlsFrame);
    centerLayout->addWidget(timerFrame);
    centerLayout->addWidget(clockFrame);

    // Add to Right
    rightLayout->addWidget(nextSlideTitle);
    rightLayout->addWidget(nextSlideView, 2);
    rightLayout->addWidget(controlsTitle);
    rightLayout->addWidget(centerContainer, 1);


    // --- COMPOSE MAIN ---
    // Stretches: 1 (25%), 2 (50%), 1 (25%) -> Total 4 parts.
    // 1/4 = 25%, 2/4 = 50%, 1/4 = 25%.

    // Wrap layouts in widgets or add directly as layouts?
    // QHBoxLayout::addLayout allows adding layouts.

    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(middleLayout, 2);
    mainLayout->addLayout(rightLayout, 1);

    setWindowTitle("Presenter Console");
    resize(1200, 800);

    // defaultState = saveState(); // Removed
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

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == currentSlideView && event->type() == QEvent::Resize) {
        // Trigger generic update when the view itself is resized
        // Use a small delay/debounce if needed, or just update directly if lightweight
        if (!resizeTimer->isActive()) {
            resizeTimer->start(50); // Debounce slightly
        }
    }
    return QMainWindow::eventFilter(obj, event);
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

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore(); // Do nothing on Escape
        return;
    }
    if (event->key() == Qt::Key_Q) {
        quitApp();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings(".my_presenter_config.ini", QSettings::IniFormat);

    restoreGeometry(settings.value("window/geometry").toByteArray());
    // restoreState removed

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

    if (settings.contains("font/clockSize")) {
        int sz = settings.value("font/clockSize").toInt();
        if (sz < 10) sz = 14;
        clockFontSlider->setValue(sz);
    }
    if (settings.contains("font/timerSize")) {
        int sz = settings.value("font/timerSize").toInt();
        if (sz < 10) sz = 14;
        timerFontSlider->setValue(sz);
    }

    if (settings.contains("features/laserSize")) {
        int size = settings.value("features/laserSize").toInt();
        laserSizeSlider->setValue(size);
    }
    if (settings.contains("features/laserOpacity")) {
        int opacity = settings.value("features/laserOpacity").toInt();
        laserOpacitySlider->setValue(opacity);
    }

    // Load Fonts (Family/Style)
    if (settings.contains("font/clockFont")) {
         QFont f;
         if (f.fromString(settings.value("font/clockFont").toString())) {
             // Enforce size from slider to avoid 0-size errors or conflicts
             f.setPointSize(clockFontSlider->value());
             timeLabel->setFont(f);
         }
    }
    if (settings.contains("font/timerFont")) {
         QFont f;
         if (f.fromString(settings.value("font/timerFont").toString())) {
             // Enforce size from slider
             f.setPointSize(timerFontSlider->value());
             elapsedLabel->setFont(f);
         }
    }

    if (settings.contains("font/clockColor")) {
        QString colorName = settings.value("font/clockColor").toString();
        timeLabel->setStyleSheet(QString("color: %1").arg(colorName));
        timeLabel->setProperty("customColor", colorName);
    }
    if (settings.contains("font/timerColor")) {
        QString colorName = settings.value("font/timerColor").toString();
        elapsedLabel->setStyleSheet(QString("color: %1").arg(colorName));
        elapsedLabel->setProperty("customColor", colorName);
    }

    // ... rest of loadSettings
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
    // saveState removed

    settings.setValue("features/laser", showLaser);
    settings.setValue("features/zoom", zoomCheckBox->isChecked());
    settings.setValue("features/zoomSize", zoomSizeSlider->value());
    settings.setValue("features/zoomMag", zoomMagSlider->value());

    settings.setValue("features/laserSize", laserSizeSlider->value());
    settings.setValue("features/laserOpacity", laserOpacitySlider->value());

    settings.setValue("font/clockSize", clockFontSlider->value());
    settings.setValue("font/timerSize", timerFontSlider->value());

    if (timeLabel->palette().color(QPalette::WindowText).isValid())
         settings.setValue("font/clockColor", timeLabel->palette().color(QPalette::WindowText).name());

    // Note: Since we use stylesheets, we might need a stored member or parse the stylesheet to get the color back effectively if palette update isn't immediate.
    // However, setStyleSheet usually updates the widget's palette. Let's make sure we save the style sheet color or just the last picked one.
    // Better strategy: We don't have a member for color, so we rely on what was set.
    // Actually, reading back from stylesheet string is messy.
    // Let's rely on the property we just set.

    // Simpler approach: Store the color string in a dynamic property 'customColor' when setting it?
    // Or just save what we can.
    // Updated plan: The lambda sets stylesheet. We can parse it or store it.
    // Let's assume for now we just save if we have a way.
    // Actually, simpler: define members `QString clockColorStr` and `QString timerColorStr`.
    // But modification of Header again?
    // Let's use QSettings to store what we put in the stylesheet if we can't easily retrieve it.
    // Wait, I can just use `timeLabel->palette().color(QPalette::WindowText).name()`?
    // setStyleSheet does NOT always update the palette accessors immediately or reliably for saving.
    // Reliable way: Extract from stylesheet or use a property.

    // Let's use dynamic properties for storage without header change.
    settings.setValue("font/clockColor", timeLabel->property("customColor"));
    settings.setValue("font/timerColor", elapsedLabel->property("customColor"));

    settings.setValue("window/consoleFullscreen", consoleFullscreenCheck->isChecked());
    settings.setValue("window/audienceFullscreen", audienceFullscreenCheck->isChecked());
    settings.setValue("window/aspectRatioLock", aspectRatioCheck->isChecked());
}
