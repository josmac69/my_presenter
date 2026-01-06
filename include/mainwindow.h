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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onBookmarkActivated(const QModelIndex &index);

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

    // UI Elements
    QSplitter *mainSplitter;
    QSplitter *rightSplitter;
    
    QLabel *currentSlideView;
    QLabel *nextSlideView;
    QTextEdit *notesView;
    QTreeView *tocView;
    
    // Audience Window
    PresentationDisplay *presentationDisplay;
};

#endif // MAINWINDOW_H
