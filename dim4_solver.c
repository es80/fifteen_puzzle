/**
 * dim4_solver.c
 *
 * This file defines the functions required for optimal solutions to 4x4
 * fifteen puzzles (or the 4x4 lower right corner of a larger puzzle).
 *
 * The method used is an A* iterative deepening search using a heuristic
 * previously generated by the program generate_dim4_heuristics.c.
 *
 * From the initial puzzle position, we use the heuristic (a minimum number of
 * moves to the solution state) as a limit for a depth first search of the
 * graph generated from successive valid moves of tiles. For each state
 * reached, if the number of moves made plus the heuristic value for that state
 * exceeds the limit, we cut off that search branch. If the search finishes
 * without sucess we increase the limit to be equal to the least of all the
 * costs that were cut off last time round and then start the search again.
 *
 * Since the heuristic never overestimates the actual cost to reach the
 * solution, the first solution we find will be optimal in the total number of
 * moves required.
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dim4.h"
#include "fifteen.h"

// Encapsulate the current state of the board, including the moves made since
// initialization, with a struct node.
typedef struct
{
    int board[DIM4_NUM_TILES];
    int empty_index;
    int heuristic;
    int num_moves;
    // A solution will have at most 80 moves.
    // http://www.iro.umontreal.ca/~gendron/Pisa/References/BB/Brungger99.pdf
    int moves[80];
}
node;

// A global to track when the puzzle becomes solved to help back out of the
// recursive search.
static bool solved;

/*
 * Starting from the given node, and using heuristics provided by dim4_array,
 * performs a depth first search cutting off search branches when the
 * estimated costs exceed the bound. Once complete, returns the least bound
 * that could be used for another such search at a greater depth. If at any
 * point the search reaches the goal state, the search terminates having saved
 * the solution path in the node n.
 */
int depth_first_search(node *n, int bound, uint8_t *dim4_array);

/*
 * Loads heuristic values from disk into an array and returns a pointer to that
 * array.
 */
uint8_t *load_dim4_heuristics(void);

/*
 * For a given board of tiles, retrieves the heuristic for that board from the
 * array of heuristic values.
 */
int get_heurisitic(uint8_t *dim4_array, int board[DIM4_NUM_TILES]);

/*
 * For a given board of tiles and a tile pattern returns a unique index based
 * on where the tiles in the pattern are on the board. The index will be
 * greater than or equal to 0 and less than 16^n, where n is the number of
 * tiles in the pattern.
 */
int arr_index(int board[DIM4_NUM_TILES], tile_pattern pattern, bool reflected);

/*
 * Given an offset so that we can identify a the 4x4 lower right corner of the
 * puzzle, and given an array of heuristic values, calls successive
 * heuristic-guided depth-first searches until an optimal solution is found to
 * arrange the 4x4 tiles correctly. Returns true on success. Otherwise returns
 * false.
 */
bool dim4_solver(int board_offset, uint8_t *dim4_array)
{
    // Setup a root node.
    node *root = malloc(sizeof(node));
    if (!root)
    {
        return false;
    }

    // Read in the 4x4 lower right corner of the puzzle board and adjust the
    // tile numbers to be in the range 1-15.
    int i = 0;
    for (int row = board_offset; row < p.dim; row++)
    {
        for (int col = board_offset; col < p.dim; col++)
        {
            if (p.board[row][col] == 0)
            {
                root->empty_index = i;
                root->board[i++] = 0;
            }
            else
            {
                int pos = p.board[row][col] - 1;
                int adjusted_row = (pos / p.dim) - board_offset;
                int adjusted_col = (pos % p.dim) - board_offset;
                root->board[i++] = (adjusted_row * DIM4) + adjusted_col + 1;
            }
        }
    }
    root->num_moves = 0;
    root->heuristic = get_heurisitic(dim4_array, root->board);

    // Use the heuristic as the initial bound for successive A* depth-first
    // searches.
    solved = false;
    int bound = root->heuristic;
    while (!solved)
    {
        bound = depth_first_search(root, bound, dim4_array);
        if (bound == INT_MAX)
        {
            return false;
        }
    }

    // The solution's tile moves are for tiles from 1 to 15. First locate the
    // corresponding tile in the original puzzle according to the offset, then
    // make the move for that tile.
    for (int i = 0; i < root->num_moves; i++)
    {
        int adjusted_row = (root->moves[i] - 1) / DIM4;
        int adjusted_col = (root->moves[i] - 1) % DIM4;
        int tile = (adjusted_row + board_offset) * p.dim + (adjusted_col + board_offset) + 1;
        slide_tile(tile);
    }

    free(root);
    return true;
}

/*
 * Starting from the given node, and using heuristics provided by dim4_array,
 * performs a depth first search cutting off search branches when the
 * estimated costs exceed the bound. Once complete, returns the least bound
 * that could be used for another such search at a greater depth. If at any
 * point the search reaches the goal state, the search terminates having saved
 * the solution path in the node n.
 */
