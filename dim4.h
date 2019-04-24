
#include <stdint.h>

#ifndef DIM4_H
#define DIM4_H

#define DIM4 4
#define DIM4_NUM_TILES 16
#define DIM4_HEURISTICS_FILE "dim4_heuristics.bin"

// Constants for a 6,6,3 tile pattern database.
#define NUM_PATTERNS 3

#define PATTERN_0 1,5,6,9,10,13
#define PATTERN_1 7,8,11,12,14,15
#define PATTERN_2 2,3,4

#define PATTERN_0_LEN 6
#define PATTERN_1_LEN 6
#define PATTERN_2_LEN 3

#define PATTERN_0_ARRAY_OFFSET 0
#define PATTERN_1_ARRAY_OFFSET 16777216  // 16^6
#define PATTERN_2_ARRAY_OFFSET 33554432  // 16^6 * 2

#define TOTAL_STATES 33558528            // 16^6 * 2 + 16^3
#define VISITED_STATES 268435456         // 16^7

// Encapsulate data for a single tile pattern.
typedef struct
{
    int tiles[DIM4_NUM_TILES];
    int num_tiles;
    int array_offset;
}
tile_pattern;

// An array for all 3 patterns.
const tile_pattern patterns[NUM_PATTERNS] = {
  {{PATTERN_0}, PATTERN_0_LEN, PATTERN_0_ARRAY_OFFSET},
  {{PATTERN_1}, PATTERN_1_LEN, PATTERN_1_ARRAY_OFFSET},
  {{PATTERN_2}, PATTERN_2_LEN, PATTERN_2_ARRAY_OFFSET},};

// We initialize an array to store the valid moves available for each
// possible position on the board of the empty tile. Picturing the board in two
// dimensions there are up to four possible directions a move can be made: move
// a tile down, left, up or right.
// If 'i' is the index of the empty tile and 'j' represents the direction
// moving clockwise, then valid_moves[i][j] gives the index of a tile that can
// be moved. The value -1 is used to indicate there is no move from that
// direction. For example:
//               0  1  2  3
//               4  5  6  7      valid_moves[6]  = {2,7,10,5}
//               8  9 10 11      valid_moves[8]  = {4,9,12,-1}
//              12 13 14 15      valid_moves[15] = {11,-1,-1,14}
const int valid_moves[DIM4_NUM_TILES][4] = {
    {-1,1,4,-1},  {-1,2,5,0},   {-1,3,6,1},    {-1,-1,7,2},
    {0,5,8,-1},   {1,6,9,4},    {2,7,10,5},    {3,-1,11,6},
    {4,9,12,-1},  {5,10,13,8},  {6,11,14,9},   {7,-1,15,10},
    {8,13,-1,-1}, {9,14,-1,12}, {10,15,-1,13}, {11,-1,-1,14}};

#endif

