#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <list>
#include <vector>
#include "puzzlePicture.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <string>
#include <iostream>
#include <fstream>


// constructor for PuzzlePicture.
// Read from file name.   If the file read fails, set dimensions to zero so we know we don't 
// have a valid picture.

PuzzlePicture::PuzzlePicture(const char *fileName ) {
     image = pixRead(fileName);
    if ( image == NULL ) {
	    spdlog::error("Failed to open %s\n",fileName);
	    width=0;
	    height=0;
	    return;
    }
    width = pixGetWidth(image);
    height = pixGetHeight(image);
    data = pixGetData(image);
    debug=false;
    debug_lvl=0;
    verbose=false;

}
// turn on debugging
void PuzzlePicture::setDebug(int _debug,bool _verbose ) {
	if ( _debug == 0 ) {
		debug=false;
	} else { 
		debug=true;
	}
	debug_lvl = _debug;
	verbose=_verbose;
}	

// accessor for height.
int PuzzlePicture::getHeight() {
	return (height);
}

// is the pixe4l at (x,y) black.  Black is defined here as r,g.b, all < 100.
bool PuzzlePicture::isBlack(int x, int y) {
	l_uint32 pixel;
	int r,g,b;
	if ( debug && debug_lvl >=2  ) {
		std::stringstream outStr;
		spdlog::debug("isBlack: ({0},{1}) {2} x {3}",x,y,height,width);
	}
	if ( pixGetPixel(image, x, y, &pixel)  == 0 ) {
		extractRGBValues(pixel, &r, &g, &b);
		if ( r< blackThreshhold && g < blackThreshhold  && b < blackThreshhold ) {
			if ( debug && debug_lvl >= 3 ) {
				spdlog::debug("Black ({0},{1})",x,y);
			}
			return true;
		} else {
			if ( debug && debug_lvl >=3 ) {
				spdlog::debug("Not Black ({0},{1})",x,y);
			}
			return false;
		}
	}
	// This error means we ran off the end of the puzzle.
	if ( debug ) {
		spdlog::debug("Unexpected error ({0},{1})",x,y);
	}
	return false;
}

// determine the length of the black line that starts at (x,y) and goes down, e.g (x,y+1).
// we might calculate this value repeatedly if (x,y) is not the start of a puzzle, so cache the value of the longest
// line you see and use that.
int PuzzlePicture::vertBlackLength(int x, int y) {
	static std::vector<int> blackIdx;
	// initialize the static blackIdx for each x to 0.
	if ( blackIdx.size() == 0 ) {
		for ( int i=0;i<width;i++ ) {
			blackIdx.push_back(0);
		}
	}
	// if we've already seen there is a black line ending at a y value > y, we just return the length from
	// our y.
	if ( blackIdx[x] > y ) {
		return (blackIdx[x] - y);
	}
	// this beyond what we'd looked at before so find the longest black line starting from (x,y).
	for ( int i=y+1;i<height;i++ ) {
	    if ( ! isBlack(x,i) ) {
		    	blackIdx[x] = i-1;
			return ( (i-1) - y);
	    }
	}
	// like the dark side of the moon, it's all black.
	blackIdx[x] = height-1;
	return((height-1) - y);
}

// determine the length of the black line start starts at (x,y) and goes right, e.g. (x+1,y);
// we might calculate the value repeatedly so cache the value to save time.
//
int PuzzlePicture::horzBlackLength(int x, int y) {
	static std::vector<int> blackIdx;
	// initialize blackIndx for each y value to 0.
	if ( blackIdx.size() == 0 ) {
		for ( int i=0;i<height;i++ ) {
			blackIdx.push_back(0);
		}
	}
	// if we'd done this before, just return what we found.
	if ( blackIdx[y] > x ) {
		return(blackIdx[y] - x);
	}
	// go right until we find non-black.
	for ( int i=x+1;i<width;i++ ) {
	    if ( ! isBlack(i,y) ) {
		blackIdx[y] = i - 1;
		return ( (i-1) - x);
	    }
	}
	// rest of the line is all black.
	blackIdx[y] = width-1;
	return((width-1) - x);
}

// we know that x0,y0 

