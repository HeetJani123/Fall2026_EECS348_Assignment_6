/*
 * Name of program: EECS 348 Assignment 6 (revision of Assignment 5)
 * Brief description: Object-oriented recursive depth-first search Sudoku solver with backtracking.
 * Inputs: puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt
 * Output: One file, solutions.txt. For each puzzle it holds the line "Puzzle: <file name>", then either every
 *         solution (same format as the input files) followed by "Solutions found: n", or "No solution found."
 *         Short status and error messages are also shown on the screen.
 * Collaborators: None for Assignment 6 (the Assignment 5 code I inherited was written by Heet Jani)
 * Other sources: Claude (baseline code of the inherited Assignment 5);
 *                 ChatGPT and Gemini (two GenAI versions generated for Assignment 6, compared in Assignment6GenAI.pdf;
 *                 ChatGPT's version was chosen as the basis, so its ideas below are marked "ChatGPT");
 *                 Claude (Anthropic) also helped me merge the ChatGPT ideas into this file.
 * Author: Heet Jani
 * Creation date: October 15, 2026
 * Revised By: Kenny Hong
 * Revision Date: 10/29/2026
 * Revised By: Heet Jani
 * Revision Date: 11/12/2026
 * What I revised (Assignment 6): results now go to ONE file (solutions.txt) with "Puzzle: x" header lines;
 *   a bad puzzle file still gets its own header and "No solution found."; clue conflicts are reported;
 *   errors go to std::cerr; every write is checked so a failed write is noticed.
 */

#include <bitset>   // gives me std::bitset, which I use to count the 1-bits in a number
#include <cstdint>  // gives me small fixed-size number types like uint8_t and uint16_t
#include <fstream>  // gives me std::ifstream so I can read the puzzle files
#include <iostream> // gives me std::cout so I can print to the screen
#include <string>   // gives me std::string and std::to_string

// Author: Claude, modified by me for optimization
// This block holds the constants and small helper functions used by the rest of the program.
namespace { // an unnamed namespace, so these names can only be used inside this file
constexpr int kSize     = 9;                  // the board is 9 rows by 9 columns, and digits go from 1 to 9
constexpr int kBoxSize  = 3;                  // each small box inside the board is 3 cells wide and 3 cells tall
constexpr int kCells    = kSize * kSize;      // total number of cells on the board, which is 81
constexpr char kBlank   = '_';                // the character that means an empty cell in the files
constexpr uint16_t kAllDigits = (1u << kSize) - 1; // 9 one-bits in a row (binary 111111111), one bit for each digit 1 to 9
constexpr int kPuzzleCount = 5;               // how many puzzle files the program reads

// Author: Claude, added by me for space safety
// The most solutions to write per puzzle. 0 means "find every solution", which the assignment needs.
// If I set this to a number above 0, the "Solutions found" line shows that capped number, not the true total.
constexpr unsigned long long kMaxSolutions = 0;

// Author: Claude, modified by me for optimization
// Works out which of the 9 small boxes a cell belongs to, numbered 0 to 8.
int boxIndex(int row, int col) { return (row / kBoxSize) * kBoxSize + col / kBoxSize; } // row of box times 3, plus column of box

// Author: Claude, modified by me for optimization
// Counts how many 1-bits are in a number. I use this to count how many digits are still allowed.
int popCount(uint16_t m)       { return static_cast<int>(std::bitset<16>(m).count()); } // put m in a bitset and count its 1-bits
}  // end of the unnamed namespace

// ---------------------------------------------------------------------------
// Author: Claude (baseline code)
// Board class: it stores the 9x9 grid, reads it from a file, and prints it.
// It does not do any solving.
// ---------------------------------------------------------------------------
class Board { // start of the Board class
public: // everything below this can be used by other parts of the program
    // Author: Claude, modified by me for correctness checks
    // Reads exactly 81 cells from a file. Digits 1-9 are clues and '_' is a blank.
    // Returns true if it worked. Returns false and fills in `error` if something is wrong.
    bool load(const std::string& filename, std::string& error) { // takes the file name and a place to put an error message
        std::ifstream in(filename); // open the file for reading
        if (!in) { error = "cannot open file"; return false; } // if the file did not open, set the error message and stop

        int count = 0; // counts how many cells I have read so far
        char ch;       // holds one character from the file at a time
        while (in >> ch) { // read one character at a time, and the >> skips spaces and new lines for me
            if (count == kCells) { error = "more than 81 cells"; return false; } // already have 81 cells but there is another character, so the file is too long
            if (ch == kBlank)               cells_[count] = 0; // an underscore means blank, so I store 0 for it
            else if (ch >= '1' && ch <= '9') cells_[count] = static_cast<uint8_t>(ch - '0'); // a digit character gets turned into a real number and stored
            else { error = std::string("illegal character '") + ch + "'"; return false; } // any other character is not allowed, so report it and stop
            ++count; // move on to the next cell
        } // end of the loop that reads characters
        if (count != kCells) { error = "expected 81 cells, found " + std::to_string(count); return false; } // the file ended too early, so report how many cells I found
        return true; // everything was fine, so tell the caller it worked
    } // end of load

