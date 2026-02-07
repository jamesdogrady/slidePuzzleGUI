#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <list>
#include <vector>

bool Debug = false;
bool isBlack(int x, int y, PIX *image) {
	l_uint32 pixel;
	int r,g,b;
	if ( pixGetPixel(image, x, y, &pixel)  == 0 ) {
		extractRGBValues(pixel, &r, &g, &b);
		if ( r<100 && g < 100  && b < 100 ) {
			if ( Debug ) {
				printf("Black (%d,%d)\n",x,y);
			}
			return true;
		} else {
			if ( Debug ) {
				printf("Not Black (%d,%d)\n",x,y);
			}
			return false;
		}
	}
	// This error means we ran off the end of the puzzle.
	printf("Unxpected error %d %d\n",x,y);
	return false;
}

// black at position x,y.  Move down until you see a non-black square
int vertBlackLength(int x, int y,int height,PIX *image ) {
    	l_uint32 pixel;
	for ( int i=y+1;i<height;i++ ) {
	    if ( ! isBlack(x,i,image) ) {
			return ( (i-1) - y);
	    }
	}
	return((height-1) - y);
}

// black at position x,y.  Move right until you see a non-black square
int horzBlackLength(int x, int y,int width,PIX *image) {
    	l_uint32 pixel;
	for ( int i=x+1;i<width;i++ ) {
	    if ( ! isBlack(i,y,image) ) {
		return ( (i-1) - x);
	    }
	}
	return((width-1) - x);
}


