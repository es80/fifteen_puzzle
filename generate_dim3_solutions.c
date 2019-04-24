/**
 * generate_3x3_solution.c
 *
 * This program generates a set of optimal solutions for the 3x3 sliding tile
 * puzzle and saves them to disk as 'dim3_solutions.bin'.
 *
 * The 3x3 puzzle board is represented as a one dimensional array of length 9
 * by reading the board left-to-right, top-to-bottom. The tiles are numbered 1
 * to 8 with a 0 representing the empty tile. Starting from the solved state
 * [1,2,3,4,5,6,7,8,0] we perform a breadth first search by making moves on the
 * board until we have explored every possible board configuration.
 *
 * A valid move is one where a tile is able to slide into the place occupied by
 * the empty tile swapping the respective values in the array. For example,
 * from the starting state there is a choice of two possible moves:
 *    [1,2,3,4,5,6,7,8,0]  -->  [1,2,3,4,5,6,7,0,8]  tile 8 moved
 *    [1,2,3,4,5,6,7,8,0]  -->  [1,2,3,4,5,0,7,8,6]  tile 6 moved
 *
 * As we make moves in the breadth first search, whenever we generate a new
 * board configuration (one that we haven't visited before) we will look to
 * save the state of the board and the tile that was moved to reach that state.
 * As a result, once the search is complete, we have saved a 'solution graph'.
 * Given any valid board position as an input we could solve the puzzle
 * optimally by looking up that board in the solution graph, moving the tile
 * saved with that board, then looking up the resultant board and moving the
 * tile saved with that board and so on and so on. Eventually we will have
 * traced the shortest path back to the solved state.
 *
 * To efficiently save (and in future lookup) the states of the board we can
 * consider the board as a permutation of the digits [0..8] and compute a
 * ranking number for that permuation (an integer in the range [0..9!-1]). We
 * use something similar to Lehmer codes to do this. Then we use that rank as
 * an index into an array where each element is just the tile that was moved in
 * reaching that board state.
 *
 * The resulting array size is 362,880 bytes corresponding to the 9! possible
 * permuations of tiles numbers 0 to 8. This could be considerably compressed
 * if desired. For example, only half of these permuations will actually be
 * valid states of the puzzle and the moves made are always numbers 1-8.
 * Alternatively one could store moves by just their direction: left, right,
 * up or down requiring only 2 bits each.
 *
 * References:
 * I. Parberry, "A Memory-Efficient Method for Fast Computation of Short
 * 15-Puzzle Solutions", IEEE Transactions on Computational Intelligence and AI
 * in Games, Vol. 7, No. 2, pp. 200-203, June 2015.
 * E. F. Moore, "The shortest path through a maze," in Proceedings of the
 * International Symposium on the Theory of Switching, 1959, pp. 285–292.
 * C. Lee, "An algorithm for path connection and its applications," IRE
 * Transactions on Electronic Computers, vol. EC-10, no. 3, pp. 346–365, 1961.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DIM3 3
#define DIM3_NUM_TILES 9
// There are 9! = 362,880 permutations of tiles numbered 0 to 8, (only half of
// which will actually be valid states of the puzzle board).
#define DIM3_NUM_BOARDS 362880
#define DIM3_SOLUTIONS_FILE "dim3_solutions.bin"

// The current state of the puzzle is encapsulated in a node. Nodes will be
// stored in a queue implemented as a linked list.
typedef struct node
{
    int board[DIM3_NUM_TILES];   // The current array for the board tiles.
    int empty_index;             // The index of the empty tile.
    int tile;                    // The last tile moved to reach this state.
    struct node *next;           // The next node in the queue.
}
node;

/*
 * Takes a node and adds it to the back of the queue.
 */
void enqueue(node *n, node **front, node **back);

/*
 * Removes and returns a node from the front of the queue.
 */
node *dequeue(node **front);

/*
 * For a given array representing the arrangement of the board's tiles, returns
 * a rank number for for that board. More specifically, this function is a
 * bijection from the set of permutations of the numbers [0...8] to integers in
 * the range [0...(9!-1)].
 */
int permuation_rank(int board[DIM3_NUM_TILES]);

