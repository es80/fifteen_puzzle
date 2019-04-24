/**
 * generate_dim4_heuristics.c
 *
 * This program generates an array of heuristic values which may be used to
 * help guide searchs for optimal solutions to the 4x4 sliding tile puzzle
 * fifteen.
 *
 * In theory one could search for optimal solutions by performing a breadth
 * first search of the solution graph - the graph whose nodes are states of the
 * board connected by edges if there is a tile move taking one state to
 * another. However the space requirements are not practical since there are
 * 16!/2 valid solvable states of the board meaning terabytes of space at best.
 * Using an informed search like A* does little to counter the impact of the
 * space requirements.
 *
 * A depth first search could be performed to avoid excessive space
 * requirements but would clearly not necessarily terminate nor be optimal.
 *
 * Instead, an iterative deepening search is required where we perform
 * successive depth-first searchs of the graph up to a limited fixed depth,
 * increasing that limit with each iteration. That way we get an optimal search
 * but without the space requirements of breath-first search.
 *
 * Performance is still generally very slow but can be much improved by using a
 * heuristic to estimate the cost to the goal. This is the A* iterative
 * deepening search [1]. Branches of the iterative deepening search can then be
 * cut off early using the heuristic as part of the limit for each iteration.
 * If we do this search as a tree search (not keeping track of closed visited
 * nodes saving on space requirements) then the heuristic needs to be
 * admissible for an optimal solution meaning it never overestimates the cost
 * to the goal.
 *
 * There are a number [2] of heuristics for the sliding tile puzzle including
 * Hamming distance [3], taxi-cab L^1 distance [4], walking distance [5],
 * inversion distance [6], pairwise distance [7] and pattern databases [8]. We
 * use additive pattern databases for the heuristic since they are fairly easy
 * to implement and solve most puzzles quickly. This program was written using
 * [7], [8] and [9].
 *
 * The idea behind additive pattern databases is to split the 15 numbered tiles
 * into disjoint 'tile patterns'. For example: [1,5,6,9,10,13] [2,3,4] and
 * [7,8,11,12,14,15]. Consider the first tile pattern and imagine we treat all
 * the other tiles as indistinguishable. In the solved state our puzzle would
 * look like:
 *                 1  x  x  x
 *                 5  6  x  x
 *                 9 10  x  x
 *                13  x  x  x
 *
 * Imagine that for every possible permutation of just the tiles in this
 * pattern we knew the cost of reaching the solved state, where a move counts 1
 * towards the cost only if it involves moving a tile in the pattern. This
 * would become a minimal number of moves to place these tiles correctly in the
 * actual puzzle we want to solve.
 *
 * Now consider doing the same for the other two tile patterns. Crucially, for
 * each tile pattern our cost only counted moves of the tiles in that pattern.
 * This means that for a given puzzle we could add together the costs for each
 * tile pattern into a single heuristic value which would still not
 * overestimate the true cost of solving the puzzle - i.e. it would be
 * admissible. It would also be feasible to compute these costs since, in the
 * case of a tile pattern of 6 when we consider all the other tiles as
 * indistinguishable, we only have 16! / (16 - 6) ! = 5765760 cost values to
 * save for that pattern.
 *
 * To compute these cost values we need to start from a solved state and do a
 * breadth-first search of all possible states. However, things are made a
 * little more complicated by the fact that to save space we are disregarding
 * the position of the empty tile when we save the costs for a given
 * permutation of the pattern tiles. Therefore, for each permutation, we will
 * want to save only the least cost we find for all possible positions of the
 * empty tile amongst the other tiles.
 *
 * To do this we use a 'visited' pattern of tiles which includes the empty tile
 * and we record the costs in a visited array, ensuring that the search
 * completes having explored every state.
 *
 * For example, with a tile pattern [1,5,6,9,10,13] the visited pattern will be
 * [0,1,5,6,9,10,13]. To complete the search we will need memory to explore
 * 16! / (16 - 7)! = 57657600 nodes but ultimately only write the disk costs
 * for 16! / (16 - 6)! = 5765760 nodes.
 *
 * It would also be possible to simply include the empty tile in each of the
 * tile patterns at the cost of needing to save a much larger number of
 * heuristic values to disk but with the benefit of a slightly better
 * heuristic.
 *
 * To record the costs in an array and write to disk we can use a sparse
 * mapping such as a 6-dimensional array whose 6 indices are indices of each of
 * the tiles 1,5,6,9,10,13. Each entry would be a byte making 16^6 bytes total.
 * Combining the (6-6-3) tile patterns we get a total around 33.5MB. Clearly
 * many array entries will be unused.
 *
 * Or we can use a compact mapping recording the costs in a 1-dimensional array
 * whose indices correspond to ranks of the permuations of the multiset
 * [1,5,6,9,10,13,x,x,x,x,x,x,x,x,x,x], see [10] and [11]. Combining the
 * (6-6-3) tile patterns this gives 16!/10! + 16!/10! + 16!/13! bytes or
 * roughly 11MB.
 *
 * Between the four options of excluding or including the empty tile in the
 * patterns and sparse or compact storage, testing shows trade offs in time and
 * space are best when excluding the empty tile from the patterns and using
 * sparse storage.
 *
 * Constants for the particular tile patterns used are given in 'dim4.h' and
 * the database this program produces is saved as 'dim4_heuristics.bin'.
 *
 * 1. https://en.wikipedia.org/wiki/Iterative_deepening_A*
 * 2. https://codereview.stackexchange.com/a/108631
 * 3. https://en.wikipedia.org/wiki/Hamming_distance
 * 4. https://en.wikipedia.org/wiki/Taxicab_geometry
 * 5. http://www.ic-net.or.jp/home/takaken/e/15pz/wd.gif
 * 6. https://www.aaai.org/Papers/AAAI/2000/AAAI00-212.pdf
 * 7. http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.58.7&rep=rep1&type=pdf
 * 8. https://www.aaai.org/Papers/JAIR/Vol30/JAIR-3006.pdf
 * 9. https://algorithmsinsight.wordpress.com/graph-theory-2/implementing-bfs-for-pattern-database/
 * 10. https://zamboch.blogspot.com/2007/10/ranking-and-unranking-permutations-of.html
 * 11. https://stackoverflow.com/a/14374455
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "dim4.h"

// The current state of the board is encapsulated in a node. These
// nodes will be used for a linked list implementation of a queue.
typedef struct node
{
    uint8_t board[DIM4_NUM_TILES];
    uint8_t empty_index;
    uint8_t heuristic;
    struct node *next;
}
node;

/*
 * Using the given tile pattern, performs a breadth-first search over all
 * possible permuations of the tiles in the pattern and the empty tile,
 * calculating a cost value as it goes and saving those values to the
 * heuristics array. Returns true upon success, false otherwise.
 */
