// EECS 348 Assignment 4 - Sudoku Solver
// A recursive, object-oriented Sudoku solver using depth-first search with backtracking.
// Reads puzzle1.txt through puzzle5.txt from the current directory,
// prints each puzzle and all its solutions, then reports the count.
// Inputs: puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt
// Output: Puzzle grid, solution grid(s) or "No solution found", and solution count
// Author: Brave's Leo genAI ***this is the only thing i changed on this file***
// Created: October 14, 2026
// Sources: Self-authored

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

// Board stores the 9x9 grid. 0 means an empty cell.
class Board {
public:
    int grid[9][9];

    // Load the grid from a file. Each row has 9 tokens separated by whitespace.
    // '_' becomes 0; digits become their integer value.
    bool load(const string& filename) {
        ifstream infile(filename);
        if (!infile.is_open()) {
            cerr << "Could not open " << filename << endl;
            return false;
        }
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                string token;
                infile >> token;
                if (token == "_") {
                    grid[r][c] = 0;
                } else {
                    grid[r][c] = stoi(token);
                }
            }
        }
        infile.close();
        return true;
    }

    // Print the grid to stdout in a human-readable format.
    void print() const {
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                if (c == 0) cout << "  ";
                if (c == 3 || c == 6) cout << " ";
                if (grid[r][c] == 0) {
                    cout << "_";
                } else {
                    cout << grid[r][c];
                }
                if (c < 8) cout << " ";
            }
            cout << endl;
        }
    }

    // Check whether placing value v at row r, column c is legal
    // under Sudoku rules (row, column, and 3x3 block uniqueness).
    bool isValid(int r, int c, int v) const {
        for (int i = 0; i < 9; i++) {
            if (grid[r][i] == v) return false;      // same row
            if (grid[i][c] == v) return false;      // same column
        }
        int br = (r / 3) * 3;
        int bc = (c / 3) * 3;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (grid[br + i][bc + j] == v) return false;  // same block
            }
        }
        return true;
    }
};

// Solver encapsulates the recursive backtracking search.
class Solver {
private:
    Board& board;           // reference to the puzzle board
    int solutionsFound;     // total number of solutions discovered

    // Return the next empty cell as [row, col], or [-1, -1] if full.
    vector<int> findEmpty() const {
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                if (board.grid[r][c] == 0) {
                    return {r, c};
                }
            }
        }
        return {-1, -1};
    }

    // The recursive depth-first search.
    // When the board is completely filled (empty returns {-1,-1}) we've found
    // a solution and print it. Otherwise we pick the first empty cell, try
    // values 1-9, and recurse. If no value works we backtrack.
    void search() {
        vector<int> empty = findEmpty();
        int r = empty[0];
        int c = empty[1];

        // Board is full — we found a solution.
        if (r == -1) {
            solutionsFound++;
            board.print();
            cout << endl;
            return;
        }

        // Try each candidate value.
        for (int v = 1; v <= 9; v++) {
            if (board.isValid(r, c, v)) {
                board.grid[r][c] = v;   // place value
                search();               // recurse
                board.grid[r][c] = 0;   // backtrack: undo placement
            }
        }
    }

public:
    // Construct a solver bound to a specific board.
    explicit Solver(Board& b) : board(b), solutionsFound(0) {}

    // Run the solver. Returns the number of solutions found.
    int solve() {
        search();
        return solutionsFound;
    }
};

// Main: iterate over puzzle1.txt … puzzle5.txt, load each, solve, print results.
int main() {
    for (int p = 1; p <= 5; p++) {
        Board board;
        ostringstream oss;
        oss << "puzzle" << p << ".txt";
        string filename = oss.str();

        cout << "Puzzle file: " << filename << endl;
        cout << "Original puzzle:" << endl;

        if (!board.load(filename)) {
            cout << endl;
            continue;
        }

        board.print();

        Solver solver(board);
        int count = solver.solve();

        if (count == 0) {
            cout << "No solution found" << endl;
        }

        cout << "Solutions found: " << count << endl;
        cout << endl;
    }

    return 0;
}
