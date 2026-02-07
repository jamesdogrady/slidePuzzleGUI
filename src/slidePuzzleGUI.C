#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QStatusBar>
#include "puzzlePicture.h"
#include "slidePuzzleGUI.h"
#include <QMessageBox>
#include <iostream>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

// init logging.
void init_log()
{
	try {
  		auto logger = spdlog::basic_logger_mt("basic_logger", "/Volumes/Mac2TBSSD/users/jimogrady/Projects/slidePuzzle/slidePuzzle-log.txt");
		spdlog::set_default_logger(logger);
		spdlog::info("Log is started");
		spdlog::error("Error message");
	} 
	catch ( const spdlog::spdlog_ex &ex)
	{
		std::cerr << "failed to initialize log" << std::endl;
	}
	spdlog::flush_on(spdlog::level::info);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Central widget and layouts
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // File selector row
    QHBoxLayout *fileLayout = new QHBoxLayout;
    fileEdit = new QLineEdit(this);
    browseButton = new QPushButton(tr("Browse..."), this);
    submitButton = new QPushButton(tr("Solve"), this);

    fileLayout->addWidget(fileEdit);
    fileLayout->addWidget(browseButton);
    mainLayout->addWidget(submitButton);

    mainLayout->addLayout(fileLayout);

    // Two binary options
    verboseCheck = new QCheckBox(tr("verbose"), this);
    debugCheck = new QCheckBox(tr("Debug"), this);

    mainLayout->addWidget(verboseCheck);
    mainLayout->addWidget(debugCheck);

    mainLayout->addStretch();

    // Connect browse button
    connect(browseButton, &QPushButton::clicked,
            this, &MainWindow::onBrowse);

    connect(submitButton, &QPushButton::clicked,
        this, &MainWindow::onSubmit);
    setWindowTitle(tr("File chooser with options"));
    resize(400, 120);
    init_log();
}

void MainWindow::onBrowse()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select a file"),
        QString(),
        tr("All files (*)")
    );  // static convenience function to open a file dialog and return a path [web:7]

    if (!fileName.isEmpty()) {
        fileEdit->setText(fileName);
    }
    spdlog::info("File is chosen");
}
// textoutputdialog.cpp
#include <QTextEdit>
#include <QVBoxLayout>

// output window.
TextOutputDialog::TextOutputDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Output"));

    textEdit = new QTextEdit(this);            // multi-line text widget [web:18]
    textEdit->setReadOnly(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(textEdit);

    resize(500, 400);
}

void TextOutputDialog::setText(const QString &text)
{
    textEdit->setPlainText(text);              // set plain text content [web:18]
}

void MainWindow::handleError(const QString &errorMessage) {
    QMessageBox::critical(this, tr("Error"), errorMessage);
}

// this is the user hitting the solve button.
void MainWindow::onSubmit()
{
    // 1. Setup the UI and Thread
    QProgressDialog *progress = new QProgressDialog("Solving Puzzle...", "Cancel", 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    
    // new thread
    QThread* thread = new QThread;
    PuzzleWorker* worker = new PuzzleWorker(fileEdit->text(), verboseCheck->isChecked(), debugCheck->isChecked());
    worker->moveToThread(thread);

    // 2. Cancellation Logic
    // When the user clicks "Cancel", tell the thread to interrupt the worker
    connect(progress, &QProgressDialog::canceled, thread, &QThread::requestInterruption);

    // 3. Execution Logic
    //  process is the big functon here.
    connect(thread, &QThread::started, worker, &PuzzleWorker::process);

    // 4. Handle Completion
    auto cleanup = [ thread, worker, progress]() {
        progress->close();
        thread->quit();
        thread->wait(); // Ensure thread is fully stopped before deletion
        worker->deleteLater();
        thread->deleteLater();
        progress->deleteLater();
    };

    connect(worker, &PuzzleWorker::finished, this, [this, cleanup](const QString &res) {
        cleanup();
        TextOutputDialog dlg(this);
        dlg.setText(res);
        dlg.exec();
    	QApplication::quit();

    });

    connect(worker, &PuzzleWorker::error, this, [this, cleanup](const QString &msg) {
        cleanup();
        this->statusBar()->showMessage(msg, 3000);
    });

    // 5. Launch
    thread->start();
    progress->show();
}

void PuzzleWorker::process()
{
    QByteArray byteArray = m_fileName.toUtf8();
    const char* fName = byteArray.constData();
    // user hit solve before selecting a file.
    if ( strlen(fName) == 0 ) {
            emit error("No file name supplied");
    }

    // create an object based on the input.
    PuzzlePicture *picture = new PuzzlePicture(fName);

    // something is wrong with the selected file.
    if (picture->getHeight() == 0) {
        emit error("Could not load the image file.");
        delete picture;
        return;
    }
    // work on the puzzle.
    SlidePuzzle *puzzle = new SlidePuzzle(3,3);
    bool verbose;
    if (m_verbose ) {
	    verbose  = true;
    }
    if (m_debug ) {
	    puzzle->setDebug(1,verbose);
	    picture->setDebug(1,verbose);
	    spdlog::set_level(spdlog::level::debug);
    }
    spdlog::info("Call to onSubmit searchForPuzzle");
    PuzzleNumber *puzzleNumber;
    // is there a puzzle in this image?
    if ( picture->searchForPuzzle()) {
	    puzzleNumber = new PuzzleNumber(picture);
    } else {
        emit error("No Puzzle Found.");
	return;
    }
    // we check for interruptions periodically
    if (QThread::currentThread()->isInterruptionRequested()) {
            emit error("Solvker was canceled by the user.");
            return; // Exit the function to stop the thread
    }
    // we did find a puzzle, so use OCR to get the values for each square
    int val;
    for ( int rowDim=0;rowDim<3;rowDim++) {
		for ( int colDim=0;colDim<3;colDim++) {
			int i = rowDim*3+colDim;
			val= puzzleNumber->getNumber(i);
			puzzle->setInitialVal(rowDim,colDim,val);
		}
    }
    if (QThread::currentThread()->isInterruptionRequested()) {
            emit error("Solver was canceled by the user.");
            return; // Exit the function to stop the thread
    }
    spdlog::info("Call to onSubmit checkPuzzle");
    // check to see if we have the puzzle inforation correct.
    puzzle->checkPuzzle();
    if ( ! puzzle->valid) {
	    std::string errStr = puzzle->getErrorString();
	    QString errQStr = QString::fromStdString(errStr);
    	    emit finished(errQStr); // Send results back to the Main Thread
	    return;
    }
    if (QThread::currentThread()->isInterruptionRequested()) {
            emit error("Solver was canceled by the user.");
            return; // Exit the function to stop the thread
    }

    // the puzzle is correct, so start working on solving it.
    bool ret;
    bool first=true;
    do {
    	spdlog::info("Call to explore");
    	ret=puzzle->explore(first,true) ;
	// subsequent calls first is not true
	first=false;
    	if (QThread::currentThread()->isInterruptionRequested()) {
            emit error("Solver was canceled by the user.");
            return; // Exit the function to stop the thread
    	}
    }
    while ( ! ret ) ;

    // we are done with the puzzle.  Did it work?
    
    std::string result = puzzle->getResults();
    QString retStr = QString::fromStdString(result);
    spdlog::debug("Results were {0}",result);
    delete puzzle;
    delete picture;

    emit finished(retStr); // Send results back to the Main Thread
}




int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}

