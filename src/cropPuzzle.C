#include <leptonica/allheaders.h>
#include <list>
#include <vector>
#include <iostream>

bool Debug = false;

int main( int argc, char **argv) {
    // Open the input image with Leptonica
    Pix *image = pixRead(argv[1]);
    l_int32 width = pixGetWidth(image);
    l_int32 height = pixGetHeight(image);
    l_int32 depth = pixGetDepth(image);
    l_uint32 *data = pixGetData(image);
    l_uint32 pixel;

    int start_x = atoi(argv[2]);
    int start_y = atoi(argv[3]);

    int end_x = atoi(argv[4]);
    int end_y = atoi(argv[5]);
    char *outfile=argv[6];

    // newImage will be the upper left corner of image.
    int new_x=0;
    int new_y = 0;
    bool lastBlack=false;
    int length_threshhold=820;
    PIX *newImage= pixCreate(end_x-start_x,end_y-start_y,32);
    new_x = 0;
    new_y = 0;

     for (int x = start_x ; x < end_x ; ++x ) {
	lastBlack=false;
	new_y=0;
    	for (int y = start_y; y < end_y; ++y) {
            l_uint32 pixel;
	    pixGetPixel(image, x, y, &pixel);
	    pixSetPixel(newImage,new_x,new_y,pixel);
	    printf("Old %d %d New %d %d\n",x,y,new_x,new_y);
	    new_y = new_y + 1;
	}
	new_x = new_x + 1;
    }

    if (pixWrite(outfile, newImage, IFF_PNG) !=0 ) {
        std::cerr << "Failed to write output: " << outfile << "\n";
    } else {
        std::cout << "Binarized image saved to " << outfile << "\n";
    }

    pixDestroy(&image);
    pixDestroy(&newImage);

    return 0;
}
