#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QLabel>
#include <QTreeView>
#include <QPdfBookmarkModel>
#include <QTextEdit>
#include <QSplitter>
#include <QImage>
#include "presentationdisplay.h"

#include <QTime>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onBookmarkActivated(const QModelIndex &index);
    void updateTimers();
    void toggleSplitView();

private:
    void loadPdf(const QString &filePath);
    void setupUi();
    void updateViews();
    void detectScreens();

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
    
    // Audience Window
    PresentationDisplay *presentationDisplay;
};

#endif // MAINWINDOW_H