bool PuzzlePicture::checkIfPuzzle(int x0,int y0) {
	// we are at the top left corner of the puzzle.  We need to find the lines separator.
	// we'd already found a candiate for the top and left of the puzzle
	// We need 5 more horizonal and 5 more vertical positions to form puzzle.
	// Vertical:
	// 	end of first column of squares
	// 	start of second column of squares
	// 	end of second column of squares
	// 	start of third column of squares
	// 	end of third column of squares.
	// Horizontal
	// 	end of first row of squares
	// 	start of second row of squares
	// 	end of second row of squares
	// 	start of third row of squares
	// 	end of third row of squares.
	// the squares are expected to be about  270 pixels and the lines between the quares about about 10 pixels
	int vert_count=1;
	int horz_count=1;
	// top line, start of first line, end of first line, start of second line, end of second line, start of last line
	//  expected lines to find.
	int max_count=6;
	// store the positions.
	int vert_idx[max_count];
	int horz_idx[max_count];
	
	// (x0,y0) is the start of the puzzle.
	// check if puzzle first finds the corner that's not part of the lines themselves, so its part of the puzzle
	vert_idx[0] = x0;
	horz_idx[0] = y0;
	// expected dimensions.
	int square_threshhold = squarePixels;
	int puzzle_threshhold = puzzlePixels;
	int line_width=linePixels;
	int x,y;
	bool done = false;
	x=x0+1;
	y=y0+3;
	int bw;
    	constexpr int squareLowSlack=25;
    	constexpr int squareHighSlack=55;
    	constexpr int lineSlack=10;
	if ( debug ) {
		spdlog::debug("Call to checkIfPuzzle  ({0},{1}) \n",x,y);
	}

	int diff;
	// this loop looks for  vertical lines.
	while ( ! done ) {
	    // potentially, the start of a line between quares
	    if ( isBlack(x,y) ) {
		bw=vertBlackLength(x,y) ;
		// the line between squares would go for the entire expected length of the puzzle.
		if ( bw > puzzle_threshhold ) {
			// we've identified a potential line.  If it separates squares, it should be around
			// the size of a square.  Ignore lines that are too close or too far apart.
			diff =  x - vert_idx[vert_count-1] ;
			if ( diff  < (square_threshhold-squareLowSlack) || diff  > (square_threshhold + squareHighSlack )) {
				if ( debug  && debug_lvl >=3 ) {
					spdlog::debug("Invalid line spacing ({0},{1}) {2}",x,y,diff);
				}
				// If we haven't found a square (that is,the pontential square is too big, 
				// stop as there is not puzzle here..
				if ( diff > (square_threshhold+squareHighSlack) ) {
					done=true;
				}
			} else {
				// now that we found the start of a black line, find the width of the line.
				// after the line will come the next region of the puzzle.
				vert_idx[vert_count++] = x-1;
				// this would be the lower line on the puzzle.
				// we don't need to find the width of the line if it's also the end of the puzzle.
				if ( vert_count >= max_count) {
					done=true;
				} else {
					// find the end of the line.
					x=x+1;
					while ( ( bw=vertBlackLength(x,y)) > puzzle_threshhold ) {
						x=x+1;
					}
					// we think we've found the widht of the black line so make sure
					// it's consistent with a puzzle.
					diff =  x - vert_idx[vert_count-1] ;
					if ( diff < (line_width-5)  || diff >  (line_width+lineSlack) ) {
						if ( debug ) {
							spdlog::debug("Invalid line spacing {1}",diff);
						}
					} else {
						// this line is non-black so it's part of the picure.
						vert_idx[vert_count++] = x;
					}
				}	

			}
		} 
	    }
	    // move to the right to look for other lines.
	    x=x+1;
	    // we are at the end of the puzzle so we've scanned all the possible veritical lines.
	    if ( x >= width) {
		    done=true;
	    }
	}
	x=x0+3;
	y=y0+1;
	done=false;
	// if this is a puzzle, we'll have the expected number of vertical lines.
	// if we don't, it's not a puzzle. 
	if ( vert_count != max_count ) {
		if (debug ) {
			spdlog::debug("Skipping horizontal lines because we don't have a match\n");
		}
		return false;
	}
	// move down looking for horizontal lines
	if ( debug ) {
		spdlog::debug("Looking for horizontal lines starting at ({0},{1})",x,y);
	}
	while ( ! done ) {
	    if ( isBlack(x,y))  {
		bw=horzBlackLength(x,y);
		if ( bw > puzzle_threshhold ) {
			// we found a black line that is long enough to separate squares inthe puzzle.
			diff = y - horz_idx[horz_count-1];
			// make sure the identified square is the proper size.  if it's too small, ignore it
			// if it's too big, this is not a puzzle.
			if ( diff  < (square_threshhold - squareLowSlack)  || diff  > (square_threshhold+squareHighSlack) ) {
				if ( debug ) {
					spdlog::debug("Invalid line spacing ({0},{1}) {2}",x,y,diff);
				}
				if ( diff > square_threshhold ) {
					done=true;
				}
			} else {
				// this line could separate squares, so record it.
				horz_idx[horz_count++] = y -1;
				if ( horz_count >= max_count) {
					// if we'd found everything we are done.
					done=true;
				} else {
					// find the width of the black lines.
					do {
						y=y+1;
					} while ( ( bw=horzBlackLength(x,y)) > puzzle_threshhold);
					// start of the picture
					diff = y - horz_idx[horz_count-1];
					if ( diff  < (line_width-lineSlack) || diff  > (line_width+lineSlack )) {
						if ( debug ) {
							spdlog::debug("Invalid line spacing {({0},{1}) {2}",x,y,diff);
						}
					} else {
						horz_idx[horz_count++] = y -1;
					}
				}
			}
	    	}
	    }
	    y=y+1;
	    if ( y >= height ) {
		    done=true;
	    }
	}

	// we found everything we need.
	if ( horz_count ==  max_count && vert_count == max_count ) {
		if ( verbose ) {
			spdlog::debug("Found a puzzle match\n");
		}
		// set up Rects to contain the rectangle regions.
		// we'll do this each of the 9 squares we found.
		int start_x,start_y,end_x,end_y;
		start_x = vert_idx[0];
		end_x = vert_idx[1];
		start_y = horz_idx[0];
		end_y = horz_idx[1];
		if ( verbose ) {
			spdlog::debug("Top Left {0} {1} {2} {3}",start_x,start_y,end_x,end_y);
		}
		rects[0] = Rectangle(start_x,start_y,end_x,end_y);
		//move across the top row by adjusting x but not y.
		start_x = vert_idx[2];
		end_x = vert_idx[3];
		if ( verbose) {
			spdlog::debug("Top Middle {0} {1} {2} {3} ",start_x,start_y,end_x,end_y);
		}
		rects[1] = Rectangle(start_x,start_y,end_x,end_y);
		start_x = vert_idx[4];
		end_x = vert_idx[5];
		if ( verbose ) {
			spdlog::debug("Top Right {0} {1} {2} {3} ",start_x,start_y,end_x,end_y);
		}
		rects[2] = Rectangle(start_x,start_y,end_x,end_y);
		// middle row adjust y
		start_x = vert_idx[0];
		end_x = vert_idx[1];
		start_y = horz_idx[2];
		end_y = horz_idx[3];
		if ( verbose ) {
			spdlog::debug("Middle Left {0} {1} {2} {3} ",start_x,start_y,end_x,end_y);
		}
		rects[3] = Rectangle(start_x,start_y,end_x,end_y);
		//move across the top row by adjusting x but not y.
		start_x = vert_idx[2];
		end_x = vert_idx[3];
		if ( verbose ) {
			spdlog::debug("Middle Middle {0} {1} {2} {3}",start_x,start_y,end_x,end_y);
		}
		rects[4] = Rectangle(start_x,start_y,end_x,end_y);
		start_x = vert_idx[4];
		end_x = vert_idx[5];
		if ( verbose ) {
			spdlog::debug("Middle Right {0} {1} {2} {3} ",start_x,start_y,end_x,end_y);
		}
		rects[5] = Rectangle(start_x,start_y,end_x,end_y);
		// bottom row adjust y
		start_x = vert_idx[0];
		end_x = vert_idx[1];
		start_y = horz_idx[4];
		end_y = horz_idx[5];
		if ( verbose ) {
			spdlog::debug("Bottom Left {0} {1} {2} {3}",start_x,start_y,end_x,end_y);
		}
		rects[6] = Rectangle(start_x,start_y,end_x,end_y);
		//move across the top row by adjusting x but not y.
		start_x = vert_idx[2];
		end_x = vert_idx[3];
		if ( verbose ) {
			spdlog::debug("Bottom Middle {0} {1} {2} {3} ",start_x,start_y,end_x,end_y);
		}
		rects[7] = Rectangle(start_x,start_y,end_x,end_y);
		start_x = vert_idx[4];
		end_x = vert_idx[5];
		if ( verbose ) {
			spdlog::debug("Bottom Right {0} {1} {2} {3}",start_x,start_y,end_x,end_y);
		}
		rects[8] = Rectangle(start_x,start_y,end_x,end_y);

		return(true);
	}
	return(false);
}

