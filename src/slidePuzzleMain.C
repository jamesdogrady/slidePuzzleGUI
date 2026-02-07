// main CLI program to solve the puzzle based on text inputs.
#include "slidePuzzle.h"
#include <unistd.h>
#include <iostream>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
bool Debug=false;
bool verbose=false;
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

// we pass in x and y directions and the initial values for the puzzle.  The blank space is "B"

int main(int argc,char **argv)  {
	const char *optStr="x:y:dv";
	int yDim,xDim;
	int opt;
	while ((opt=getopt(argc,argv,optStr)) != -1 )  {
		switch(opt) {
		case 'y':
			yDim=atoi(optarg);
			break;
		case 'x':
			xDim=atoi(optarg);
			break;
		case 'd':
			Debug=true;
			break;
		case 'v':
			verbose =true;
			break;
		}
	}
	if ( Debug ) {
		std::cout << yDim << " " << xDim << std::endl;
	}
	init_log();
	// create the slidePuzzle Object
	SlidePuzzle *puzzle = new SlidePuzzle(xDim,yDim);
	// starting puzzle Configuration.
	int i = argc;
	int rowDim=0;
	int colDim=0;

	// Here, we just check that the values are in range.  Whether the puzzle is validly constructed is 
	// checked in checkPuzzle() so the checking can be resued.
	
	int lastVal = xDim * yDim;
	while ( optind < argc ) {
		// std::cout << argv[optind][0] << std::endl;
		if ( rowDim > xDim - 1 ) {
			std::cerr <<  "Too many values for puzzle" << std::endl;
			exit(1);
		}
		if ( argv[optind][0] == SlidePuzzle::BLANK_CHAR ) {
			// the OCR version of the tool returns 0 for blank, so we use that here.
			puzzle->setInitialVal(rowDim,colDim,SlidePuzzle::BLANK_VAL);
		} else {
			int iVal = atoi(argv[optind]);
			if ( iVal >=  lastVal )  {
				std::cerr << "Value too Large.  Values are 1 to " << lastVal  -1 << " plus B"  << std::endl;
				exit(1);
			}
			puzzle->setInitialVal(rowDim,colDim,iVal);
			
		}
		// std::cout << "Row " << rowDim << " COL " << colDim << " Value " << atoi(argv[optind]) << std::endl;
		if ( colDim==yDim-1) {
			colDim=0;
			rowDim=rowDim+1;
		} else {
			colDim= colDim+1;
		}

		optind++;
	}
	if ( Debug) {
		puzzle->setDebug(3,verbose);
		spdlog::set_level(spdlog::level::debug);
	}
	// check for validity
	puzzle->checkPuzzle();
	if ( ! puzzle->valid) {
		std::string puzzleError = puzzle->getErrorString();
		std::cout << puzzleError << std::endl;
		exit(0);
	}
	// sole the puzzle.
	if ( ! puzzle->explore(true,false) ) {
		// if we get here, we did not find a solution
		std::cout << "No Solution Found" << std::endl;
	}
	std::string resultsStr = puzzle->getResults();
	std::cout << "Puzzle Solved:" <<  resultsStr << std::endl;
}
