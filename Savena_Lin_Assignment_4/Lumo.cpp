/*******************************************************************************
 * EECS 348 - Software Engineering I
 * Assignment 4 - Recursive, Object-Oriented Sudoku Solver
 * -----------------------------------------------------------------------------
 * Program name : Assignment4.cpp
 * Description  : Reads Sudoku puzzle files (puzzle1.txt ... puzzle5.txt),
 *                prints each puzzle exactly as it was read, then finds EVERY
 *                solution using a depth-first search that backtracks. For each
 *                puzzle it prints the file name, the original puzzle, each
 *                solution found (or "No solution found"), and the total number
 *                of solutions.
 * Inputs       : puzzle1.txt ... puzzle5.txt in the current working directory.
 *                Each file supplies 81 cells; '1'-'9' are givens and '_'
 *                (also '.' or '0') marks a blank. Optional command-line
 *                arguments may name other files and/or "--max=N" to stop
 *                after N solutions per puzzle.
 * Output       : Console: file name, puzzle, all solutions, solution count,
 *                nodes searched, and elapsed time per puzzle.
 * Collaborators: <none>
 * Other sources: Skeleton generated with <GenAI #1 name> and <GenAI #2 name>;
 *                backtracking approach from the EECS 348 lecture slides.
 * Author       : Proton's Lumo genAI
 * Creation date: <2026-10-14>
 ******************************************************************************/

#include <array>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// --------------------------- shared constants -------------------------------
constexpr int kSize      = 9;               // grid is 9 x 9
constexpr int kBlockDim  = 3;               // each sub-grid is 3 x 3
constexpr int kCellCount = kSize * kSize;   // 81 cells per puzzle
constexpr unsigned short kAllDigits = 0x3FE;// bits 1..9 set (bit 0 unused)

// Map a (row, col) pair onto its 3x3 block number, 0..8.
inline int blockIndexOf(int row, int col) {
    return (row / kBlockDim) * kBlockDim + (col / kBlockDim);
}

// Build the one-hot bit mask for a digit 1..9.
inline unsigned short bitFor(int digit) {
    return static_cast<unsigned short>(1u << digit);
}

// Count the set bits of a mask (how many candidates remain).
inline int countBits(unsigned short mask) {
    int count = 0;
    while (mask != 0) {
        mask = static_cast<unsigned short>(mask & (mask - 1)); // clear lowest set bit
        ++count;
    }
    return count;
}

/*******************************************************************************
 * Class: SudokuGrid
 * Purpose: Value object holding the 81 cell values of one puzzle (0 = blank).
 *          Knows how to read itself from a file, print itself, and check that
 *          its given clues do not already contradict each other.
 * Authorship: <your name>, generated with <GenAI name> and revised by hand.
 ******************************************************************************/
class SudokuGrid {
public:
    // Start with an empty grid so a default-constructed object is always valid.
    SudokuGrid() { cells_.fill(0); }

    // Read 81 cells from a text file. Returns false if the file is missing or
    // does not contain 81 usable cells.
    bool loadFromFile(const std::string& fileName) {
        std::ifstream input(fileName.c_str());      // open the puzzle file
        if (!input.is_open()) {                     // report failure to caller
            return false;
        }
        cells_.fill(0);                             // reset any previous content
        int index = 0;                              // how many cells read so far
        char ch = '\0';                             // current character
        while (index < kCellCount && input.get(ch)) {// stop at 81 cells or EOF
            if (ch >= '1' && ch <= '9') {           // a given clue digit
                cells_[index++] = ch - '0';         // store its numeric value
            } else if (ch == '_' || ch == '.' || ch == '0') { // a blank cell
                cells_[index++] = 0;                // 0 marks "not yet filled"
            }
            // Anything else (spaces, tabs, newlines) is simply skipped.
        }
        return index == kCellCount;                 // success only if all 81 read
    }

    // Read-only accessor for one cell.
    int valueAt(int row, int col) const {
        return cells_[row * kSize + col];
    }

    // Write accessor used by the solver while it tries and un-tries digits.
    void setValue(int row, int col, int value) {
        cells_[row * kSize + col] = value;
    }

    // True when the cell has not been filled in yet.
    bool isEmpty(int row, int col) const {
        return valueAt(row, col) == 0;
    }

