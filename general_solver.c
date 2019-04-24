/**
 * general_solver.c
 *
 * This file implements various functions which are used to automatically
 * solve the 15-puzzle and its (n^2-1)-puzzle variants. The methods used here
 * are non-optimal meaning there exist more efficient solutions.
 *
 * In the case of puzzles of dimension less than or equal to 4, these methods
 * are only used if the optimal solvers are not available. In the case of
 * dimension greater than 5, these methods must be used in order for the solver
 * to be computationally feasible.
 *
 * The approach taken mimics what a human solver might try. Let us say we have
 * a puzzle of dimension 5. We first arrange tiles 1 to 5 along the top row.
 * For tiles 1 to 4 this is just a matter of locating the tile and locating the
 * empty tile and making moves to place the tile in the correct spot,
 *
 *          |  1  |  2  |  3  |  4  |  x  |
 *          |  x  |  x  |  x  |  x  |  x  |
 *
 * For the last tile on the row, 5, we must first shift the tiles 1-4 all to
 * the right then place tile 5 as shown below,
 *
 *          |  x  |  1  |  2  |  3  |  4  |
 *          |  x  |  x  |  x  |  x  |  5  |
 *
 * Then we can slide the 1-4 tiles back to the left and slide the 5 tile up.
 *
 * We then leave the first row intact and work on the rest of the first column,
 * using the same technique to place tiles 6, 11, 17 and 22.
 *
 * Having solved the first row and column we can now iteratively arrange the
 * second row and second column and so on.
 *
 * When we start sliding tiles towards their destinations, movements are done
 * by postitioning the empty tile to either below or to the right of the tile
 * that is being moved so that any tiles we already have in the correct
 * locations are not displaced.
 *
 * Once the unsolved area of the puzzle is reduced to the last 4 rows by 4
 * columns, the optimal solver for 4x4 boards can be used to complete the
 * solution if it is available.
 */

#include "fifteen.h"

/*
 * Moves the empty tile towards a specified target row and column and, without
 * touching the target tile at that location, leaves the empty tile at one of
 * the up to 8 adjacent locations around the target tile. I.e. for target tile
 * T, at one of the locations marked 'e'
 *
 *              |  e  |  e  |  e  |
 *              |  e  |  T  |  e  |
 *              |  e  |  e  |  e  |
 */
void move_empty_to_target(int target_row, int target_col)
{
    while (p.empty_col + 1 < target_col)
    {
        slide('l');
    }
    while (p.empty_col - 1 > target_col)
    {
        slide('r');
    }
    while (p.empty_row + 1 < target_row)
    {
        slide('u');
    }
    while (p.empty_row - 1 > target_row)
    {
        slide('d');
    }
}

/*
 * Moves the empty tile so that it sits directly underneath a given target row
 * and column for a target tile. If the target tile is on the bottom row it
 * will be moved up so that the empty tile can go below it, otherwise the
 * target tile is not moved.
 */
void move_empty_to_below_target(int target_row, int target_col)
{
    // Move the empty tile to an adjacent location.
    move_empty_to_target(target_row, target_col);

    // Deal with each of the possible locations the empty tile is at, it may
    // already be in the correct space or in one of 7 other locations.

    // Empty tile is below the target row.
    if (p.empty_row == target_row + 1)
    {
        if (p.empty_col == target_col - 1)
        {
            slide('l');
        }
        else if (p.empty_col == target_col + 1)
        {
            slide('r');
        }
    }

    // Empty tile is on the target row.
    else if (p.empty_row == target_row)
    {
        if (p.empty_col == target_col - 1)
        {
            // If the target tile is on the bottom row.
            if (target_row == p.dim - 1)
            {
                slide('d');
                slide('l');
                slide('u');
                target_row--;
            }
            else
            {
                slide('u');
                slide('l');
            }
        }
        else if (p.empty_col == target_col + 1)
        {
            // If the target tile is on the bottom row.
            if (target_row == p.dim - 1)
            {
                slide('d');
                slide('r');
                slide('u');
                target_row--;
            }
            else
            {
                slide('u');
                slide('r');
            }
        }
    }

    // Empty tile is above the target row.
    else if (p.empty_row == target_row - 1)
    {
        // If the target tile is on the bottom row.
        if (target_row == p.dim - 1)
        {
            if (p.empty_col == target_col - 1)
            {
                slide('l');
            }
            else if (p.empty_col == target_col + 1)
            {
                slide('r');
            }
            slide('u');
            target_row--;
        }
        else
        {
            if (p.empty_col == target_col - 1)
            {
                slide('u');
                slide('u');
                slide('l');
            }
            else if (p.empty_col == target_col)
            {
                // If the target tile is on the rightmost column.
                if (target_col == p.dim - 1)
                {
                    slide('r');
                    slide('u');
                    slide('u');
                    slide('l');
                }
                // Otherwise move around the target on the righthand side.
                else
                {
                    slide('l');
                    slide('u');
                    slide('u');
                    slide('r');
                }
            }
            else if (p.empty_col == target_col + 1)
            {
                slide('u');
                slide('u');
                slide('r');
            }
        }
    }
}

