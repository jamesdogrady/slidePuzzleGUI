#include <vector>
#include <list>
#include <sstream>

// class respresenting a coordinate of the puzzle.
class Coord {
	public:
		int x;
		int y;
	Coord(int x,int y );
};
// class represents a confgiguration of the puzzle.  A configuration is the values of each square plus the set
// of moves that get you from an initial puzzle configuration to this one.
class PuzzleConfig {
	public:
	PuzzleConfig(int x,int y) ;
	PuzzleConfig(PuzzleConfig *oPuzzle,Coord move) ;
	bool compare (PuzzleConfig *otherConfig) ;
	~PuzzleConfig();
	void setBlank(Coord c) ;
	void add(int val ,Coord c);
	void print(bool detail = true) ;
	unsigned char *contents;
	Coord blank;
	bool final;
	std::list<Coord> moveList;
	friend std::ostream& operator<<(std::ostream& os, const PuzzleConfig & obj);
	void setVerbose(bool val);
	private:
	bool verbose;
	int xSize;
	int ySize;
	int getVal(Coord c) ;

};


// object representing a slide puzzle.
// The initial state is in the attribute startConfig.
// Note we aren't solving the puzzle so much as looking for a solution, so we just generating puzzleConfigs
// until we find the final Configuration.  The puzzle doesn't really have internal state that changes in the course
// of finding a solution.
class SlidePuzzle {
	public :
		SlidePuzzle ( int x, int y ) ;
		// look for a solution.
		bool explore(bool first,bool oneLoop) ;
		// print the solution into resultString.
		void printFinalConfig(PuzzleConfig *finalConfig, PuzzleConfig * startConfig,bool verbose=true);
		void setDebug(int _lvl,bool verbose);
		std::string getResults();
		std::string getErrorString();
		// populate the starting configuration for the puzzle.
		bool setInitialVal(int x,int y, int val);
		// check that the initial configuration is valid.
		bool checkPuzzle();
		// the puzzle is properly populated so a solution can be searched for.
		bool valid;
		static constexpr unsigned char BLANK_CHAR = 'B';
		static constexpr unsigned char BLANK_VAL = 0;
	private:
		int xDim;
		int yDim;
		int size;
		// desired final contents of the puzzle, 1:N ending with "B"
		unsigned char *finalContents;
		// the set of moves that can be done given a blank space.
		// This is populated at init time and will be the same for every similarly sized puzzle object
		std::vector<std::vector<std::list<Coord> > > moves;
		// the set of configurations we'd already explored, and don't need to explore again.
		// This is a two dimensional vector with the first two characters of the puzzle config each vector
		// being a list of moves we've found so for.  The vector tries to make thes search faster.
		std::vector<std::vector <std::list <PuzzleConfig * >  > >  allConfigs;
		// add to allConfigs
		void add(PuzzleConfig *config) ;
		// find in allConfigs
		bool search(PuzzleConfig *config ) ;
		// at init time, set up the final configuration 1:N ending with "B"
		void setFinalContents();
		PuzzleConfig *startConfig;
		std::stringstream resultsString;
		std::stringstream errorString;
		bool gotVal[9];

		bool debug;
		bool verbose;
		int debug_lvl;
		
};