    // Author: Claude (baseline code)
    // Gives back the number stored in one cell. 0 means blank.
    int  at(int cell) const          { return cells_[cell]; } // look up the cell and return its value (const means it does not change the board)

    // Author: Claude (baseline code)
    // Puts a digit into one cell.
    void set(int cell, int digit)    { cells_[cell] = static_cast<uint8_t>(digit); } // store the digit in that cell

    // Author: Claude (baseline code)
    // Prints the board to the given output stream, one row per line.
    void print(std::ostream& os) const { // os is where to print, like std::cout
        for (int r = 0; r < kSize; ++r) { // go through each row from 0 to 8
            for (int c = 0; c < kSize; ++c) { // go through each column from 0 to 8
                int v = cells_[r * kSize + c]; // get the value of this cell (row times 9 plus column gives its spot in the list)
                os << (v ? static_cast<char>('0' + v) : kBlank) << (c + 1 < kSize ? ' ' : '\n'); // print the digit (or _ if blank), then a space, or a new line after the last column
            } // end of the column loop
        } // end of the row loop
    } // end of print

private: // everything below this can only be used inside the Board class
    uint8_t cells_[kCells] = {}; // the 81 cells of the board, all starting at 0 (blank)
}; // end of the Board class

// ---------------------------------------------------------------------------
// Author: Claude, modified by me for optimization
// Solver class: finds every solution with recursive depth-first search and backtracking.
// It keeps bitmasks of which digits are already used in each row, column, and box,
// so checking if a digit is allowed is very fast.
// Each solution is printed right away instead of being saved.
// ---------------------------------------------------------------------------
class Solver { // start of the Solver class
public: // everything below this can be used by other parts of the program
    // Author: Claude (baseline code)
    // Makes a solver. It keeps its own copy of the puzzle and remembers where to print.
    Solver(const Board& puzzle, std::ostream& out) : board_(puzzle), out_(out) {} // copy the puzzle into board_ and remember the output stream in out_

    // Author: Claude, modified by me for optimization
    // Finds all solutions and prints each one as soon as it is found.
    // Returns how many solutions there were. A limit of 0 means find all of them.
    unsigned long long solveAll(unsigned long long limit = 0) { // limit is optional and defaults to 0 (no limit)
        limit_ = limit; // save the limit so the search can check it later
        found_ = 0;     // start the count of solutions at zero
        conflict_ = false; // Revised by: Heet Jani - assume the clues are fine until buildMasks says otherwise
        if (!buildMasks()) { conflict_ = true; return 0; } // if the starting clues clash, remember that fact (so main can report it) and return 0 solutions
        search(emptyCount_); // start the recursive search with all the blank cells still to fill
        return found_; // give back how many solutions were found
    } // end of solveAll

    // Revised by: Heet Jani. Idea from ChatGPT: it printed a message when clues conflict.
    // Tells the caller whether the last solveAll() stopped because two clues clashed (true) or not (false).
    bool hasConflict() const { return conflict_; } // just hands back the flag; const because it changes nothing

private: // everything below this can only be used inside the Solver class
    bool conflict_ = false; // Revised by: Heet Jani - true when two starting clues share a row, column, or box
    Board board_;  // my working copy of the board, which changes while searching
    std::ostream& out_; // where the solutions get printed
    uint16_t rowUsed_[kSize] = {}, colUsed_[kSize] = {}, boxUsed_[kSize] = {}; // one bitmask per row, column, and box, showing which digits are already used (all start empty)
    int emptyCount_ = 0; // how many blank cells the puzzle starts with
    unsigned long long found_ = 0, limit_ = 0; // found_ is how many solutions so far, limit_ is the most to look for (0 means no limit)