/*
 * Moves the empty tile so that it sits directly to the right of a given target
 * row and column for a target tile. If the target tile is on the rightmost
 * column it will be moved left so that the empty tile can go to the right of
 * it, otherwise the target tile is not moved.
 */
void move_empty_to_right_of_target(int target_row, int target_col)
{
    // Move the empty tile to an adjacent location.
    move_empty_to_target(target_row, target_col);

    // Empty tile is to the right of the target column.
    if (p.empty_col == target_col + 1)
    {
        if (p.empty_row == target_row - 1)
        {
            slide('u');
        }
        else if (p.empty_row == target_row + 1)
        {
            slide('d');
        }
    }

    // Empty tile is on target column.
    else if (p.empty_col == target_col)
    {
        if (p.empty_row == target_row - 1)
        {
            // If the target tile is on the rightmost column.
            if (target_col == p.dim - 1)
            {
                slide('r');
                slide('u');
                slide('l');
                target_col--;
            }
            else
            {
                slide('l');
                slide('u');
            }
        }
        if (p.empty_row == target_row + 1)
        {
            // If the target tile is on the rightmost column.
            if (target_col == p.dim - 1)
            {
                slide('r');
                slide('d');
                slide('l');
                target_col--;
            }
            else
            {
                slide('l');
                slide('d');
            }
        }
    }

    // Empty tile is to the left of the target column.
    else if (p.empty_col == target_col - 1)
    {
        // If the target tile is on the rightmost column.
        if (target_col == p.dim - 1)
        {
            if (p.empty_row == target_row - 1)
            {
                slide('u');
            }
            else if (p.empty_row == target_row + 1)
            {
                slide('d');
            }
            slide('l');
            target_col--;
        }
        else
        {
            if (p.empty_row == target_row - 1)
            {
                slide('l');
                slide('l');
                slide('u');
            }
            else if (p.empty_row == target_row)
            {
                // If the target tile is on the bottom row.
                if (target_row == p.dim - 1)
                {
                    slide('d');
                    slide('l');
                    slide('l');
                    slide('u');
                }
                // Otherwise move around underneath the target.
                else
                {
                    slide('u');
                    slide('l');
                    slide('l');
                    slide('d');
                }
            }
            else if (p.empty_row == target_row + 1)
            {
                slide('l');
                slide('l');
                slide('d');
            }
        }
    }
}

/*
 * Moves a given target tile to a given destination row.
 */
void move_target_to_row(int target_tile, int destination_row)
{
    // Find current row and column of target tile.
    int target_row;
    int target_col;
    for (int row = 0; row < p.dim; row++)
    {
        for (int col = 0; col < p.dim; col++)
        {
            if (p.board[row][col] == target_tile)
            {
                target_row = row;
                target_col = col;
                row = p.dim;
                col = p.dim;
            }
        }
    }

    // Determine how many rows we want to move the target by.
    int num_rows = destination_row - target_row;
    if (num_rows == 0)
    {
        return;
    }

    // Move the empty tile to the right of the target in preparation.
    move_empty_to_right_of_target(target_row, target_col);

    // If num_rows is positive we are moving the tile down.
    if (num_rows > 0)
    {
        // Iterate a sequence of slides that moves a tile down.
        for (int i = 0; i < num_rows; i++)
        {
            slide('u');
            slide('r');
            slide('d');
            // If we are done then break, else move the empty tile back to the
            // right of the target for another loop.
            if (i == num_rows - 1)
            {
                break;
            }
            slide('l');
            slide('u');
        }
    }
    // Else we are moving the tile up.
    else
    {
        num_rows = -num_rows;
        // Iterate a sequence of slides that moves a tile up.
        for (int i = 0; i < num_rows; i++)
        {
            slide('d');
            slide('r');
            slide('u');
            // If we are done then break, else move the empty tile back to the
            // right of the target for another loop.
            if (i == num_rows - 1)
            {
                break;
            }
            slide('l');
            slide('d');
        }
    }
}

/*
 * Moves a given target tile to a given destination column.
 */
void move_target_to_col(int target_tile, int destination_col)
{
    // Find current row and column of target tile.
    int target_row;
    int target_col;
    for (int row = 0; row < p.dim; row++)
    {
        for (int col = 0; col < p.dim; col++)
        {
            if (p.board[row][col] == target_tile)
            {
                target_row = row;
                target_col = col;
                row = p.dim;
                col = p.dim;
            }
        }
    }

    // Determine how many columns we want to move the target by.
    int num_cols = destination_col - target_col;
    if (num_cols == 0)
    {
        return;
    }

    // Move the empty tile to below the target in preparation.
    move_empty_to_below_target(target_row, target_col);

    // If num_cols is positive we are moving the tile right.
    if (num_cols > 0)
    {
        // Iterate a sequence of slides that moves a tile right.
        for (int i = 0; i < num_cols; i++)
        {
            slide('l');
            slide('d');
            slide('r');
            // If we are done then break, else move the empty tile back to
            // below the target for another loop.
            if (i == num_cols - 1)
            {
                break;
            }
            slide('u');
            slide('l');
        }
    }
    // Else we are moving the tile left.
    else
    {
        num_cols = -num_cols;
        // Iterate a sequence of slides that moves a tile left.
        for (int i = 0; i < num_cols; i++)
        {
            slide('r');
            slide('d');
            slide('l');
            // If we are done then break, else move the empty tile back to
            // below the target for another loop.
            if (i == num_cols - 1)
            {
                break;
            }
            slide('u');
            slide('r');
        }
    }
}

