#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>



// stuff about the imkage itself.
//
// stuff about it being a picture.
//
//
// class to define a rectangle by it's corners.
class Rectangle {
	public:
	int top_x;
	int top_y;
	int bottom_x;
	int bottom_y;
	Rectangle(int x1,int y1,int x2,int y2);
	Rectangle();
};


// PuzzlePicture represents a picture that may be a puzzle or may not be.
// the picture itself is a Leptonica object.
// There are a few image manipulation classes to try to get OCR to work better.
//
class PuzzlePicture {
	public:
		static constexpr int puzzleSquareCnt=9;
		PuzzlePicture(const char *fileName);
		~PuzzlePicture();
		bool searchForPuzzle();

		// if we do find a puzzle, rects are the region of each square.
		Rectangle rects[puzzleSquareCnt];
		PIX *image;
		// convert the picture to black and while. The picure is also reversed so that light areas become
		// dark and dark areas become white.
		void cvt2BW(int x1,int y1,int x2,int y2);
		// blacken the edges of a region so that the border of the region is only black.
		void blackenEdges(int x1,int y1,int x2,int y2);
		// return a new PIX image specifed by the region.
		PIX *splitPicture(int x1,int y1,int x2,int y2,int size);
		void setDebug(int _debug,bool _verbose);
		int getHeight();
	private:
		bool debug;
		int debug_lvl;
		bool verbose;
		l_int32 width;
		l_int32 height;
		l_uint32 *data;
		bool isBlack(int x, int y);
		// length of a vertical black line starting at (x,y)
		int vertBlackLength(int x,int y);
		// length of a horizonal black line starting at (x,y)
		int horzBlackLength(int x,int y);
		// see if the region starting at (x0,y0) contains a puzzle, that is is surrounded by a square
		// and containing nine sub-squares of proper size.
		bool checkIfPuzzle(int x0,int y0);
		// search the picture starting at (x,y) for a potential puzzle, e.g. there is a top and left
		// edge at (x,y);
		bool searchForPuzzle(int x,int y);
		static constexpr int puzzlePixels=820;
		static constexpr int squarePixels=240;
		static constexpr int linePixels=10;
		static constexpr int blackThreshhold=100;

};
		
// Try to get the digit for each region of the puzzle.
class PuzzleNumber {
	public: PuzzlePicture *picture;
  	static constexpr unsigned char blank_ret = 0;
	bool debug;
	bool verbose;
	void setDebug(bool debug, bool verbose);
	PuzzleNumber(PuzzlePicture *pict);
	int getNumber(int no);
	tesseract::TessBaseAPI *api;
	void setConfig(std::string configStr);
	private:
		int xOffset;
		int yOffset;
		int xSize;
		int ySize;
		static constexpr int init_xOffset = 11;
		static constexpr int init_yOffset = 13;
		static constexpr int init_xSize = 32;
		static constexpr int init_ySize = 37;
};
