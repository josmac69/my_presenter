#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QLabel>
#include <QTreeView>
#include <QPdfBookmarkModel>
#include <QTextEdit>
#include <QSplitter>
#include <QScreen>
#include <QPushButton>
#include "screenselectorwidget.h"
#include <QShortcut>
#include "presentationdisplay.h"
#include <QCheckBox>
#include <QSlider>

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

private slots:
    void onBookmarkActivated(const QModelIndex &index);
    void updateTimers();
    void toggleSplitView();

    // Hotkey Actions
    void nextSlide();
    void prevSlide();
    void firstSlide();
    void lastSlide();
    void toggleLaser();
    void toggleZoom();
    void quitApp();
    
    // Screen Management
    void switchScreens();
    void onScreenCountChanged();
    void onAudienceScreenSelected(int index);

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

    // Data
    QPdfDocument *pdf;
    QPdfBookmarkModel *bookmarkModel;
    int currentPage;
    bool showLaser;
    bool useSplitView;

    // Timers
    QTimer *clockTimer;
    QTime startTime;
    bool timerRunning;
    QTimer *resizeTimer;

    // Screen Management
    QPushButton *switchScreenButton;
    ScreenSelectorWidget *screenSelector;

    // UI Elements
    QSplitter *mainSplitter;
    QSplitter *rightSplitter;
    
    QLabel *currentSlideView;
    QLabel *nextSlideView;
    QTextEdit *notesView;      // For text notes
    QLabel *notesImageView;    // For visual Beamer notes (split right half)
    QTreeView *tocView;
    
    QLabel *timeLabel;
    QLabel *elapsedLabel;
    QCheckBox *laserCheckBox;
    QCheckBox *zoomCheckBox;
    QSlider *zoomSizeSlider;
    QSlider *zoomMagSlider;
    
    // Audience Window
    PresentationDisplay *presentationDisplay;
};

#endif // MAINWINDOW_H