int depth_first_search(node *n, int bound, uint8_t *dim4_array)
{
    // Check for solved state.
    if (n->heuristic == 0)
    {
        solved = true;
        return 0;
    }

    // Look for a new bound to return.
    int new_bound = INT_MAX;

    // For each neighbour node.
    for (int i = 0; i < 4; i++)
    {
        int move_index = valid_moves[n->empty_index][i];

        // If we have a valid move.
        if (move_index != -1)
        {
            int tile = n->board[move_index];

            if (tile != n->moves[n->num_moves - 1])
            {
                int old_empty_index = n->empty_index;

                // Track the old heuristic value.
                int old_heuristic = n->heuristic;

                // Make the move by updating the node n.
                n->board[n->empty_index] = tile;
                n->board[move_index] = 0;
                n->empty_index = move_index;

                // Get the new heuristic after the move.
                n->heuristic = get_heurisitic(dim4_array, n->board);

                // Add the move to the moves list.
                n->moves[n->num_moves] = tile;
                n->num_moves += 1;

                // The new bound.
                int b = 1 + n->heuristic;

                if (b <= bound)
                {
                    // Search deeper.
                    b = 1 + depth_first_search(n, bound - 1, dim4_array);
                }

                // The puzzle is solved so back out of recursion.
                if (solved)
                {
                    return b;
                }

                // Take the minimum of the b as the next bound.
                if (b < new_bound)
                {
                    new_bound = b;
                }

                // Undo the move in preparation for the next neighbour.
                n->board[move_index] = tile;
                n->board[old_empty_index] = 0;
                n->empty_index = old_empty_index;

                // Restore the node's old heuristic.
                n->heuristic = old_heuristic;

                // Take the move off the moves list.
                n->num_moves -= 1;
                n->moves[n->num_moves] = 0;

            }
        }
    }
    return new_bound;
}

/*
 * Loads heuristic values from disk into an array and returns a pointer to that
 * array.
 */
uint8_t *load_dim4_heuristics(void)
{
    uint8_t *dim4_array = malloc(TOTAL_STATES);
    if (!dim4_array)
    {
        return NULL;
    }

    // Load heuristics data.
    FILE *fp = fopen(DIM4_HEURISTICS_FILE, "rb");
    if (!fp)
    {
        if (dim4_array)
        {
            free(dim4_array);
        }
        return NULL;
    }

    if (fread(dim4_array, TOTAL_STATES, 1, fp) != 1)
    {
        if (dim4_array)
        {
            free(dim4_array);
        }
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    return dim4_array;
}

/*
 * For a given board of tiles, retrieves a heuristic for that board from the
 * array of heuristic values.
 */
int get_heurisitic(uint8_t *dim4_array, int board[DIM4_NUM_TILES])
{
    int index;
    int heuristic = 0;
    int reflected_heuristic = 0;
    // Sum the heuristic values for each pattern.
    for (int i = 0; i < NUM_PATTERNS; i++)
    {
        // First get the normal heuristic.
        index = arr_index(board, patterns[i], false);
        heuristic += dim4_array[index + patterns[i].array_offset];

        // Now calculate the heuristic for the board reflected about the main
        // diagonal.
        index = arr_index(board, patterns[i], true);
        reflected_heuristic += dim4_array[index + patterns[i].array_offset];
    }

    // Take the maximum of the two heuristics.
    if (heuristic < reflected_heuristic)
    {
        heuristic = reflected_heuristic;
    }

    return heuristic;
}

/*
 * For a given board of tiles and a tile pattern returns a unique index based
 * on where the tiles in the pattern are on the board. The index will be
 * greater than or equal to 0 and less than 16^n, where n is the number of
 * tiles in the pattern.
 */
int arr_index(int board[DIM4_NUM_TILES], tile_pattern pattern, bool reflected)
{
    int index = 0;
    int k = 1;

    if (reflected)
    {
        for (int i = 0; i < pattern.num_tiles; i++)
        {
            for (int j = 0; j < DIM4_NUM_TILES; j++)
            {
                // If we are computed a heuristic for the board reflected along
                // its main diagonal we use the corresponding reflected patten
                // and must compute the location of the tile under the
                // reflection to get the index to lookup.
                if (pattern.reflected_tiles[i] == board[j])
                {
                    index += (DIM4 * j - ((DIM4_NUM_TILES - 1) * (j / DIM4))) * k;
                    k *= DIM4_NUM_TILES;
                    break;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < pattern.num_tiles; i++)
        {
            for (int j = 0; j < DIM4_NUM_TILES; j++)
            {
                if (pattern.tiles[i] == board[j])
                {
                    index += j * k;
                    k *= DIM4_NUM_TILES;
                    break;
                }
            }
        }
    }

    return index;
}