int main(void)
{
    // To be able to quickly lookup what moves are legal from a given position
    // we create a 2d array, valid_moves, such that there is a row for the 9
    // possible indices of the empty tile and a column for the, up to, 4
    // possilbe indices of a tile which could be moved. When there are less
    // than 4 moves available we can use -1 as a sentinel value to indicate
    // this.
    // For example consider the indices of the board array laid out as a 3x3
    // board:
    //
    //  0 1 2      If the empty tile is at index 5, we could move a tile down
    //  3 4 5      from index 2, we couldn't move a tile left, we could move a
    //  6 7 8      tile up from index 8 or move a tile right from index 4.
    //             Thus valid_moves[5] = {2, -1, 8, 4}.
    //
    int valid_moves[DIM3_NUM_TILES][4];
    for (int i = 0; i < DIM3_NUM_TILES; i++)
    {
        // Move a tile down unless empty tile on top row.
        valid_moves[i][0] = i >= DIM3 ? i - DIM3 : -1;
        // Move a tile left unless empty tile on rightmost column.
        valid_moves[i][1] = i % DIM3 != DIM3 - 1 ? i + 1 : -1;
        // Move a tile up unless empty tile on bottom row.
        valid_moves[i][2] = i < DIM3 * (DIM3 - 1) ? i + DIM3 : -1;
        // Move a tile right unless empty tile on leftmost column.
        valid_moves[i][3] = i % DIM3 ? i - 1 : -1;
    }

    // Initialise an array to store the results of our search. The indices of
    // this array are the permuation ranks for a given arrangement of the
    // board's tiles. The element stored is the number of the last tile moved.
    // A zero represents that this board has no yet been seen in the search.
    uint8_t dim3_array[DIM3_NUM_BOARDS] = {0};

    // Initialise a root node for our search.
    node *root = malloc(sizeof(node));
    if (!root)
    {
        return 1;
    }
    // The root node represents a solved puzzle meaning the board has tiles 1-8
    // in order, then the empty tile.
    for (int i = 0; i < DIM3_NUM_TILES; i++)
    {
        root->board[i] = i == DIM3_NUM_TILES - 1 ? 0 : i + 1;
    }
    root->empty_index = DIM3_NUM_TILES - 1;
    // We need a sentinel value (not in 0-8) to represent that the last tile
    // moved to reach this position is 'none'.
    root->tile = DIM3_NUM_TILES;

    // We can now define a front and back for a queue of nodes and enqueue the
    // root node.
    node *front = root;
    node *back = root;
    // Then we mark this node as seen by storing the last tile moved in
    // dim3_array.
    dim3_array[permuation_rank(root->board)] = root->tile;

    // Now start the breadth first search.
    while (front)
    {
        node *n = dequeue(&front);

        // For each possible neighbour of n (up to 4 possible moves).
        for (int i = 0; i < 4; i++)
        {
            // Lookup a valid move.
            int move_index = valid_moves[n->empty_index][i];

            // Check the move is indeed valid.
            if (move_index != -1)
            {
                // The tile being moved.
                int tile = n->board[move_index];

                // Save time by checking this move isn't just a repeat of the
                // last one.
                if (tile != n->tile)
                {
                    // Make the move on the board.
                    n->board[n->empty_index] = tile;
                    n->board[move_index] = 0;

                    // If we haven't already seen this board.
                    if (!dim3_array[permuation_rank(n->board)])
                    {
                        // Initialise a new node.
                        node *new = malloc(sizeof(node));
                        if (!new)
                        {
                            return 1;
                        }
                        for (int j = 0; j < DIM3_NUM_TILES; j++)
                        {
                            new->board[j] = n->board[j];
                        }
                        new->tile = tile;
                        new->empty_index = move_index;

                        // Enqueue this node and mark it as seen by recording
                        // the tile moved.
                        enqueue(new, &front, &back);
                        dim3_array[permuation_rank(new->board)] = new->tile;
                    }

                    // Undo the move before looking at the next neighbour.
                    n->board[n->empty_index] = 0;
                    n->board[move_index] = tile;
                }
            }
        }

        // We are now done with this node.
        free(n);
    }

    // Now the search is complete, write the array to disk.
    FILE *file = fopen(DIM3_SOLUTIONS_FILE, "wb");
    if (!file)
    {
        return 1;
    }
    if (fwrite(dim3_array, DIM3_NUM_BOARDS, 1, file) != 1)
    {
        fclose(file);
        return 1;
    }
    fclose(file);

    return 0;
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

/*
 * For a given array representing the arrangement of the board's tiles, returns
 * a rank number for that board. More specifically, this function is a
 * bijection from the set of permutations of the numbers [0...8] to integers in
 * the range [0...(9!-1)].
 */
int permuation_rank(int board[DIM3_NUM_TILES])
{
    // We could implement this function using the Lehmer code (1) to rank each
    // possible permutation of the board's tiles. That is an O(n^2) algorithm.
    // There exist much better approaches, see (2) for an overview.
    // Since we do not require a lexicographic ordering and we want a simple
    // implementation, we use a linear time algorithm much like (3) but taken
    // from (4),(5). It reduces quadratic time to linear time using a similar
    // swapping trick to the/ Fisher-Yates shuffle (6).
    //
    // 1. https://en.wikipedia.org/wiki/Lehmer_code
    // 2. Efficient Algorithms to Rank and Unrank Permutations in Lexicographic
    //    Order (Bonet)
    // 3. Ranking and Unranking Permutations in Linear Time (Myrvold and
    //    Ruskey)
    // 4. https://stackoverflow.com/a/24689277
    // 5. http://antoinecomeau.blogspot.com/2014/07/mapping-between-permutations-and.html
    // 6. https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#The_modern_algorithm
    //
    // Much like the Lehmer code this algorithm generates a rank for a
    // permutation by iterating over the elements of the input array and
    // generating a factoradic number (a number in factorial base), which is
    // converted to decimal as we go. The difference to Lehmer code is that the
    // order in which the permutations sit according to the ranking is not
    // lexicographic.
    //
    // We use two auxillary arrays: numbers is a copy of the numbers we are
    // permuting and positions will contain the indices where elements of the
    // input array are to be found in the numbers array.
    //
    // As we process each element of the input array, we locate it in the
    // numbers array using the positions array to provide the right index.
    //
    // These indices provide the digits of the factoradic number associated
    // with the permuation.
    //
    // Then we update the numbers array by swapping the last element (of those
    // not yet seen) with the current element and update the positions array to
    // reflect this change.
    //
    // For example, with input array [2,0,1,4,3]
    //
    //            input number   factoradic digit    numbers      positions
    //    start                                    [0,1,2,3,4]   [0,1,2,3,4]
    //                 2                 2         [0,1,4,3,-]   [0,1,-,3,2]
    //                 0                 0         [3,1,4,-,-]   [-,1,-,0,2]
    //                 1                 1         [3,4,-,-,-]   [-,-,-,0,1]
    //                 4                 1         [3,-,-,-,-]   [-,-,-,0,-]
    //                 3                 0         [-,-,-,-,-]   [-,-,-,-,-]
    // (where a '-' means we can now ignore this member of the array)
    //
    // The invariant maintained in the loop is that input[i], will be located
    // at index positions[input[i]] in the numbers array, that is,
    // numbers[positions[input[i]]] == input[i].
    //
    // We obtain the permutation rank by converting the factoradic number
    // 20110 to a unique integer in the range 0 to (n!-1). This can be done as
    // 2x4! + 0x3! + 1x2! + 1x1! + 0x0! (giving 51) or can also be done as
    // 2x(5!/5!) + 0x(5!/4!) + 1x(5!/3!) + 1x(5!/2!) + 0x(5!/1!) (giving 82).
    // This algorithm uses the second method on-the-fly within the loop.
    //
    // Note the final iteration of the loop is not necessary since the digit is
    // always 0.

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
        // Get the index of numbers where board[i] is located.
        int pos = positions[board[i]];
        // Get the last unseen element of numbers.
        int last = numbers[DIM3_NUM_TILES - i - 1];
        // Now replace the board[i] element in numbers by last and update the
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