    // Print the grid as nine rows; blanks appear as '_' so the original puzzle
    // is still recognisable after solving.
    void print(std::ostream& out) const {
        for (int row = 0; row < kSize; ++row) {          // one output line per row
            for (int col = 0; col < kSize; ++col) {      // nine cells per line
                const int value = valueAt(row, col);     // fetch this cell
                out << (value == 0 ? '_' : static_cast<char>('0' + value)) << ' ';
            }
            out << '\n';                                 // end of the row
        }
    }

    // Verify that no digit appears twice in any row, column, or 3x3 block of
    // the *given* clues. A contradiction here proves no solution can exist.
    bool hasConsistentGivens() const {
        for (int row = 0; row < kSize; ++row) {          // scan every row
            unsigned short seen = 0;                     // digits seen in this row
            for (int col = 0; col < kSize; ++col) {
                const int value = valueAt(row, col);     // current cell value
                if (value == 0) continue;                // blanks cannot clash
                const unsigned short bit = bitFor(value);// mask for this digit
                if ((seen & bit) != 0) return false;     // duplicate found
                seen = static_cast<unsigned short>(seen | bit); // record digit
            }
        }
        for (int col = 0; col < kSize; ++col) {          // scan every column
            unsigned short seen = 0;
            for (int row = 0; row < kSize; ++row) {
                const int value = valueAt(row, col);
                if (value == 0) continue;
                const unsigned short bit = bitFor(value);
                if ((seen & bit) != 0) return false;
                seen = static_cast<unsigned short>(seen | bit);
            }
        }
        for (int block = 0; block < kSize; ++block) {    // scan every 3x3 block
            unsigned short seen = 0;
            const int baseRow = (block / kBlockDim) * kBlockDim; // block origin
            const int baseCol = (block % kBlockDim) * kBlockDim;
            for (int r = 0; r < kBlockDim; ++r) {        // three rows in block
                for (int c = 0; c < kBlockDim; ++c) {    // three columns in block
                    const int value = valueAt(baseRow + r, baseCol + c);
                    if (value == 0) continue;
                    const unsigned short bit = bitFor(value);
                    if ((seen & bit) != 0) return false;
                    seen = static_cast<unsigned short>(seen | bit);
                }
            }
        }
        return true;                                     // all clues agree
    }

private:
    std::array<int, kCellCount> cells_;   // row-major storage of the 81 cells
};

/*******************************************************************************
 * Class: CandidateTracker
 * Purpose: Keeps three arrays of bit masks describing which digits are already
 *          used in each row, column, and block. Candidates for a cell are then
 *          one bitwise operation instead of a nine-cell scan, which is the main
 *          reason the search finishes in milliseconds.
 * Authorship: <your name>, generated with <GenAI name> and revised by hand.
 ******************************************************************************/
class CandidateTracker {
public:
    // Build the masks from the clues already present in the grid.
    explicit CandidateTracker(const SudokuGrid& grid) {
        rowUsed_.fill(0);                       // no digits used anywhere yet
        colUsed_.fill(0);
        blockUsed_.fill(0);
        for (int row = 0; row < kSize; ++row) {          // walk the whole grid
            for (int col = 0; col < kSize; ++col) {
                const int value = grid.valueAt(row, col); // clue in this cell
                if (value != 0) {                        // only givens matter
                    markUsed(row, col, value);           // record the digit
                }
            }
        }
    }

    // Mask of digits that may legally be placed in (row, col).
    unsigned short candidatesAt(int row, int col) const {
        const unsigned short used = static_cast<unsigned short>(
            rowUsed_[row] | colUsed_[col] | blockUsed_[blockIndexOf(row, col)]);
        return static_cast<unsigned short>(kAllDigits & ~used); // invert + trim
    }

    // Record that a digit now occupies (row, col).
    void markUsed(int row, int col, int digit) {
        const unsigned short bit = bitFor(digit);        // one-hot mask
        rowUsed_[row] |= bit;                            // update its row
        colUsed_[col] |= bit;                            // update its column
        blockUsed_[blockIndexOf(row, col)] |= bit;       // update its block
    }

