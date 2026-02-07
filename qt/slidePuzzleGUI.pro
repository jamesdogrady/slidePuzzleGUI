TEMPLATE = app          # build an application
CONFIG  += c++17 qt warn_on
QT      += widgets      # use Qt Widgets module
INCLUDEPATH=/opt/homebrew/include
INCLUDEPATH += "../"
CFLAGS += "-g -fsanitize=address -fno-omit-frame-pointer"

SOURCES += \
    ../src/slidePuzzleGUI.C \
    ../src/slidePuzzle.C \
    ../src/puzzlePicture.C

HEADERS += \
    ../src/slidePuzzleGUI.h  \
    ../src/slidePuzzle.h \
    ../src/puzzlePicture.h

LIBS += -L "/opt/homebrew/lib"
LIBS +=  -ltesseract
LIBS += -lleptonica
LIBS+= -lpng
LIBS+= -lfmt