bool bfs_tile_pattern(tile_pattern pattern, uint8_t heuristics[]);

/*
 * For a given board of tiles and a tile pattern returns a unique index based
 * on where the tiles in the pattern are on the board. The index will be
 * greater than or equal to 0 and less than 16^n, where n is the number of
 * tiles in the pattern.
 */
int arr_index(uint8_t board[DIM4_NUM_TILES], tile_pattern pattern);

/*
 * Takes a node and adds it to the back of the queue.
 */
void enqueue(node *n, node **front, node **back);

/*
 * Removes and returns a node from the front of the queue.
 */
node *dequeue(node **front);


int main(void)
{
    // Initialise an array to save all the heuristic values in.
    uint8_t *heuristics = malloc(TOTAL_STATES);
    if (!heuristics)
    {
        return 1;
    }
    for (int i = 0; i < TOTAL_STATES; i++)
    {
        heuristics[i] = UINT8_MAX;
    }

    // For each tile pattern, perform a breadth-first search saving the
    // heuristic values in the heuristics.
    bool success;
    for (int i = 0; i < NUM_PATTERNS; i++)
    {
        success = bfs_tile_pattern(patterns[i], heuristics);
        if (!success)
        {
            if (heuristics)
            {
                free(heuristics);
            }
            return 1;
        }
    }

    // Write the array to disk and free memory.
    FILE *file = fopen(DIM4_HEURISTICS_FILE, "wb");
    if (!file)
    {
        return 1;
    }
    if (fwrite(heuristics, TOTAL_STATES, 1, file) != 1)
    {
        fclose(file);
        return 1;
    }
    fclose(file);

    if (heuristics)
    {
        free(heuristics);
    }

    return 0;
}

/*
 * Using the given tile pattern, performs a breadth-first search over all
 * possible permuations of the tiles in the pattern and the empty tile,
 * calculating a cost value as it goes and saving those values to the
 * heuristics array. Returns true upon success, false otherwise.
 */