// look through the picture seeing if we can find a slizePuzzle.  This function just looks for candidates,
// that is horizontal and vertical lines that could form the top and left of a picture.  If a candiate is found,
// checkIfPuzzle() is called.

bool PuzzlePicture::searchForPuzzle() {
    int newx=0;
    int newy = 0;
    // we aren't looking for an exact size for the squares.  let them vary a bit.


    // looking for lines at least this long.
    // Note that if  x is less than this from the width or y is less that this from the height, there cannot
    // be a puzzle here, so we can stop.
    int length_threshhold=puzzlePixels;

    // the problem is if the first pixel is black in the picture, this won't work.  We'll mis-identify the edge.
    // if we find an edge, we have to move to the left and/or right until we find the start of the pictures.
    // this does more pixel examination that we'd normally want to.  If we don't find an edge, we can perhaps keep
    // track so we don't look again.  The border of the picture has a grey pixel, but let's not rely on it.
    int x =0;
    int y = 0;
    // this is the last candiate we found, Skip it if we were to find it again.
    int last_c_x=0;
    int last_c_y=0;
    // start to work on the puzzle
    while ( y < (height-length_threshhold)  ) {

	    if ( y % 500 == 0 ) {
		spdlog::debug("Looking for puzzle, y={0}",y);
	    }
	    while ( x<(width- length_threshhold) ) {
		if ( (x % 500) == 0 ){
			spdlog::debug("Looking for puzzle, y={0} x={1}",y,x);
		}
		// if (x,y) is black, see if there are lines starting here.
	    	if ( isBlack(x,y) ) {
			newx=0;
			newy=0;
			// we are looking for candiates.  Don't bother unless we see both required lines.
			int bw=horzBlackLength(x,y);
			// does the horizontal length mean we could be a candiate?
			if ( bw> length_threshhold)  {
				int bw2=vertBlackLength(x,y);
				if ( bw2 > length_threshhold ) {
					spdlog::debug("Checking for Puzzle corner ({0},{1})",x,y);
					// x,y is the start of a set of lines that could mark the start of a puzzle
					// the puzzle starts at the end of black line, so find the width of each
					// line
					// this could be a candiate so move down and right to find the corner
					newy = y +1;
					// moving down until we don't see a line, so the puzzle might start here
					while ( horzBlackLength(x,newy) > length_threshhold) {
						newy++;
					}
					newx = x+1;
					// moving right until we don't see a line, so the puzzle might start here
					while ( vertBlackLength(newx,y) > length_threshhold) {
						newx++;
					}
					// this is a check to prevent us from finding the same corner and checking
					// that repeatedly.
					if ( newx-1 != last_c_x && newy-1 != last_c_y ) {
						if ( debug ) {
							spdlog::debug("Posistion ({0},{1}) is a candidate\n",newx-1,newy-1);
						}
						// verify that this is the start of a puzzle.
						if (checkIfPuzzle(newx-1,newy-1) ) {
							return(true);
						 }
						// we did not actually find a puzzle, so note that this is NOT
						// the start of a puzzle and resume scanning.  Note we could skip 
						// the lines we found but instead, we cache the corner we found
						last_c_x = newx - 1;
						last_c_y = newy - 1;
						}
				}
			}
			// we've checked this for the puzzle so move on.
		}
		// move to next x
		x=x+1;
	    }
	    // we are done with line y, so start at the left side of line y+1.
	    y=y+1;
	    x=0;
	}
    	return false;
			    
}