    // Undo markUsed when the search backtracks out of this cell.
    void markFree(int row, int col, int digit) {
        const unsigned short bit = static_cast<unsigned short>(~bitFor(digit));
        rowUsed_[row] &= bit;                            // clear from the row
        colUsed_[col] &= bit;                            // clear from the column
        blockUsed_[blockIndexOf(row, col)] &= bit;       // clear from the block
    }

private:
    std::array<unsigned short, kSize> rowUsed_;    // digits used per row
    std::array<unsigned short, kSize> colUsed_;    // digits used per column
    std::array<unsigned short, kSize> blockUsed_;  // digits used per 3x3 block
};

/*******************************************************************************
 * Class: SudokuSolver
 * Purpose: Owns the recursive depth-first search with backtracking. It mutates
 *          one working grid in place (restoring it on the way back up) and
 *          records a copy of the grid each time a complete solution is reached,
 *          so multiple solutions are reported.
 * Authorship: <your name>, generated with <GenAI name> and revised by hand.
 ******************************************************************************/
class SudokuSolver {
public:
    // solutionLimit == 0 means "report every solution" (the assignment default).
    SudokuSolver(const SudokuGrid& puzzle, std::size_t solutionLimit)
        : puzzle_(puzzle), limit_(solutionLimit), nodesVisited_(0), hitLimit_(false) {}

    // Run the search to completion (or until the optional limit is reached).
    void solve() {
        solutions_.clear();                          // fresh results
        nodesVisited_ = 0;                           // reset instrumentation
        hitLimit_ = false;
        if (!puzzle_.hasConsistentGivens()) {        // contradictory clues
            return;                                  // provably no solution
        }
        SudokuGrid working(puzzle_);                 // mutable copy to search in
        CandidateTracker tracker(working);           // masks for that copy
        search(working, tracker);                    // start the recursion
    }

    const std::vector<SudokuGrid>& solutions() const { return solutions_; }
    std::size_t nodesVisited() const { return nodesVisited_; }
    bool hitLimit() const { return hitLimit_; }

private:
    // True once the caller-supplied cap on reported solutions is reached.
    bool limitReached() const {
        return limit_ != 0 && solutions_.size() >= limit_;
    }

    // One recursive step: pick the most constrained empty cell, try each of its
    // candidates, recurse, then undo the guess if the branch failed.
    void search(SudokuGrid& grid, CandidateTracker& tracker) {
        ++nodesVisited_;                             // count this visit
        if (limitReached()) {                        // optional early stop
            hitLimit_ = true;
            return;
        }

        int bestRow = -1, bestCol = -1;              // chosen empty cell
        int bestCount = kSize + 1;                   // fewest candidates so far
        unsigned short bestMask = 0;                 // candidates of that cell
        bool forced = false;                         // stop scanning if forced

        // Minimum-remaining-values scan: the cell with the fewest candidates is
        // tried first, which prunes the search tree dramatically.
        for (int row = 0; row < kSize && !forced; ++row) {
            for (int col = 0; col < kSize; ++col) {
                if (!grid.isEmpty(row, col)) continue;          // already filled
                const unsigned short mask = tracker.candidatesAt(row, col);
                const int count = countBits(mask);              // candidate count
                if (count == 0) {                               // dead end
                    return;                                     // backtrack now
                }
                if (count < bestCount) {                        // better cell
                    bestCount = count;
                    bestRow = row;
                    bestCol = col;
                    bestMask = mask;
                    if (count == 1) {                           // cannot do better
                        forced = true;
                        break;
                    }
                }
            }
        }

        if (bestRow < 0) {                           // no empty cell remained
            solutions_.push_back(grid);              // record a full solution
            return;
        }

        for (int digit = 1; digit <= kSize; ++digit) {          // try candidates
            if ((bestMask & bitFor(digit)) == 0) continue;      // not a candidate
            grid.setValue(bestRow, bestCol, digit);             // tentative place
            tracker.markUsed(bestRow, bestCol, digit);          // update masks
            search(grid, tracker);                              // recurse deeper
            tracker.markFree(bestRow, bestCol, digit);          // undo masks
            grid.setValue(bestRow, bestCol, 0);                 // undo the cell
            if (limitReached()) {                               // stop if capped
                hitLimit_ = true;
                return;
            }
        }
    }

    SudokuGrid puzzle_;                  // the puzzle being solved
    std::size_t limit_;                  // 0 = unlimited solutions
    std::vector<SudokuGrid> solutions_;  // every solution discovered
    std::size_t nodesVisited_;           // search-tree nodes expanded
    bool hitLimit_;                      // true if the cap ended the search
};

