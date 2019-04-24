/**
 * logic.c
 *
 * This file implements functions which deal with the game logic -
 * - The initialization of the board for a new puzzle in either a standard or
 *   random configuration and some helper functions for this.
 * - Functions to move tiles, either by a direction or by a tile number.
 * - A function to check if the puzzle is solved.
 * - A function for the automatic solver or 'God mode'.
 * - Functions which assist 'God mode' in the 3x3 case.
 */

#define _XOPEN_SOURCE 500

#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fifteen.h"

/*
 * Some constants used by the optimal solver for 3x3 puzzles.
 */
#define DIM3_NUM_TILES 9
// There are 9! = 362,880 permutations of tiles numbered 0 to 8, (only half of
// which will actually be valid states of the puzzle board).
#define DIM3_NUM_BOARDS 362880
#define DIM3_SOLUTIONS_FILE "dim3_solutions.bin"

/*
 * Swaps the contents of array[i] and array[j], increments a swap counter.
 */
void swap(int array[], int i, int j, int *swap_count)
{
    if (array[i] == array[j])
    {
        return;
    }
    *swap_count += 1;
    int temp = array[i];
    array[i] = array[j];
    array[j] = temp;
    return;
}

/*
 * Quicksorts an array between indices low and high inclusive. Also counts the
 * number of swaps made whilst sorting.
 */
void quicksort(int array[], int low, int high, int *swap_count)
{
    if (low < high)
    {
        // Using array[high] as a pivot, partition the array and get the index
        // of where the pivot should go.

        // Track the new index where the pivot value should go once finished
        int pivot_index = low;

        // Loop through the array up to but not including pivot array[high].
        for (int i = low; i < high; i++)
        {
            // Swap values as necessary comparing against the pivot value.
            if (array[i] <= array[high])
            {
                swap(array, i, pivot_index, swap_count);
                // If a swap is done, increment the new index for the pivot.
                pivot_index++;
            }
        }

        // Now swap the pivot currently at array[high] into the correct index.
        swap(array, pivot_index, high, swap_count);

        // Recursively call on sub-arrays below and above pivot index.
        quicksort(array, low, pivot_index - 1, swap_count);
        quicksort(array, pivot_index + 1, high, swap_count);
    }
    return;
}

/*
 * Initializes the game's data. The tiles are produced using either a
 * psuedo-random ordering, the standard ordering or some custom orderings for
 * troubleshooting. Ensures the resultant board configuration is solvable.
 */
