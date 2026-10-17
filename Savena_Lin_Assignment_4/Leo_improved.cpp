/*
Name of program: EECS 348 Assignment 4
Description: Sudoku solver
Inputs: puzzles with clues
Output: completed sudoku puzzle, unless there's no solutions
All collaborators: None
Other sources for the code: Lumo, Leo, comment lines from Leo were kept
Author’s full name: Savena Lin
Creation date: 14 October 2026
*/


    #include <iostream>
    #include <fstream>
    #include <sstream>
    #include <string>
    #include <vector>
    #include <iomanip>

    using namespace std;

    // Board stores the 9x9 grid. 0 means an empty cell. *leo comment*
    class Board {
    public:
        int grid[9][9]; //2d array that represents the sudoku grid 

        // Load the grid from a file. Each row has 9 tokens separated by whitespace. *leo comment*
        // '_' becomes 0; digits become their integer value. *leo comment*
        bool load(const string& filename) { //creates the input file steam, puzzle files are put thru this
            ifstream infile(filename); //checks if successful file open
            if (!infile.is_open()) {
                cerr << "Could not open " << filename << endl;
                return false;
                //This block checks to make sure the file is successfully opened, returns false if it cannot
            }
            for (int r = 0; r < 9; r++) {
                for (int c = 0; c < 9; c++) { 
            //these two lines of code looping thru all rows and columns
                    string token;
                    infile >> token;
                    if (token == "_") {
                        grid[r][c] = 0;
                        //Checks if the token is empty/an underscore
                    } else {
                        grid[r][c] = stoi(token);
                        //If it's not empty, convert into an int and store it
                    }
                }
            }
            infile.close(); //closes the file
            return true; //indicates the file was successfully opened
        }

        // Print the grid to stdout in a human-readable format. *leo comment*
        void print() const {
            for (int r = 0; r < 9; r++) {
                for (int c = 0; c < 9; c++) {
                //looping thru all rows and columns
            
                    if (c == 0) cout << "  "; //visual, adds indentation for alignment
                    if (c == 3 || c == 6) cout << " "; //adds a space for clarity between every 3rd column
                    if (grid[r][c] == 0) {
                        cout << "_";
                        //checks if the cell is empty, if so, prints an underscore
                    } else {
                        cout << grid[r][c];
                        //otherwise, it prints the int of the clue
                    }
                    if (c < 8) cout << " "; //places a space between all the cells, but not for the last column
                }
                cout << endl; //prints a new line after each row
            }
        }

        // Check whether placing value v at row r, column c is legal *leo comment*
        // under Sudoku rules (row, column, and 3x3 block uniqueness). *leo comment*
        bool isValid(int r, int c, int v) const {
            for (int i = 0; i < 9; i++) {
                if (grid[r][i] == v) return false; // same row *leo comment*
                //checks if the value already exists in the same row
                if (grid[i][c] == v) return false; // same column *leo comment*
                //checks if the value already exists in the same column
            }
            int br = (r / 3) * 3; //creates the top left corner row of the 3x3 block
            int bc = (c / 3) * 3; //creates the top left corner column of the 3x3 block
            for (int i = 0; i < 3; i++) { //loops thru the 3x3 block's rows
                for (int j = 0; j < 3; j++) { //loops thru the 3x3 block's column
                    if (grid[br + i][bc + j] == v) return false; // same block *leo comment*
                    //checks if theres a dupe in that 3x3 block
                }
            }
            return true; //returns true if all placements are allowed
        }
    
        //added, checks to see if the clues given are contradictory/invalid
        vector<string> findContradictions() const { 
            vector<string> conflicts; //stores conflict messages
            int tmp[9][9]; //Temp 2d array that stores the grid while it's checking for contradictions
            for (int r = 0; r < 9; r++) 
                for (int c = 0; c < 9; c++)
                    tmp[r][c] = grid[r][c];
                    //stores the original grid

            for (int r = 0; r < 9; r++) {
                for (int c = 0; c < 9; c++) {
                //loops thru the array
                    if (tmp[r][c] == 0) continue; //skips all cells w 0
                    int v = tmp[r][c]; //stores the value in a temp variable
                    tmp[r][c] = 0; // hide this clue before testing it
                    bool dup = false; //variable created to track if dupe is found
                    for (int i = 0; i < 9; i++) { 
                        if (tmp[r][i] == v || tmp[i][c] == v) dup = true;
                    } //checks if value exists in the same row
                    int br = (r / 3) * 3, bc = (c / 3) * 3; //creates the 3x3 block
                    for (int i = 0; i < 3 && !dup; i++)
                        for (int j = 0; j < 3; j++)
                            if (tmp[br + i][bc + j] == v) { dup = true; break; }
                            //for the 3x3 block, check if there's a dupe in there
                    if (dup) {
                        ostringstream msg;
                        msg << "Duplicate " << v << " at row " << (r + 1)
                            << ", col " << (c + 1);
                        conflicts.push_back(msg.str());
                        //creates the conflict message
                    }
                    tmp[r][c] = v; // restores the temp array to its original value
                }
            }
            return conflicts;
        }
    };
    // Solver encapsulates the recursive backtracking search. *leo comment*
    class Solver {
    private:
        Board& board; // reference to the puzzle board *leo comment*
        int solutionsFound; // total number of solutions discovered *leo comment*

        // Return the next empty cell as [row, col], or [-1, -1] if full.
        vector<int> findEmpty() const {
            for (int r = 0; r < 9; r++) {
                for (int c = 0; c < 9; c++) {
                    if (board.grid[r][c] == 0) {
                        return {r, c};
                        //iterates thru the board and adds the coordinates of empty cells as vectors
                    }
                }
            }
            return {-1, -1}; //returns this if there are no empty cells
        }

        // The recursive depth-first search. *leo comment*
        // When the board is completely filled (empty returns {-1,-1}) we've found *leo comment*
        // a solution and print it. Otherwise we pick the first empty cell, try *leo comment*
        // values 1-9, and recurse. If no value works we backtrack. *leo comment*
        void search() {
            vector<int> empty = findEmpty();
            int r = empty[0]; //grabs the row coordinate of the empty cell
            int c = empty[1]; //grabs the col coordinate of the empty cell

            // Board is full — we found a solution. *leo comment*
            if (r == -1) {
                solutionsFound++; //adds to the number of found solutions
                board.print(); //prints the solved puzzle grid 
                cout << endl; //prints new line
                return; //continue with the recursive call, keeps backtracking
            }

            //Try each candidate value. *leo comment*
            for (int v = 1; v <= 9; v++) { //goes from 1->9 in every empty slot
                if (board.isValid(r, c, v)) { //checks if that int is valid in that position
                    board.grid[r][c] = v; // place value *leo comment*
                    search(); // recurse *leo comment*
                    board.grid[r][c] = 0;// backtrack: undo placement *leo comment*
                }
            }
        }

    public:
        // Construct a solver bound to a specific board. *leo comment*
        explicit Solver(Board& b) : board(b), solutionsFound(0) {} //initializes the solutions found to 0

        // Run the solver. Returns the number of solutions found. *leo comment*
        int solve() {
            search();
            return solutionsFound; //starts the recursive search and returns the amount of solutions found
        }
    };

    // Main will iterate over puzzle1 to puzzle5, load each, solve, and print the results
    int main() {
        for (int p = 1; p <= 5; p++) {
            Board board;
            ostringstream oss;
            oss << "puzzle" << p << ".txt"; //builds the puzzle file name 
            string filename = oss.str(); //stores the above as the file name

            cout << "Puzzle file: " << filename << endl;
            cout << "Original puzzle:" << endl;
            //prints the filename and the word "original puzzle"

            if (!board.load(filename)) {
                cout << endl;
                continue;
            }
            //if the board loads, keeps going, if it fails, skips this puzzle and goes to next puzzle 

            board.print(); //prints the original puzzle with the clues
            cout << endl; //For more clarity, adding the space between the clues and the solutions

            // Reject contradictory puzzles immediately instead of searching
            vector<string> conflicts = board.findContradictions();

            if (!conflicts.empty()) { //checks for if there were issues with the clues
                cout << "Invalid puzzle - contradictory clues:" << endl; //prints that the puzzle is not possible
                for (const string& msg : conflicts) {
                    cout << "  " << msg << endl;
                }
                cout << "Solutions found: 0" << endl; //if its an invalid puzzle, 0 solutions found
                cout << endl;
                continue; //goes to next iteration
            }

            Solver solver(board); //goes to solve the puzzle
            int count = solver.solve(); //variable for how many solutions found

            if (count == 0) {
                cout << "No solution found" << endl; //if it returns that there are 0 solutions, print no solutions
            }

            cout << "Solutions found: " << count << endl;
            cout << endl; //if there are solutions, prints it
        }

        return 0;
    }