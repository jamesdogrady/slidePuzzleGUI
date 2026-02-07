// textoutputdialog.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QApplication>
#include "slidePuzzle.h"
#include "spdlog/spdlog.h"
// header for a QT GUI implemenation of the slide Puzzle.
// There's an initial window for looking for the puzzle PNG file.
// When the puzzle is solved, the data is written to a window that pops up.
// When that window is closed, the GUI exits.
// the work of solving the puzzle is done on it's own thread, which allows for a cancel button in case things
// take too long.





#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QCheckBox;

class PuzzleWorker : public QObject
{
    Q_OBJECT
public:
    // Pass only the data needed, not the UI widgets
    explicit PuzzleWorker(const QString &fileName, bool verbose, bool debug, QObject *parent = nullptr)
        : QObject(parent), m_fileName(fileName), m_verbose(verbose), m_debug(debug) {}

public slots:
    void process(); // The heavy lifting happens here

signals:
    void finished(const QString &result);
    void error(const QString &message);

private:
    QString m_fileName;
    bool m_verbose;
    bool m_debug;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onBrowse();
    void onSubmit();

private:
    void handleError(const QString &) ;
    QLineEdit  *fileEdit;
    QPushButton *browseButton;
    QPushButton *submitButton;
    QCheckBox *verboseCheck;
    QCheckBox *debugCheck;
    void showSolution(SlidePuzzle *);
};

class QTextEdit;

class TextOutputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextOutputDialog(QWidget *parent = nullptr);

    void setText(const QString &text);

private:
    QTextEdit *textEdit;
};


#endif // MAINWINDOW_H
