#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QLabel>
#include <QTreeView>
#include <QPdfBookmarkModel>
#include <QTextEdit>
#include <QScreen>
#include <QPushButton>
#include "screenselectorwidget.h"
#include <QShortcut>
#include "presentationdisplay.h"
#include <QCheckBox>
#include <QSlider>
#include <QColorDialog>
#include <QFontDialog>
#include <QScrollArea>

#include <QTime>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onBookmarkActivated(const QModelIndex &index);
    void updateTimers();
    void toggleSplitView();
    void resetLayout();

    // Hotkey Actions
    void nextSlide();
    void prevSlide();
    void firstSlide();
    void lastSlide();
    void toggleLaser();
    void activateLaser(); // L: Switch to Laser (force Zoom off)
    void resetCursor();   // N: Switch to Normal (Laser/Zoom off)
    void toggleZoom();
    void toggleTimer();
    void quitApp();
    
    // Screen Management
    void switchScreens();
    void onScreenCountChanged();
    void onAudienceScreenSelected(int index);
    void onConsoleScreenSelected(int index);

private:
    void loadPdf(const QString &filePath);
    void setupUi();
    void updateViews();
    void detectScreens();
    void syncTocWithPage(int page);
    void setupShortcuts(QWidget *target);
    void updateScreenControls();
    void loadSettings();
    void saveSettings();
    
    // Window Mode Slots
    void toggleConsoleFullscreen(bool enabled);
    void toggleAudienceFullscreen(bool enabled);
    void toggleAspectRatioLock(bool enabled);

    // Data
    QPdfDocument *pdf;
    QPdfBookmarkModel *bookmarkModel;
    int currentPage;
    bool showLaser;
    bool useSplitView;

    // Timers
    QTimer *clockTimer;
    QTimer *resizeTimer;
    QTime startTime;
    QTime pauseStartTime; // Track when pause started
    bool timerRunning;
    bool timerHasStarted; // Track if timer was ever started
    
    QPushButton *timerButton;

    // Screen Management
    QPushButton *switchScreenButton;
    ScreenSelectorWidget *screenSelector;

    // UI Elements
    // Removed Docks for fixed layout
    // QDockWidget *tocDock; ...

    QLabel *currentSlideView;
    QLabel *nextSlideView;
    QTextEdit *notesView;
    QLabel *notesImageView;
    QTreeView *tocView;
    
    QLabel *timeLabel;
    QLabel *elapsedLabel;
    QCheckBox *laserCheckBox;
    QCheckBox *zoomCheckBox;
    QSlider *zoomSizeSlider;
    QSlider *zoomMagSlider;
    QSlider *laserSizeSlider;
    QSlider *laserOpacitySlider;
    
    // Font Controls
    QPushButton *clockFontButton;
    QPushButton *timerFontButton;
    QSlider *clockFontSlider;
    QSlider *timerFontSlider;
    QPushButton *clockColorButton;
    QPushButton *timerColorButton;
    
    // Window Controls
    QCheckBox *consoleFullscreenCheck;
    QCheckBox *audienceFullscreenCheck;
    QCheckBox *aspectRatioCheck;
    QPushButton *closeButton;

    // QByteArray defaultState; // Removed for fixed layout
    
    // Audience Window
    PresentationDisplay *presentationDisplay;
};

#endif // MAINWINDOW_H