bool bfs_tile_pattern(tile_pattern pattern, uint8_t heuristics[])
{
    // Create and initialise a root node.
    node *root = malloc(sizeof (node));
    if (root == NULL)
    {
        return false;
    }
    // Aside from tiles in the pattern and the empty tile, we want other tiles
    // to have the same sentinel value.
    for (int i = 0; i < DIM4_NUM_TILES; i++)
    {
        root->board[i] = UINT8_MAX;
    }
    for (int i = 0; i < pattern.num_tiles; i++)
    {
        root->board[pattern.tiles[i] - 1] = pattern.tiles[i];
    }
    root->empty_index = DIM4_NUM_TILES - 1;
    root->board[root->empty_index] = 0;
    root->heuristic = 0;
    root->next = NULL;

    // Initialise the front and back of our queue of nodes.
    node *front = root;
    node *back = root;

    // Save the heuristic value for the root node in the heuristics array.
    int index = arr_index(root->board, pattern);
    // The index into the array must include an offset so that heuristics for
    // each pattern are saved in a single array.
    heuristics[index + pattern.array_offset] = root->heuristic;

    // When we search we need to track which states are already visited and
    // those states must include the empty tile. We will use a new tile pattern
    // for this which includes 0.
    tile_pattern visited_pattern;
    visited_pattern.num_tiles = pattern.num_tiles + 1;
    visited_pattern.tiles[0] = 0;
    for (int i = 0; i < pattern.num_tiles; i++)
    {
        visited_pattern.tiles[i + 1] = pattern.tiles[i];
    }
    // We don't need the array_offset member since we use a fresh visited array
    // for each search.

    // Initialise an array to save to heuristic values for the visited states.
    uint8_t *visited = malloc(VISITED_STATES);
    if (visited == NULL)
    {
        return false;
    }
    for (int i = 0; i < VISITED_STATES; i++)
    {
        visited[i] = UINT8_MAX;
    }

    // Save the heuristic value for the root node in the visited array. We can
    // use the same arr_index function to get an index provided we now call it
    // with the visited_pattern.
    index = arr_index(root->board, visited_pattern);
    visited[index] = root->heuristic;

    // Note the heuristic values we save in heuristics are each a minimum of
    // those heuristic values we save in the visited array which have the same
    // arrangement of tiles in the pattern but with the empty tile located in
    // different places.

    while (front)
    {
        node *n = dequeue(&front);

        // Find neighbours by looking up valid moves.
        for (int j = 0; j < 4; j++)
        {
            int move_index = valid_moves[n->empty_index][j];
            if (move_index != -1)
            {
                int tile = n->board[move_index];

                // To reduce calls to malloc and memcpy, make the move on the
                // node n first, then undo it later.
                n->board[n->empty_index] = tile;
                n->board[move_index] = 0;

                int heuristic = n->heuristic;
                // Only add to heuristic for moves of tiles in the pattern.
                if (tile != UINT8_MAX)
                {
                    heuristic++;
                }

                index = arr_index(n->board, visited_pattern);

                if (visited[index] <= heuristic)
                {
                    // We've seen this state but it had a lower heuristic. Use
                    // that value instead.
                    heuristic = visited[index];
                }

                else if (visited[index] > heuristic)
                {
                    // Either we've seen this state before but it had a higher
                    // heuristic or this state is unseen. Either way we need a
                    // new node to add to our queue so we can explore this path
                    // further.
                    visited[index] = heuristic;

                    // Create a neighbour node and initialise it.
                    node *neighbour = malloc(sizeof(node));
                    if (!neighbour)
                    {
                        return false;
                    }
                    memcpy(neighbour, n, sizeof (node));
                    neighbour->empty_index = move_index;
                    neighbour->heuristic = heuristic;
                    enqueue(neighbour, &front, &back);
                }

                // Now store the heuristic.
                index = arr_index(n->board, pattern);
                if (heuristics[index + pattern.array_offset] > heuristic)
                {
                    // Take the minimum.
                    heuristics[index + pattern.array_offset] = heuristic;
                }

                // Finally undo the move in preparation for the next move.
                n->board[move_index] = tile;
                n->board[n->empty_index] = 0;
            }
        }
        free(n);
    }
    free(visited);
    return true;
}

/*
 * For a given board of tiles and a tile pattern returns a unique index based
 * on where the tiles in the pattern are on the board. The index will be
 * greater than or equal to 0 and less than 16^n, where n is the number of
 * tiles in the pattern.
 */
int arr_index(uint8_t board[DIM4_NUM_TILES], tile_pattern pattern)
{
    int index = 0;
    int k = 1;
    for (int i = 0; i < pattern.num_tiles; i++)
    {
        for (int j = 0; j < DIM4_NUM_TILES; j++)
        {
            if (pattern.tiles[i] == board[j])
            {
                index += j * k;
                k *= DIM4_NUM_TILES;
            }
        }
    }
    return index;
}

/*
 * Takes a node and adds it to the back of the queue.
 */
void enqueue(node *n, node **front, node **back)
{
    if (*front == NULL)
    {
        n->next = NULL;
        *front = n;
        *back = n;
    }
    else
    {
        (*back)->next = n;
        n->next = NULL;
        *back = n;
    }
}

/*
 * Removes and returns a node from the front of the queue.
 */
node *dequeue(node **front)
{
    if (*front != NULL)
    {
        node *ptr = *front;
        *front = (*front)->next;
        return ptr;
    }
    else
    {
        return NULL;
    }
}