/*******************************************************************************
 * Class: SudokuApplication
 * Purpose: Drives the program: loads each puzzle file, prints the required
 *          report (file name, puzzle, solutions, count), and times the search.
 * Authorship: <your name>, generated with <GenAI name> and revised by hand.
 ******************************************************************************/
class SudokuApplication {
public:
    SudokuApplication(const std::vector<std::string>& puzzleFiles,
                      std::size_t solutionLimit)
        : puzzleFiles_(puzzleFiles), solutionLimit_(solutionLimit) {}

    // Process every requested puzzle file in order.
    void run() {
        for (std::size_t i = 0; i < puzzleFiles_.size(); ++i) {
            processPuzzle(puzzleFiles_[i]);          // handle one file
        }
    }

private:
    // Produce the complete report for a single puzzle file.
    void processPuzzle(const std::string& fileName) {
        std::cout << "\n==================================================\n";
        std::cout << "Puzzle file: " << fileName << '\n';   // required heading
        std::cout << "==================================================\n";

        SudokuGrid puzzle;                              // empty grid object
        if (!puzzle.loadFromFile(fileName)) {           // read the file
            std::cout << "ERROR: could not open or parse '" << fileName << "'\n";
            return;                                     // skip to next puzzle
        }

        std::cout << "\nPuzzle:\n";                     // label the puzzle
        puzzle.print(std::cout);                        // show it as loaded

        const std::chrono::steady_clock::time_point start =
            std::chrono::steady_clock::now();           // start the clock
        SudokuSolver solver(puzzle, solutionLimit_);    // build the solver
        solver.solve();                                 // run the recursion
        const std::chrono::steady_clock::time_point stop =
            std::chrono::steady_clock::now();           // stop the clock

        const std::vector<SudokuGrid>& found = solver.solutions();
        if (found.empty()) {                            // unsatisfiable puzzle
            std::cout << "\nNo solution found\n";
        } else {                                        // print every solution
            for (std::size_t i = 0; i < found.size(); ++i) {
                std::cout << "\nSolution " << (i + 1) << ":\n";
                found[i].print(std::cout);              // nine rows of digits
            }
        }

        std::cout << "\nSolutions found: " << found.size();
        if (solver.hitLimit()) {                        // cap was reached
            std::cout << " (search stopped at the requested limit)";
        }
        std::cout << '\n';

        const double elapsedMs =                    // milliseconds elapsed
            std::chrono::duration<double, std::milli>(stop - start).count();
        std::cout << "Search nodes visited: " << solver.nodesVisited() << '\n';
        std::cout << "Elapsed time: " << elapsedMs << " ms\n";
    }

    std::vector<std::string> puzzleFiles_;   // files to process
    std::size_t solutionLimit_;              // 0 = report all solutions
};

/*******************************************************************************
 * Function: main
 * Purpose : Parse optional command-line arguments, default to the five
 *           assignment puzzle files, and run the application.
 * Authorship: <your name>, generated with <GenAI name> and revised by hand.
 ******************************************************************************/
int main(int argc, char* argv[]) {
    std::vector<std::string> files;          // puzzle files to solve
    std::size_t solutionLimit = 0;           // 0 means "no limit"

    for (int i = 1; i < argc; ++i) {         // inspect each argument
        const std::string arg(argv[i]);      // current argument as a string
        if (arg.rfind("--max=", 0) == 0) {   // "--max=N" caps solutions
            try {
                solutionLimit = static_cast<std::size_t>(
                    std::stoull(arg.substr(6)));   // parse N
            } catch (const std::exception&) {      // bad number supplied
                std::cerr << "Ignoring invalid argument: " << arg << '\n';
                solutionLimit = 0;                 // fall back to unlimited
            }
        } else {
            files.push_back(arg);            // any other argument is a file
        }
    }

    if (files.empty()) {                     // no files given: use defaults
        for (int i = 1; i <= 5; ++i) {
            files.push_back("puzzle" + std::to_string(i) + ".txt");
        }
    }

    SudokuApplication application(files, solutionLimit); // build the app
    application.run();                                   // do the work
    return 0;                                            // normal exit
}