    // Author: Claude, modified by me for optimization
    // Records the starting clues in the bitmasks. Returns false if two clues clash.
    bool buildMasks() { // no inputs, it works on the board_ copy
        for (int cell = 0; cell < kCells; ++cell) { // go through all 81 cells
            int d = board_.at(cell); // get the digit in this cell
            if (d == 0) { ++emptyCount_; continue; } // if it is blank, count it and skip to the next cell
            uint16_t bit = 1u << (d - 1); // turn the digit into a single bit (digit 1 is the first bit, digit 9 is the ninth bit)
            int r = cell / kSize, c = cell % kSize, b = boxIndex(r, c); // work out this cell's row, column, and box number
            if ((rowUsed_[r] | colUsed_[c] | boxUsed_[b]) & bit) return false; // if this digit is already used in the row, column, or box, the clues clash, so stop
            place(cell, bit); // otherwise mark this digit as used in its row, column, and box
        } // end of the loop over cells
        return true; // all the clues were fine
    } // end of buildMasks

    // Author: Claude, modified by me for optimization
    // Marks a digit (given as a bit) as used in the row, column, and box of this cell.
    void place(int cell, uint16_t bit) { // cell is which cell, bit is the digit as a bit
        int r = cell / kSize, c = cell % kSize; // work out the row and column of the cell
        rowUsed_[r] |= bit; colUsed_[c] |= bit; boxUsed_[boxIndex(r, c)] |= bit; // turn the bit on in the row, column, and box masks
    } // end of place

    // Author: Claude, modified by me for optimization
    // Does the opposite of place. It marks the digit as free again in the row, column, and box.
    void unplace(int cell, uint16_t bit) { // same inputs as place
        int r = cell / kSize, c = cell % kSize; // work out the row and column of the cell
        rowUsed_[r] &= ~bit; colUsed_[c] &= ~bit; boxUsed_[boxIndex(r, c)] &= ~bit; // turn the bit off in the row, column, and box masks
    } // end of unplace

    // Author: Claude, modified by me for optimization
    // Gives back a mask of all the digits that are still allowed in this cell.
    uint16_t candidates(int cell) const { // const because it only looks and does not change anything
        int r = cell / kSize, c = cell % kSize; // work out the row and column of the cell
        return kAllDigits & ~(rowUsed_[r] | colUsed_[c] | boxUsed_[boxIndex(r, c)]); // start with all 9 digits and remove any digit already used in the row, column, or box
    } // end of candidates

    // Author: Claude, modified by me for optimization
    // Finds the blank cell with the fewest allowed digits.
    // Returns -1 if some blank cell has no allowed digits at all (a dead end).
    int mostConstrainedCell() const { // const because it only looks at the board
        int best = -1, bestCount = kSize + 1; // best is the best cell so far, bestCount starts at 10 so any real cell beats it
        for (int cell = 0; cell < kCells; ++cell) { // go through all 81 cells
            if (board_.at(cell) != 0) continue;            // skip cells that are already filled
            int n = popCount(candidates(cell)); // count how many digits are allowed in this cell
            if (n < bestCount) { best = cell; bestCount = n; if (n <= 1) break; } // if this cell has fewer options, remember it, and stop looking if it has 1 or 0 options
        } // end of the loop over cells
        return bestCount == 0 ? -1 : best; // if the best cell has 0 options, return -1 for dead end, otherwise return the best cell
    } // end of mostConstrainedCell

    // Author: Claude, modified by me for optimization
    // The recursive search. It fills one blank cell, calls itself, then undoes the move (backtracking).
    // Returns true when the search should stop because the limit was reached.
    bool search(int remaining) { // remaining is how many blank cells are still empty
        if (remaining == 0) { report(); return limit_ && found_ >= limit_; } // no blanks left means the board is solved, so print it and stop only if the limit was reached

        int cell = mostConstrainedCell(); // pick the blank cell with the fewest options
        if (cell < 0) return false;                        // a blank cell has no options, so this is a dead end and I go back

        for (uint16_t mask = candidates(cell); mask; mask &= mask - 1) { // go through each allowed digit, and mask &= mask - 1 removes the lowest 1-bit each time
            uint16_t bit = mask & -mask;                   // keep only the lowest 1-bit, which is the next digit to try
            int digit = popCount(bit - 1) + 1;             // turn the bit back into a digit number (count the bits below it, then add 1)
            board_.set(cell, digit);                       // choose: write the digit into the board
            place(cell, bit);                              // mark the digit as used in its row, column, and box
            bool stop = search(remaining - 1);             // explore: call search again with one less blank cell
            unplace(cell, bit);                            // backtrack: mark the digit as free again
            board_.set(cell, 0);                           // backtrack: make the cell blank again
            if (stop) return true;                         // if the limit was reached, pass the stop signal back up
        } // end of the loop over allowed digits
        return false; // tried every digit here and did not hit the limit, so keep going
    } // end of search