void check_if_puzzle(int x0,int y0,int width, int height,PIX *image ) {
	// we are at the top left corner of the puzzle.  We need to find the lines
	// this could be the puzzle if we find two vertical 
	int vert_count=1;
	int horz_count=1;
	// top line, start of first line, end of first line, start of second line, end of second line, start of last line
	int max_count=6;
	int vert_idx[6];
	int horz_idx[6];
	
	vert_idx[0] = x0;
	horz_idx[0] = y0;
	int square_threshhold = 270;
	int puzzle_threshhold = 820;
	int line_width=10;
	int x,y;
	bool done = false;
    	l_uint32 pixel;
	// if there is a puzzle here, we are at the top left. There will be three verical lines and
	// three horizontal lines space about 270 apart.  Note that we are ultimately trying to find
	// all sides of the pictures, so when we find a line, we need to also find where the line ends.
	// move right , looking for a black and a vertical line
	x=x0+1;
	y=y0+3;
	int bw;
	bool lastBlack=false;
	printf("Looking for vertical lines starting at %d %d\n",x,y);
	// when you see more than one line in a row, take the first one only.
	bool lastLine=false;
	int diff;
	while ( ! done ) {
	    if ( isBlack(x,y,image) ) {
		bw=vertBlackLength(x,y,height,image);
		if ( bw > puzzle_threshhold ) {
			// start of line so it must be square threshhold away from the last entry.
			diff =  x - vert_idx[vert_count-1] ;
			if ( diff  < 250 || diff  > 325 ) {
				printf("Invalid line spacing %d %d %d\n",x,y,diff);
				// if we don't find a square we aren't in a puzzle so just give up.
				if ( diff > 325 ) {
					done=true;
				}
			} else {
				printf("%d %d %d found good vertical line\n",x,y,bw);
				vert_idx[vert_count++] = x-1;
				// this would be the lower line on the puzzle.
				if ( vert_count >= max_count) {
					done=true;
				} else {
					// find the end of the line.
					x=x+1;
					while ( ( bw=vertBlackLength(x,y,height,image)) > puzzle_threshhold ) {
						x=x+1;
					}
					diff =  x - vert_idx[vert_count-1] ;
					if ( diff < 5 || diff > 20 ) {
						printf("Invalid line spacing %d\n",diff);
					} else {
						printf("%d %d %d found good vertical line\n",x,y,bw);
						// this line is non-black so it's part of the picure.
						vert_idx[vert_count++] = x;
					}
				}	

			}
		} 
	    }
	    x=x+1;
	    if ( x >= width) {
		    done=true;
	    }
	}
	// move down looking for horizontal lines
	x=x0+3;
	y=y0+1;
	done=false;
	lastBlack=false;
	if ( vert_count != max_count ) {
		printf("Skipping horizontal lines because we don't have a match\n");
		return;
	}
	printf("Looking for horizontal lines starting at %d %d\n",x,y);
	while ( ! done ) {
	    if ( isBlack(x,y,image))  {
		bw=horzBlackLength(x,y,width,image);
		if ( bw > puzzle_threshhold ) {
			diff = y - horz_idx[horz_count-1];
			if ( diff  < 250 || diff  > 325 ) {
				printf("Invalid line spacing %d %d %d\n",x,y,diff);
				if ( diff > 250 ) {
					done=true;
				}
			} else {
				// good value
				printf("%d %d %d found good horizontal line\n",x,y,bw);
				horz_idx[horz_count++] = y -1;
				if ( horz_count >= max_count) {
					done=true;
				} else {
					// find the end of the black lines.
					do {
						y=y+1;
					} while ( ( bw=horzBlackLength(x,y,width,image)) > puzzle_threshhold);
					// start of the picture
					diff = y - horz_idx[horz_count-1];
					if ( diff  < 5 || diff  > 20 ) {
						printf("Invalid line spacing %d %d %d\n",x,y,diff);
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
	// 0-1 2-4 4-6
	if ( horz_count ==  max_count && vert_count == max_count ) {
		printf("This is a puzzle match\n");
		printf("Top Left %d %d %d %d\n",vert_idx[0],horz_idx[0],vert_idx[1],horz_idx[1]);
		printf("Top Middle %d %d %d %d\n",vert_idx[2],horz_idx[0],vert_idx[3],horz_idx[1]);
		printf("Top Right %d %d %d %d\n",vert_idx[4],horz_idx[0],vert_idx[5],horz_idx[1]);
		printf("Middle Left %d %d %d %d\n",vert_idx[0],horz_idx[2],vert_idx[3],horz_idx[3]);
		printf("Middle Middle %d %d %d %d\n",vert_idx[2],horz_idx[2],vert_idx[3],horz_idx[3]);
		printf("Middle Right %d %d %d %d\n",vert_idx[4],horz_idx[2],vert_idx[3],horz_idx[3]);
		printf("bottom Left %d %d %d %d\n",vert_idx[0],horz_idx[4],vert_idx[1],horz_idx[5]);
		printf("Bottom Middle %d %d %d %d\n",vert_idx[2],horz_idx[4],vert_idx[3],horz_idx[5]);
		printf("Bottom Right %d %d %d %d\n",vert_idx[4],horz_idx[4],vert_idx[5],horz_idx[5]);
	}
}
int main() {
	/*
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();

    // Initialize tesseract with English. Pass tessdata path if needed.
    if (api->Init(NULL, "eng")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        return 1;
    }
    */

    // Open the input image with Leptonica
    Pix *image = pixRead("shot.png");
    l_int32 width = pixGetWidth(image);
    l_int32 height = pixGetHeight(image);
    l_int32 depth = pixGetDepth(image);
    l_uint32 *data = pixGetData(image);
    l_uint32 pixel;



    // newImage will be the upper left corner of image.
    int newx=0;
    int newy = 0;
    bool lastBlack=false;
    int length_threshhold=820;

    std::vector<bool> lastRow ;
    std::vector<bool> thisRow; 
    // the problem is if the first pixel is black in the picture, this won't work.  We'll mis-identify the edge.
    // if we find an edge, we have to move to the left and/or right until we find the start of the pictures.
    // this does more pixel examination that we'd normally want to.  If we don't find an edge, we can perhaps keep
    // track so we don't look again.  The border of the picture has a grey pixel, but let's not rely on it.
    int x =0;
    int y = 0;
    int last_c_x=0;
    int last_c_y=0;
    while ( y < height ) {
	    while ( x<width) {
	    	if ( isBlack(x,y,image) ) {
			int newx=0;
			int newy=0;
			// we are looking for candiates.  Don't bother unless we see both required lines.
			int bw=horzBlackLength(x,y,width,image);
			if ( bw> length_threshhold)  {
				int bw2=vertBlackLength(x,y,height,image);
				if ( bw2 > length_threshhold ) {
					// this could be a candiate so move right and down to find the edge
					newx=x+1;
					while ( horzBlackLength(x,newy,width,image) > length_threshhold) {
						newy++;
					}
					newy=y+1;
					while ( vertBlackLength(newx,y,height,image) > length_threshhold) {
						newx++;
					}
					if ( newx-1 != last_c_x && newy-1 != last_c_y ) {
						printf("Posistion %d %d  is a candidate\n",newx-1,newy-1);
						 check_if_puzzle(newx-1,newy-1,width,height,image);
						last_c_x = newx - 1;
						last_c_y = newy - 1;
					}
				}
			}
		}
		x=x+1;
	    }
	    y=y+1;
	    x=0;
	}
			    
    pixDestroy(&image);

    return 0;
}