void init(char *type)
{
    // Declare a p.dim x p.dim array for numbers 1 to p.dim x p.dim.
    int array[p.dim * p.dim];

    if (!strcmp(type, "random"))
    {
        // Simulataneously initialize and shuffle the contents of the array
        // using Fisher-Yates shuffle inside-out variation
        // https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle

        // Iterate over the array.
        for (int i = 0; i < (p.dim * p.dim); i++)
        {
            // Let j be a random number between 0 and i inclusively,
            // (int) (drand48() * n) returns a random integer 0 to n-1.
            int j = (int) (drand48() * (i + 1));

            // Array[i] takes the value of the randomly chosen array[j].
            if (j != i)
            {
                array[i] = array[j];
            }
            // Whilst array[j] is initialized to the next value we want to
            // place in the array.
            array[j] = i + 1;
        }
    }

    // The standard board configuration.
    else if (!strcmp(type, "standard"))
    {
        for (int i = 0; i < p.dim * p.dim; i++)
        {
            array[i] = p.dim * p.dim - i - 1;
        }
        array[p.dim * p.dim - 1] = p.dim * p.dim;

        // To be solvable we must swap the 1 and 2 tiles when dim is even.
        if (p.dim % 2 == 0)
        {
            array[p.dim * p.dim - 2] = 2;
            array[p.dim * p.dim - 3] = 1;
        }
    }

    // For testing generate some known solvable boards.
    // A solved board.
    else if (!strcmp(type, "solved"))
    {
        for (int i = 0; i < p.dim * p.dim; i++)
        {
            array[i] = i + 1;
        }
    }

    // One move required to solve.
    else if (!strcmp(type, "trivial"))
    {
        for (int i = 0; i < p.dim * p.dim; i++)
        {
            array[i] = i + 1;
        }
        // Move last tile right.
        array[p.dim * p.dim - 1] = p.dim * p.dim - 1;
        array[p.dim * p.dim - 2] = p.dim * p.dim;
    }

    // Four moves required to solve.
    else if (!strcmp(type, "almost"))
    {
        for (int i = 0; i < p.dim * p.dim; i++)
        {
            array[i] = i + 1;
        }
        // Rotate 3 tiles in bottom right corner.
        array[p.dim * p.dim - 2] = p.dim * (p.dim - 1) - 1;
        array[p.dim * (p.dim - 1) - 1] = p.dim * p.dim - 1;
        array[p.dim * (p.dim - 1) - 2] = p.dim * (p.dim - 1);
    }

    // Use the generated array to populate the 2D board.
    for (int row = 0; row < p.dim; row++)
    {
        for (int col = 0; col < p.dim; col++)
        {
            // Make the p.dim * p.dim value the empty tile, track its position.
            if (array[row * p.dim + col] == p.dim * p.dim)
            {
                p.board[row][col] = 0;
                p.empty_row = row;
                p.empty_col = col;
            }
            else
            {
                p.board[row][col] = array[row * p.dim + col];
            }
        }
    }
    // Initialize the rest of the puzzle data.
    p.move_number = 0;
    p.puzzle_state = UNSOLVED;

    // To ensure the generated board is solvable, the parity of the permutation
    // of the values 1 to p.dim x p.dim plus the parity of the taxicab distance
    // (number of rows plus number of columns) of the empty tile from the lower
    // right corner, must be even. (This is an invariant for the puzzle moves.)

    // Quicksort the array and count the swaps.
    int swap_count = 0;
    quicksort(array, 0, (p.dim * p.dim) - 1, &swap_count);
    // Determine taxicab distance of empty tile.
    int taxicab_dist = (p.dim - 1) - p.empty_row + (p.dim - 1) - p.empty_col;

    // Check the parity of the invariant.
    if ((swap_count + taxicab_dist) % 2)
    {
        // The board is unsolvable so swap two non-empty tiles.
        if (p.board[0][0] != 0)
        {
            if (p.board[0][1] != 0)
            {
                int temp = p.board[0][0];
                p.board[0][0] = p.board[0][1];
                p.board[0][1] = temp;
            }
            else
            {
                int temp = p.board[0][0];
                p.board[0][0] = p.board[1][0];
                p.board[1][0] = temp;
            }
        }
        else
        {
            int temp = p.board[0][1];
            p.board[0][1] = p.board[1][0];
            p.board[1][0] = temp;
        }
    }
}

/*
 * Attempts to slide a tile in the given direction.
 */
void slide(char direction)
{
    switch (direction)
    {
        case 'l':
            if (p.empty_col < p.dim - 1)
            {
                int tile = p.board[p.empty_row][p.empty_col + 1];
                p.board[p.empty_row][p.empty_col] = tile;
                p.board[p.empty_row][p.empty_col + 1] = 0;
                p.empty_col++;
                p.move_number++;
            }
            break;

        case 'r':
            if (p.empty_col > 0)
            {
                int tile = p.board[p.empty_row][p.empty_col - 1];
                p.board[p.empty_row][p.empty_col] = tile;
                p.board[p.empty_row][p.empty_col - 1] = 0;
                p.empty_col--;
                p.move_number++;
            }
            break;

        case 'u':
            if (p.empty_row < p.dim - 1)
            {
                int tile = p.board[p.empty_row + 1][p.empty_col];
                p.board[p.empty_row][p.empty_col] = tile;
                p.board[p.empty_row + 1][p.empty_col] = 0;
                p.empty_row++;
                p.move_number++;
            }
            break;

        case 'd':
            if (p.empty_row > 0)
            {
                int tile = p.board[p.empty_row - 1][p.empty_col];
                p.board[p.empty_row][p.empty_col] = tile;
                p.board[p.empty_row - 1][p.empty_col] = 0;
                p.empty_row--;
                p.move_number++;
            }
            break;
    }

    // If using God mode, provide a pause between each move for animation.
    if (p.puzzle_state == GOD_MODE)
    {
        refresh();
        napms(100);
        draw_board();
    }
}

/*
 * Attempts to slide the given tile number.
 */