// for debugging, a program creates a new image that contains the part of the image from (x1,y1) to (x2,y2)
PIX * PuzzlePicture::splitPicture(int x1,int y1, int x2,int y2,int size) {
	PIX *newImage;
	int start_x,start_y,end_x,end_y;
	if ( size == 0 ) {
		// the whole region
		newImage = pixCreate(x2-x1,y2-y1, 32);
		start_x = x1;
		start_y = y1;
		end_x = x2;
		end_y = y2;
	} else {
		newImage = pixCreate(size-10,size-5,32);
		start_x = x2-(size-10);
		end_x = x2-10;
		start_y = y1+5;
		end_y = start_y+(size-5);
	}
	int new_x =0;
	for ( int x = start_x;x<end_x;++x) {
		int new_y = 0;
		for ( int y = start_y;y<end_y;++y) {
			l_uint32 pixel;
	    		pixGetPixel(image, x, y, &pixel);
	    		pixSetPixel(newImage,new_x,new_y,pixel);
	    		new_y = new_y + 1;
		}
		new_x = new_x + 1;
	}
	return(newImage);
}

// represents a rectangle.
Rectangle::Rectangle(int x1,int y1,int x2,int y2) {
	top_x =x1;
	top_y = y1;
	bottom_x = x2;
	bottom_y = y2;
}