    // Author: Claude, modified by me for optimization
    // Prints one finished solution right away instead of saving it, which uses less memory.
    // Author: Claude, modified by me for file output
    // Writes one finished solution to the output stream (which is now a solution file).
    // It prints ONLY the board in the same format as the puzzle files, with no "Solution N:" header.
    // Solutions are separated from each other by one blank line.
    void report() { // no inputs, it writes the current board
        ++found_; // add one to the number of solutions found so far
        if (found_ > 1) out_ << '\n'; // from the second solution on, write a blank line first so solutions are separated
                                      // (the first solution does not get one, so the file never starts with a blank line)
        board_.print(out_); // write the solved board in the same "5 3 _ ..." format as the input files
    } // end of report
}; // end of the Solver class

// Author: Claude, modified by me for file output; revised for Assignment 6 by Heet Jani
// main: goes through the five puzzle files, solves each one, and writes ALL results into the single file solutions.txt.
// Each puzzle's block in that file starts with "Puzzle: <file name>" and then holds either:
//   - the single line "No solution found."  (no solutions, or the puzzle file could not be used), or
//   - every solution in the input-file format, followed by the line "Solutions found: n".
// The "header first, even for bad files" order and the std::cerr error messages follow ChatGPT's version;
// the rest is the inherited Assignment 5 main, changed by me.
int main() { // the program starts here
    const std::string outName = "solutions.txt"; // the one results file the assignment asks for (it replaces the old solutionN.txt files)
    std::ofstream outFile(outName); // open (or create) solutions.txt; any old copy is replaced
    if (!outFile) { // if the file could not be opened for writing
        std::cerr << "Error: cannot create " << outName << '\n'; // tell the user on the error stream
        return 1; // there is nowhere to put results, so end with an error code
    } // end of the output file check

    for (int i = 1; i <= kPuzzleCount; ++i) { // do the steps below for puzzle 1 up to puzzle 5
        const std::string name = "puzzle" + std::to_string(i) + ".txt"; // build the input file name, like puzzle1.txt

        std::cout << "Processing " << name << '\n'; // short status line on the screen so I can see progress

        outFile << "Puzzle: " << name << '\n'; // required header line; written BEFORE loading so every puzzle always has one (ChatGPT's order)

        Board puzzle; // make an empty board to hold the puzzle
        std::string error; // a place to store an error message if reading fails
        if (!puzzle.load(name, error)) { // try to read the puzzle file, and if that fails do the next lines
            std::cerr << "  Error reading " << name << ": " << error << '\n'; // show what went wrong on the error stream
            outFile << "No solution found.\n"; // an unusable puzzle has no solution, and this is the only result line the assignment allows
            continue; // go on to the next puzzle instead of stopping the whole program
        } // end of the error check

        // The solver writes each solution straight into solutions.txt the moment it is found,
        // so no solution is ever kept in memory (this is the constant-space idea both ChatGPT and the inherited code use).
        Solver solver(puzzle, outFile); // make a solver whose output stream is the results file
        unsigned long long count = solver.solveAll(); // find every solution; count is how many were found

        if (count == 0) { // nothing was written for this puzzle
            if (solver.hasConflict()) std::cerr << "  Error: conflicting clues in " << name << '\n'; // tell the user why, so it is not a silent "no solution"
            outFile << "No solution found.\n"; // this single line is the whole result for the puzzle
        } else { // at least one solution was written
            outFile << "Solutions found: " << count << '\n'; // the total count goes on the line right after the last solution, as the assignment words it
        } // end of the result check

        if (!outFile) { // a failed write (for example a full disk) puts the stream into a fail state
            std::cerr << "  Error: problem while writing " << outName << '\n'; // report it
            return 1; // the results file cannot be trusted any more, so stop
        } // end of the write check
        std::cout << "  Solutions found: " << count << '\n'; // also show the count on the screen
    } // end of the loop over puzzles

    outFile.close(); // close the file so everything is saved to disk
    if (!outFile) { // closing can fail too, for example when the last buffered text cannot be flushed
        std::cerr << "Error: problem while closing " << outName << '\n'; // report it
        return 1; // end with an error code
    } // end of the close check
    return 0; // tell the computer the program finished fine
} // end of main