void slide_tile(int tile)
{
    // Determine the location of the tile.
    int tile_row = -1;
    int tile_col = -1;

    if (p.empty_col < p.dim - 1)
    {
        if (tile == p.board[p.empty_row][p.empty_col + 1])
        {
            tile_row = p.empty_row;
            tile_col = p.empty_col + 1;
        }
    }
    if (p.empty_col > 0)
    {
        if (tile == p.board[p.empty_row][p.empty_col - 1])
        {
            tile_row = p.empty_row;
            tile_col = p.empty_col - 1;
        }
    }
    if (p.empty_row < p.dim - 1)
    {
        if (tile == p.board[p.empty_row + 1][p.empty_col])
        {
            tile_row = p.empty_row + 1;
            tile_col = p.empty_col;
        }
    }
    if (p.empty_row > 0)
    {
        if (tile == p.board[p.empty_row - 1][p.empty_col])
        {
            tile_row = p.empty_row - 1;
            tile_col = p.empty_col;
        }
    }

    // Providing we found the tile, make the move.
    if (tile_row != -1 && tile_col != -1)
    {
        p.board[p.empty_row][p.empty_col] = tile;
        p.board[tile_row][tile_col] = 0;
        p.empty_row = tile_row;
        p.empty_col = tile_col;
        p.move_number++;
    }

    // If using God mode, provide a pause between each move for animation.
    if (p.puzzle_state == GOD_MODE)
    {
        refresh();
        napms(100);
        draw_board();
    }
}

/*
 * Returns true if and only if puzzle is solved.
 */
bool is_solved(void)
{
    // Check empty tile in correct place.
    if (p.board[p.dim - 1][p.dim - 1] != 0)
    {
        return false;
    }

    // Check the other tiles.
    for (int row = 0; row < p.dim; row++)
    {
        for (int col = 0; col < p.dim; col++)
        {
            // Check if tiles match the solved pattern (excluding the empty).
            if (p.board[row][col] != row * p.dim + col + 1 &&
                (row != p.dim - 1 || col != p.dim - 1))
            {
                return false;
            }
        }
    }

    // All tiles passed.
    return true;
}

/*
 * For 3x3 puzzles, uses the current arrangement of the board's tiles to
 * return a rank number for that board. More specifically, this function
 * provides a bijection from permutations of tiles [0...8] on the board to
 * integers in the range [0...(9!-1)].
 */
int permuation_rank(void)
{
    // A full explanation of how this function works is provided in
    // generate_dim3_solutions.c. The only difference here is that the our
    // puzzle board is a 2d array.

    // We require two auxillary arrays initialised as [0...8].
    int positions[DIM3_NUM_TILES];
    int numbers[DIM3_NUM_TILES];
    for (int i = 0; i < DIM3_NUM_TILES; i++)
    {
        positions[i] = i;
        numbers[i] = i;
    }

    // The rank number we will produce.
    int rank = 0;
    // The place value in decimal for the factoradic digit under consideration.
    int m = 1;

    for (int i = 0; i < DIM3_NUM_TILES - 1; i++)
    {
        // Get the index of numbers where the puzzle board tile is located.
        int pos = positions[p.board[i / p.dim][i % p.dim]];
        // Get the last unseen element of numbers.
        int last = numbers[DIM3_NUM_TILES - 1 - i];
        // Now replace the board element in numbers by last and update the
        // positions array according.
        numbers[pos] = last;
        positions[last] = pos;

        // Add the decimal contribution of the factoradic digit to the rank
        // number so far.
        rank += m * pos;
        m *= DIM3_NUM_TILES - i;
    }

    return rank;
}

/*
 * Loads from DIM3_SOLUTIONS_FILE an array containing a solution graph for 3x3
 * puzzles and returns a pointer to that array. In case of any errors returns
 * NULL.
 */
