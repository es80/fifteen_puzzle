#include <stdbool.h>
#include <stdint.h>

#ifndef FIFTEEN_H
#define FIFTEEN_H

// The maximum dimension of a puzzle.
#define DIM_MAX 9

// Various states that the puzzle might be in. Used to display messages.
enum state { UNSOLVED, SOLVED, GOD_MODE, BUSY, GOD_SOLVED, GOD_SOLVED_OPTIMAL,
             THERE_IS_NO_GOD };

// We use a struct puzzle as an encapsulation of the puzzle data.
struct puzzle {

    // The puzzle board tiles are stored as a 2d array.
    int board[DIM_MAX][DIM_MAX];

    // The dimension of the puzzle, e.g. the 15-puzzle, 4x4, is dimension 4.
    int dim;

    // The current indices for the location of the empty tile.
    int empty_row;
    int empty_col;

    // A count of the number of moves made.
    int move_number;

    // The state of the puzzle. Used to display messages.
    enum state puzzle_state;
};

// We have a single global variable for a puzzle p, defined in fifteen.c.
extern struct puzzle p;


////////////////////////////////////////////////////////////////////////////////
// Functions defined in dim4_solver.c
////////////////////////////////////////////////////////////////////////////////

/*
 * Loads heuristic values from disk into an array and returns a pointer to that
 * array.
 */
uint8_t *load_dim4_heuristics(void);

/*
 * Given an offset so that we can identify a the 4x4 lower right corner of the
 * puzzle, and given an array of heuristic values, calls successive
 * heuristic-guided depth-first searches until an optimal solution is found to
 * arrange the 4x4 tiles correctly. Returns true on success. Otherwise returns
 * false.
 */
bool dim4_solver(int board_offset, uint8_t *dim4_array);


////////////////////////////////////////////////////////////////////////////////
// Functions defined in fifteen.c
////////////////////////////////////////////////////////////////////////////////

/*
 * Draws the puzzle board and, if needed, a message underneath.
 */
void draw_board(void);


////////////////////////////////////////////////////////////////////////////////
// Functions defined in logic.c
////////////////////////////////////////////////////////////////////////////////

/*
 * Initializes the game's data. The tiles are produced using either a
 * psuedo-random ordering, the standard ordering or some custom orderings for
 * troubleshooting. Ensures the resultant board configuration is solvable.
 */
void init(char *type);

/*
 * Attempts to slide a tile in the given direction.
 */
void slide(char direction);

/*
 * Attempts to slide the given tile number.
 */
void slide_tile(int tile);

/*
 * Returns true if and only if puzzle is solved.
 */
bool is_solved(void);

/*
 * Provides the automatic solver, 'God mode'. From the current state of the
 * puzzle, calls a series of moves until the puzzle is solved. Returns true
 * upon success, false otherwise.
 */
bool god_mode(uint8_t **dim3_array, uint8_t **dim4_array);


////////////////////////////////////////////////////////////////////////////////
// Functions defined in general_solver.c
////////////////////////////////////////////////////////////////////////////////

/*
 * For a given row, arrange the tiles in the correct order for that row.
 * We assume we have already arranged offset many rows and columns and now wish
 * to arrange the row whose index is offset (rows are zero-indexed).
 */
void arrange_row(int offset);

/*
 * For a given column, arrange the tiles in the correct order for that column.
 * We assume we have already arranged offset many rows and columns and now wish
 * to arrange the column whose index is offset (columns are zero-indexed).
 */
void arrange_column(int offset);

#endif

