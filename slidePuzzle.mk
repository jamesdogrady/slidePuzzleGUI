all:	bld slidePuzzleGUI

slidePuzzleGUI: 
	cd qt;\
	qmake slidePuzzleGUI.pro;\
	make

bld:
	cd build;\
	make


