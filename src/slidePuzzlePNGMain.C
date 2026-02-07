/* Command line tool to solve a slidePuzzle designated by a PNG image, presumably a screen snapshot
 * The image is assumed to be from micrsoft rewards.
*/
#include "slidePuzzle.h"

#include "puzzlePicture.h"
#include <unistd.h>
#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
bool Debug=false;
bool verbose=false;
// init spdlog with local file.
// This is hard coded which is bad.
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

int main(int argc,char **argv)  {
	const char *optStr="dvc:";
	int yDim,xDim;
	int opt;
	char * progName = argv[0];
	std::string configStr="";
	init_log();

	while ((opt=getopt(argc,argv,optStr)) != -1 )  {
		switch(opt) {
		case 'd':
			Debug=true;
			break;
		case 'v':
			verbose =true;
			break;
		case 'c':
			configStr = optarg;
			break;
		}
	}
	// we know the microsoft rewards puzzles are 3x3.
	SlidePuzzle *puzzle = new SlidePuzzle(3,3);
	char *fileName = argv[optind];
	// create the object from the file nam,e
	PuzzlePicture *picture = new PuzzlePicture(fileName);
	// if this isn't a picture, fail.
	if ( picture->getHeight() == 0 )  {
		printf("Failed to oopen file %s\n",fileName);
		exit(1);
	}
	// the OCR class.
	PuzzleNumber *puzzleNumber;

	if ( Debug ) {
		picture->setDebug(1,verbose);
		puzzle->setDebug(1,verbose);
		spdlog::set_level(spdlog::level::debug);
	}
	// look in the picture for the puzzle.
	if (picture->searchForPuzzle() ) {
		puzzleNumber = new PuzzleNumber(picture);
	} else {
		printf("Did not find puzzle in %s\n",fileName);
		exit(1);
	}
	if ( Debug ) {
		puzzleNumber->setDebug(Debug,verbose);
	}
	// if we find a puzle, see if we can determine the starting configuration.
	// use ocr to find numbers in the puzzle squares.  We expect 1-8 and one with no value which is the 
	// blank square.  We also need to make sure we got what we expected.
	// 
	int lastVal=8;
	std::cout << std::flush;
	// set the initial configuration of the puzzle to allow for solving later.
	// configStr can be used to test various options for restricting the part of the picture that tesseract
	// looks at.
	if ( configStr.length() != 0 ) {
		puzzleNumber->setConfig(configStr);
	}

	bool puzzleError=false;

	for ( int rowDim=0;rowDim<3;rowDim++) {
		for ( int colDim=0;colDim<3;colDim++) {
			int i = rowDim*3+colDim;

			std::cout << std::flush;

			// determine the value of square i.
			int val = puzzleNumber->getNumber(i);
			// value must be 8 or less.
			if ( val > lastVal ) {
				fprintf(stderr,"%s: ERROR: Value out of range %d\n",progName,val);
				puzzleError=true;
			} else {
				if ( val == PuzzleNumber::blank_ret) {
					val=SlidePuzzle::BLANK_VAL;
				}
				puzzle->setInitialVal(rowDim,colDim,val);
				spdlog::debug("Adding {0} ({1},{2})",val,rowDim,colDim);
			}
		}
	}
	// prints image with adjustments for OCR
	// We might have adjusted the corner of the picture to help with OCR, so look at that if debug is on.`
	if ( Debug ) {
		pixWrite("TEST.png",picture->image,IFF_PNG);
	}
	// check the puzzle.

	puzzle->checkPuzzle();

	if ( ! puzzle->valid) {
		std::string puzzleError = puzzle->getErrorString();
		std::cout << puzzleError << std::endl;
		exit(1);
	}
	spdlog::debug("Solving the puzzle");

	// finding the solution to the puzzle.
	if ( ! puzzle->explore(true, false) ) {
		// if we get here, we did not find a solution
		std::cout << progName << " No Solution Found " << fileName  << std::endl;
	} else {
		std::cout << progName << " Solution Found " << fileName << puzzle->getResults() << std::endl;
	}

}
