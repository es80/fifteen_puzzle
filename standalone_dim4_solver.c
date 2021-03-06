/**
 * standalone_dim4_solver.c
 *
 * This program reads lines from stdin and attempts to parse each line as a
 * list of 16 tile numbers which form a valid 4x4, fifteen puzzle. Upon success
 * it outputs the optimal number of moves required to solve the puzzle and a
 * list of the tiles to move.
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

#define _GNU_SOURCE

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dim4.h"

// Minimum number of characters for a line of text to be a valid puzzle.
#define MINIMUM_CHARS 37

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
 * Given a board of tiles and an array of heuristic values, calls successive
 * heuristic-guided depth-first searches until a solution is found to the
 * puzzle. Prints the solution and returns true. Otherwise returns false.
 */
bool dim4_solver(uint8_t *dim4_array, int board[DIM4_NUM_TILES]);

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
 * Returns true if and only if the puzzle represented by board is solved.
 */
bool is_solved(int board[DIM4_NUM_TILES]);

/*
 * Swaps the contents of array[i] and array[j], increments a swap counter.
 */
void swap(int array[], int i, int j, int *swap_count);

/*
 * Quicksorts an array between indices low and high inclusive. Also counts the
 * number of swaps made whilst sorting.
 */
void quicksort(int array[], int low, int high, int *swap_count);

/*
 * Returns true if array represents a solvable puzzle, returns false otherwise.
 */
bool is_solvable(int array[]);


int main(void)
{
    // Load the heuristic values into an array.
    uint8_t *dim4_array = load_dim4_heuristics();
    if (!dim4_array)
    {
        return 1;
    }

    // Continuously read lines from stdin.
    char *line = NULL;
    size_t line_len = 0;
    ssize_t num_chars = 0;
    char *endptr = NULL;
    int board[DIM4_NUM_TILES] = {0};

    while ((num_chars = getline(&line, &line_len, stdin)) != -1)
    {
        // Remove trailing newlines.
        while (line[num_chars - 1] == '\r' || line[num_chars - 1] == '\n')
        {
            line[num_chars] = 0;
            num_chars--;
        }

        // Ensure we have a minimum number of characters that we might have a
        // valid puzzle.
        if (num_chars >= MINIMUM_CHARS)
        {
            char *p = line;
            int i = 0;
            while (true)
            {
                board[i] = (int) strtol(p, &endptr, 10);
                if ((board[i] < 0 || board[i] > 15) || (p == endptr)
                    || (i >= DIM4_NUM_TILES))
                {
                    break;
                }
                p = endptr;
                i++;
            }
            // Verify we have a solvable puzzle, then call the solver.
            if (i == DIM4_NUM_TILES && is_solvable(board))
            {
                if (!dim4_solver(dim4_array, board))
                {
                    break;
                }
            }
        }
    }

    free(dim4_array);
    if (line)
    {
        free(line);
    }

    return 0;
}

/*
 * Given a board of tiles and an array of heuristic values, calls successive
 * heuristic-guided depth-first searches until a solution is found to the
 * puzzle. Prints the solution and returns true. Otherwise returns false.
 */
bool dim4_solver(uint8_t *dim4_array, int board[DIM4_NUM_TILES])
{
    // Setup a root node.
    node *root = malloc(sizeof(node));
    if (!root)
    {
        return false;
    }
    for (int i = 0; i < DIM4_NUM_TILES; i++)
    {
        if (board[i] == 0)
        {
            root->empty_index = i;
        }
        root->board[i] = board[i];
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

    // Print the number of moves and optionally the moves themselves required
    // to solve the puzzle.
    bool print_moves = true;
    if (is_solved(root->board))
    {
        if (print_moves)
        {
            printf("%i moves: ", root->num_moves);
            for (int i = 0; i < root->num_moves; i++)
            {
                printf("%i ", root->moves[i]);
            }
            printf("\n");
        }
        else
        {
            printf("%i\n", root->num_moves);
        }
    }
    else
    {
        printf("Error!\n");
        return false;
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

/*
 * Returns true if and only if puzzle is solved.
 */
bool is_solved(int board[DIM4_NUM_TILES])
{
    // Check empty tile in correct place.
    if (board[DIM4_NUM_TILES - 1] != 0)
    {
        return false;
    }

    // Check the other tiles.
    for (int i = 0; i < DIM4_NUM_TILES - 1; i++)
    {
        // Check if tiles match the solved pattern (excluding the empty).
        if (board[i] != i + 1)
        {
            return false;
        }
    }

    return true;
}

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
 * Returns true if array represents a solvable puzzle, returns false otherwise.
 */
bool is_solvable(int board[])
{
    // To ensure a valid solvable puzzle, the parity of the permutation of the
    // tiles 1 to 16 (with the empty tile being 16) plus the parity of the
    // taxicab distance (number of rows plus number of columns) of the empty
    // tile from the lower right corner, must be even. (This is an invariant
    // for the puzzle moves.)

    // Find the index of the empty tile.
    int i;
    for (i = 0; i < DIM4_NUM_TILES; i++)
    {
        if (board[i] == 0)
        {
            break;
        }
    }

    // Copy the board and change the empty tile to a 16.
    int array[DIM4_NUM_TILES];
    memcpy(array, board, sizeof (int) * DIM4_NUM_TILES);
    array[i] = DIM4_NUM_TILES;

    // Quicksort the array and count the swaps.
    int swap_count = 0;
    quicksort(array, 0, DIM4_NUM_TILES - 1, &swap_count);
    // Determine taxicab distance of empty tile.
    int taxicab_dist = DIM4 - 1 - (i % DIM4) + DIM4 - 1 - (i / DIM4);

    return (swap_count + taxicab_dist) % 2 == 0;
}