/*
 * For a given row, arrange the tiles in the correct order for that row.
 * We assume we have already arranged offset many rows and columns and now wish
 * to arrange the row whose index is offset (rows are zero-indexed).
 */
void arrange_row(int offset)
{
    // Arrange all but the last tile for the row.
    for (int j = offset + 1; j < p.dim; j++)
    {
        // Determine which tile numbers we are moving. (There are offset many
        // tiles already in the correct location on this row since offset many
        // columns are already arranged.)
        int target_tile = j + (p.dim * offset);
        // Move the target tile to the correct location.
        move_target_to_col(target_tile, j - 1);
        move_target_to_row(target_tile, offset);
        // If the empty tile is still on the row, move if to the next row down
        // so we do not interfere with tiles already placed when locating the
        // next target.
        if (p.empty_row == offset)
        {
            slide('u');
        }
    }

    // Now arrage the last tile in the row.
    int target_tile = p.dim * (offset + 1);
    if (p.board[offset][p.dim-1] != target_tile)
    {
        // Move the empty tile to the last column of the row.
        while (p.empty_col != p.dim - 1)
        {
            slide('l');
        }
        while (p.empty_row != offset)
        {
            slide('d');
        }

        // We now want to shuffle all the tiles on the row after the offset
        // column one place to the right so we can slot in our target tile on
        // the next row in the last column. Then shuffling all the tiles back
        // to the left and moving the target tile up leaves the row arranged.

        // However, before we start we need to move the target tile out of the
        // way if it is at the following awkward location:
        if (p.board[offset+1][offset] == target_tile)
        {
            while (p.empty_col != offset + 1)
            {
                slide('r');
            }
            slide('u');
            slide('r');
            slide('u');
            slide('l');
            slide('d');
            slide('d');
            slide('r');
        }
        // Now we are safe to shuffle tiles on the row to the right.
        else
        {
            while (p.empty_col != offset)
            {
                slide('r');
            }
            slide('u');
        }

        // Place our target tile on the next row in the last column.
        move_target_to_col(target_tile, p.dim - 2);
        move_target_to_row(target_tile, offset + 1);
        move_target_to_col(target_tile, p.dim - 1);

        // Now move the empty tile back to the offset column and row.
        while (p.empty_col != offset)
        {
            slide('r');
        }
        while (p.empty_row != offset)
        {
            slide('d');
        }
        // Slide all the tiles on the row left.
        while (p.empty_col != p.dim - 1)
        {
            slide('l');
        }
        // And finally slide the target tile up.
        slide('u');
    }
}

/*
 * For a given column, arrange the tiles in the correct order for that column.
 * We assume we have already arranged offset many rows and columns and now wish
 * to arrange the column whose index is offset (columns are zero-indexed).
 */
void arrange_column(int offset)
{
    // This function follows the same pattern as arrange_top_row.

    for (int j = offset + 1; j < p.dim - 1; j++)
    {
        int target_tile = (j * p.dim) + offset + 1;
        move_target_to_row(target_tile, j);
        move_target_to_col(target_tile, offset);
        if (p.empty_col == offset)
        {
            slide('l');
        }
    }

    int target_tile = (p.dim * (p.dim - 1)) + offset + 1;
    if (p.board[p.dim-1][offset] != target_tile)
    {
        while (p.empty_row != p.dim - 1)
        {
            slide('u');
        }
        while (p.empty_col != offset)
        {
            slide('r');
        }

        if (p.board[offset+1][offset+1] == target_tile)
        {
            while (p.empty_row != offset + 2)
            {
                slide('d');
            }
            slide('l');
            slide('d');
            slide('l');
            slide('u');
            slide('r');
            slide('r');
            slide('d');
        }
        else
        {
            while (p.empty_row != offset + 1)
            {
                slide('d');
            }
            slide('l');
        }

        move_target_to_row(target_tile, p.dim - 2);
        move_target_to_col(target_tile, offset + 1);
        move_target_to_row(target_tile, p.dim - 1);

        while (p.empty_row != offset + 1)
        {
            slide('d');
        }
        while (p.empty_col != offset)
        {
            slide('r');
        }
        while (p.empty_row != p.dim - 1)
        {
            slide('u');
        }
        slide('l');
    }
}

