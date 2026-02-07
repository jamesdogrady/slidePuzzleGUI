This is a set of programs to solve the microsoft rewards slidePuzzle.
make -f slidePuzzle.mk builds command line tools in the build directory and a QT app in the qt directory.

build/slidePuzzle will solve the puzzle based on the command line paramters (./slidePuzzle -x 3 -y 3 B 1 2 3 4 5 6 7 8).
build/slidePuzzlePNG takes as a parameter a screen snapshot containing the web page with the slide puzzle and generates a solution.
The QT app does the same thing with a GUI.

Each slide puzzle you solve gets you 5 rewards points which is < .1 cent if redeemed for amazon gift cards.  So it's free money, but you only get one puzzle a week.