uint8_t *load_dim3_solutions(void)
{
    uint8_t *dim3_array = malloc(DIM3_NUM_BOARDS);
    if (!dim3_array)
    {
        return NULL;
    }

    // Load 3x3 solutions data.
    FILE *fp = fopen(DIM3_SOLUTIONS_FILE, "rb");
    if (!fp)
    {
        return NULL;
    }

    if (fread(dim3_array, DIM3_NUM_BOARDS, 1, fp) != 1)
    {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    return dim3_array;
}

/*
 * Provides the automatic solver, 'God mode'. From the current state of the
 * puzzle, calls a series of moves until the puzzle is solved. Returns true
 * upon success, false otherwise.
 */
bool god_mode(uint8_t **dim3_array, uint8_t **dim4_array)
{
    // Reset the move counter to provide the number of moves the solver used.
    p.move_number = 0;
    // Set the puzzle state and track whether the solver provided an optimal
    // solution.
    p.puzzle_state = GOD_MODE;
    draw_board();
    refresh();
    bool optimally = false;

    // Although very unlikely to be used, the following optimally solves the
    // 2x2 case and is a useful first step for testing god mode.
    if (p.dim == 2)
    {
        // Form an 4d array of the optimal moves to make for each possible
        // configuration of tiles, so moves[i][j][k][l] means an i top left,
        // a j top right, k bottom left, l bottom right.
        char d[4][4][4][4];
        d[1][2][3][0] = 0;   d[1][0][3][2] = 'u'; d[0][1][3][2] = 'l';
        d[3][1][0][2] = 'd'; d[3][1][2][0] = 'r'; d[3][0][2][1] = 'u';
        d[0][3][2][1] = 'u'; d[2][3][0][1] = 'l'; d[2][3][1][0] = 'd';
        d[2][0][1][3] = 'r'; d[0][2][1][3] = 'u'; d[1][2][0][3] = 'l';

        while (d[p.board[0][0]][p.board[0][1]][p.board[1][0]][p.board[1][1]])
        {
            slide(d[p.board[0][0]][p.board[0][1]][p.board[1][0]][p.board[1][1]]);
        }
        optimally = true;
    }

    // Try and use the optimal solver for the 3x3 case.
    else if (p.dim == 3)
    {
        // Check whether we have already loaded solutions for 3x3 puzzles.
        if (!*dim3_array)
        {
            *dim3_array = load_dim3_solutions();
        }

        if (*dim3_array)
        {
            // If successful, determine an index into the array by producing a
            // rank number based on the board's current tile arrangment. Then
            // make the move corresponding to the tile number located in the
            // array at that index. Continue until we reach the sentinel value
            // which corresponds to the solved board.
            int tile;
            while ((tile = (*dim3_array)[permuation_rank()]) != DIM3_NUM_TILES)
            {
                slide_tile(tile);
            }
            optimally = true;
        }
    }

    // For larger puzzle sizes or if the 3x3 optimal solver failed.
    if (!is_solved())
    {
        // Iterate over unsolved row-column pairs using the non-optimal general
        // solver, until we are down to the 4x4 lower-right corner of the
        // puzzle at which point we can try using the optimal 4x4 solver if it
        // is available, else continue with the non-optimal general solver.
        for (int offset = 0; offset < p.dim - 1; offset++)
        {
            // If we are in a position to use the 4x4 optimal solver.
            if (p.dim - offset == 4)
            {
                // Check whether we have already loaded heuristics for 4x4
                // puzzles.
                if (!*dim4_array)
                {
                    *dim4_array = load_dim4_heuristics();
                }

                // If so, use the 4x4 optimal solver on the unsolved
                // lower-right 4x4 corner of the board.
                if (*dim4_array)
                {
                    // Display a message in case the solver takes a long time.
                    p.puzzle_state = BUSY;
                    draw_board();
                    refresh();
                    p.puzzle_state = GOD_MODE;
                    // Call the solver.
                    dim4_solver(offset, *dim4_array);
                }

                // Check for success.
                if (is_solved())
                {
                    // If the puzzle was 4x4 we have an optimal solution.
                    if (p.dim == 4)
                    {
                        optimally = true;
                    }
                    break;
                }
            }

            // Place the tiles in row offset in the correct locations.
            arrange_row(offset);
            if (offset == p.dim - 2)
            {
                // There is no enough room to arrange the tiles in the second
                // to last column but this is actually just one tile, the last
                // one. If needed, move it to the correct location.
                if (p.board[p.dim - 1][p.dim - 1] != 0)
                {
                    slide('l');
                }
                break;
            }
            // Place the tiles in column offset in the correct locations.
            arrange_column(offset);
        }
    }

    // Make a final check and change the puzzle_state.
    if (is_solved())
    {
        if (optimally)
        {
            p.puzzle_state = GOD_SOLVED_OPTIMAL;
        }
        else
        {
            p.puzzle_state = GOD_SOLVED;
        }
        return true;
    }
    return false;
}