Rectangle::Rectangle() {
	top_x =0;
	top_y = 0;
	bottom_x = 0;
	bottom_x = 0;
}

PuzzlePicture::~PuzzlePicture() {
	pixDestroy(&image);
}

// convert the pixes in the puzzle within the given range to reversed black and white.
// OCR sometimes works better with this type of image.
void
PuzzlePicture::cvt2BW(int x1,int y1,int x2,int y2) {
	for (int x =  x1;x < x2;x++ ) {
		for ( int y = y1;y<y2;y++) {
			 l_uint32 pixel;
            		pixGetPixel(image, x, y, &pixel);
            		int r, g, b;
            		extractRGBValues(pixel, &r, &g, &b);
			int brightness = (r + g + b) / 3;

			if (brightness < 80 ) {
				// Set to white
				l_uint32 white;
				composeRGBPixel(255, 255, 255, &white);
				pixSetPixel(image, x, y, white);

            		} else {
                		l_uint32 black;

                		composeRGBPixel(0, 0, 0, &black);
				pixSetPixel(image, x, y, black);
			}
	    	}
	}

}

// the section of the image should have white numbers in a black background.
// In case there is stray white on the edges, make it black so the OCR doesn't 
// get confused. This makes all pixels from the edge of the selection to black area black as well,
// so the result should be the number with some white removed. Note that if the puzzle image is black
// at the edge we don't blacken so we should have black non-black black background.
void
PuzzlePicture::blackenEdges(int x1,int y1,int x2,int y2) {
	int x,y;
	l_uint32 black;
        composeRGBPixel(0, 0, 0, &black);
	// veritical direction
	for ( x=x1;x<x2;x++ ) {
		y=y1;
		while ( ! isBlack(x,y) && y<=y2 ) {
			pixSetPixel(image,x,y,black);
			y=y+1;
		}
		y=y2;
		while ( ! isBlack(x,y) && y>=y1 ) {
			pixSetPixel(image,x,y,black);
			y=y-1;
		}

	}
	for ( y=y1;y<y2;y++ ) {
		x=x1;
		while ( ! isBlack(x,y) && x<=x2 ) {
			pixSetPixel(image,x,y,black);
			x=x+1;
		}
		x=x2;
		while ( ! isBlack(x,y) && x>=x1 ) {
			pixSetPixel(image,x,y,black);
			x=x-1;
		}

	}
		
}

// we are doing OCR with this class by takeing a small rectangle at the top left of the puzzle square

PuzzleNumber::PuzzleNumber(PuzzlePicture *pict) {
	picture=pict;
	debug=false;
	verbose=false;
	// we estimate the oval comes in 54 pixels from each side.
	xOffset=init_xOffset;
	yOffset=init_yOffset;
	xSize=init_xSize;
	ySize=init_ySize;
		
}

void
PuzzleNumber::setDebug(bool _debug,bool _verbose ) {
	debug = _debug;
	verbose= _verbose;
}
// this is the function that actually does the OCR on puzzle square square.
// using the rectangle in square rects[square], restrict tesseract upper left corner of the given rectanle.
// look for a single digit in the square.
// If we don't find it, blacken the edges and try again.
// If we still don't find it, reverse the image to BW and try again.
// If we still can't find it, retun 0.  This might the the blank square or it might be something we can't recognize.
int PuzzleNumber::getNumber(int square) {
	// we don't want tesseract to write to stderr
    	api = new tesseract::TessBaseAPI();
    	if (api->Init(NULL, "eng")) {
		spdlog::error( "Could not initialize tesseract.\n");
    	}
	api->SetVariable("debug_file","/dev/null");
	api->SetVariable("tessedit_char_whitelist", "12345678"); 
	Rectangle myRect = picture->rects[square];
	// we want a rectangle  at the top right of the picture, 10 from the right side and 5 down from the top.
	// rectangle dimensions.
	// offset from the top right of the picture.
	int start_x = (myRect.bottom_x - (xSize + xOffset));
	int end_x = (myRect.bottom_x - xOffset);
	int start_y = (myRect.top_y + yOffset);
	int end_y = (myRect.top_y + (ySize  + yOffset));
	api->SetImage(picture->image);
	// we are looking for a single char
	api->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
	if ( debug ) {
		spdlog::debug("Rectangle IMAGE {0} {1} {2} {3}",myRect.top_x,myRect.top_y,myRect.bottom_x,myRect.bottom_y);
		spdlog::debug("API IMAGE {0} {1} {2} {3}",start_x,start_y,end_x - start_x,end_y-start_y);
		spdlog::debug("CROP IMAGE {0} {1} {2} {3} ",start_x,start_y,end_x ,end_y);
	}
	// restrict to the top right of the square
	api->SetRectangle(start_x,start_y,end_x - start_x,end_y-start_y);
	char *outText = api->GetUTF8Text();
	if ( debug ) {
		spdlog::debug("{0} Got text {1} ",square,outText);
	}
	api->End();
	delete api;
	//end the API
	if ( debug ) {
		// write the image to a file for debugging.
		PIX *badImage=picture->splitPicture(start_x,start_y,end_x,end_y,0);
		std::string fname = "Out_" + std::to_string(square) + ".png";
		pixWrite(fname.c_str(),badImage,IFF_PNG);
		pixDestroy(&badImage);
	}
	if (strlen(outText) == 0 || atoi(outText) > 10 ) {
		// we did not get anything or we got more than one digit.
		if ( debug ) {
			PIX *badImage=picture->splitPicture(start_x,start_y,end_x,end_y,0);
			std::string fname = "Out_" + std::to_string(square) + ".png";
			pixWrite(fname.c_str(),badImage,IFF_PNG);
			pixDestroy(&badImage);
		}
		// we did not find a number, blacken edges
    		api = new tesseract::TessBaseAPI();
    		if (api->Init(NULL, "eng")) {
			spdlog::error( "Could not initialize tesseract.\n");
    		}
		// we have to reinit the API for the next try
		api->SetVariable("tessedit_char_whitelist", "12345678"); 
		api->SetVariable("debug_file","/dev/null");
		picture->blackenEdges(start_x,start_y,end_x,end_y);
		api->SetImage(picture->image);
		api->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
		api->SetRectangle(start_x,start_y,end_x - start_x,end_y-start_y);
		outText = api->GetUTF8Text();
		if ( debug ) {
			PIX *badImage=picture->splitPicture(start_x,start_y,end_x,end_y,0);
			std::string fname = "BOut_" + std::to_string(square) + ".png";
			pixWrite(fname.c_str(),badImage,IFF_PNG);
			pixDestroy(&badImage);
		}
		if ( debug ) {
			spdlog::debug("{0} Got text {1}\n",square,outText);
		}
		api->End();
		delete api;
		// re-init the API if we have to try again.
		if (strlen(outText) == 0 || atoi(outText) > 10 ) {

			// we did not find a number, convert to BW and try again.
    			api = new tesseract::TessBaseAPI();
    			if (api->Init(NULL, "eng")) {
				spdlog::error( "Could not initialize tesseract.\n");
    			}
			api->SetVariable("tessedit_char_whitelist", "12345678"); 
			api->SetVariable("debug_file","/dev/null");
			// convert the top right to black and white.
			picture->cvt2BW(start_x,start_y,end_x,end_y);
			api->SetImage(picture->image);
			api->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
			api->SetRectangle(start_x,start_y,end_x - start_x,end_y-start_y);
			outText = api->GetUTF8Text();
			if ( debug ) {
				spdlog::debug("{0} Got text {1}",square,outText);
			}
			api->End();
			delete api;
		}
		if ( strlen(outText)== 0 ) {
			return(blank_ret);
		} else {
			return(atoi(outText));

		}
	} else {
		return(atoi(outText));
	}
	return(0);
}

// we are going to assume a format each 2 digits separated by :
// for the command line version of the tool, we can play with how big the top right corner is.
void
PuzzleNumber::setConfig(std::string configStr) {
	int i ;
	// xx:yy:zz:aa
	i=atoi(configStr.substr(0,2).c_str());
	if ( i !=0 ) {
		xOffset=i;
	}
	i=atoi(configStr.substr(3,2).c_str());
	if ( i !=0 ) {
		yOffset=i;
	}
	i=atoi(configStr.substr(6,2).c_str());
	if ( i !=0 ) {
		xSize=i;
	}
	i=atoi(configStr.substr(9,2).c_str());
	if ( i !=0 ) {
		ySize=i;
	}
}


