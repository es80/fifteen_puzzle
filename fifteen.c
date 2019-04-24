/**
 * fifteen.c
 *
 * Implements the 'Game of Fifteen' puzzle, also know as the 15-puzzle or 4x4
 * sliding tile puzzle. Allows puzzles to be played of any square dimension
 * between 2x2 and 9x9 and also provides an automatic solver. In the cases of
 * 3x3 and 4x4 puzzles, the automatic solver can be made to generate optimal
 * solutions.
 * https://en.wikipedia.org/wiki/15_puzzle
 *
 * The code started from a solution to a problem set from Harvard's CS50
 * Introduction to Computer Science course.
 * https://docs.cs50.net/problems/fifteen/fifteen.html
 * It borrows a small amount of ncurses code from a related problem set
 * http://cdn.cs50.net/2011/fall/psets/4/pset4.pdf

 * Run the program with './fifteen' and you are presented with a standard 4x4
 * 15-puzzle. The game is implemented using ncurses. The arrow keys move tiles
 * whilst 's' and 'r' start new puzzles with either the standard or a random
 * tile configuration respectively. The numbers '2' to '9' will change the
 * dimensions of the puzzle between 2x2 and 9x9. Pressing 'g' calls 'God mode'
 * in which the computer automatically solves the remainer of the puzzle.
 *
 * The code is split into a number of files,
 * - fifteen.c implements the game loop and functions for ncurses display.
 * - logic.c implements functions dealing with the game's logic.
 * - general_solver.c implements a number of functions used by the automatic
 *   solver.
 *
 * Optionally the following files can be used so that the automatic solver may
 * will produce optimal solutions in the 3x3 and 4x4 cases of the puzzle.
 * - generate_dim3_solutions.c - used to generate a small binary file
 *   containing optimal solutions for the 3x3 puzzle.
 * - generate_dim4_heuristics.c - used to generate a large (11.5 MB) binary
 *   file containing heuristic data to aid the 4x4 puzzle solver.
 * - dim4_solver.c - implements an optimal solver for the 4x4 puzzle case using
 *   the heuristic data. This program can also be run independently to read in
 *   and solve 4x4 puzzles optimally.
 */

#define _XOPEN_SOURCE 500

#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fifteen.h"

// Macro for processing control characters.
#define CTRL(x) ((x) & ~0140)

/*
 * Draws the header and footer.
 */
void draw_header_footer(void);

/*
 * Redraws everything on the screen.
 */
void redraw_all(void);

/*
 * Startup ncurses and returns true iff successful.
 */
bool startup(void);

/*
 * Handle terminal window size changed signal, if received call redraw_all.
 */
void handle_signal(int signum);

// We use a single global variable p to contain our puzzle's data.
struct puzzle p;


int main(int argc, char *argv[])
{
    // Start ncurses.
    if (!startup())
    {
        fprintf(stderr, "Error starting up ncurses!\n");
        return 1;
    }

    // Register handler for SIGWINCH (SIGnal WINdow CHanged).
    signal(SIGWINCH, (void (*)(int)) handle_signal);

    // Seed the random number generator.
    //srand48(0); // testing
    srand48((long int) time(NULL));

    // Initialize a standard 4x4 puzzle.
    p.dim = 4;
    init("standard");
    draw_header_footer();
    draw_board();

    // Arrays which may be used by solver.
    uint8_t *dim3_array = NULL;
    uint8_t *dim4_array = NULL;

    // The user's input.
    int ch;
    // The move direction to make.
    char mv = 0;

    // Game loop.
    do
    {
        // Refresh the screen.
        refresh();

        // Get user's input and capitalize.
        ch = getch();
        ch = toupper(ch);
        switch (ch)
        {
            // Manually redraw screen with ctrl-L.
            case CTRL('l'):
                redraw_all();
                break;

            // New standard puzzle.
            case 'S':
                init("standard");
                break;

            // New random puzzle.
            case 'R':
                init("random");
                break;

            // Change puzzle dimension.
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                p.dim = ch - '0';
                init("standard");
                redraw_all();
                break;

            // Move the tiles with keypad.
            case KEY_LEFT:
                mv = 'l';
                break;
            case KEY_RIGHT:
                mv = 'r';
                break;
            case KEY_UP:
                mv = 'u';
                break;
            case KEY_DOWN:
                mv = 'd';
                break;

            // Enter 'God-mode'.
            case 'G':
                if (p.puzzle_state == UNSOLVED)
                {
                    if (!god_mode(&dim3_array, &dim4_array))
                    {
                        // An error message is produced.
                        p.puzzle_state = THERE_IS_NO_GOD;
                    }
                }
                break;
        }

        // Make the move
        if (mv && (p.puzzle_state == UNSOLVED
                   || p.puzzle_state == THERE_IS_NO_GOD))
        {
            slide(mv);
            mv = 0;
            p.puzzle_state = UNSOLVED;
        }

        // Check for solved puzzle, then redraw the board.
        if (is_solved() && p.puzzle_state == UNSOLVED)
        {
            p.puzzle_state = SOLVED;
        }
        draw_board();
    }
    while (ch != 'Q');

    // Shutdown ncurses.
    endwin();

    // Free malloc'd memory.
    if (dim3_array)
    {
        free(dim3_array);
    }
    if (dim4_array)
    {
        free(dim4_array);
    }

    // Clears screen using ANSI escape sequences.
    printf("\033[2J");
    printf("\033[%d;%dH", 0, 0);

    return 0;
}

/*
 * Draws the header and footer.
 */
void draw_header_footer(void)
{
    // Get the window's dimensions.
    int maxy;
    int maxx;
    getmaxyx(stdscr, maxy, maxx);

    // Draw the header and footer.
    const char head[] = "Fifteen";
    mvaddstr(0, (maxx - strlen(head)) / 2, head);
    const char foot[] = "New: [S]tandard/[R]andom   "
                        "Change dimensions: [2]...[9]   [G]od mode!   [Q]uit";
    mvaddstr(maxy - 1, (maxx - strlen(foot)) / 2, foot);
}

/*
 * Draws the puzzle board and, if needed, a message underneath.
 */
void draw_board(void)
{
    // Get the window's dimensions.
    int maxy;
    int maxx;
    getmaxyx(stdscr, maxy, maxx);

    // Determine a scaling factor, a number between 0 and 2 based upon the
    // available space in the window and the dimensions of the puzzle.
    int sf_x = (((maxx - 3) / p.dim) - 5) / 4;
    int sf_y = (((maxy - 8) / p.dim) - 2) / 2;
    // Use the smallest of these numbers and make sure it is at most 2.
    int scaling_factor = sf_x < sf_y ? sf_x : sf_y;
    scaling_factor = scaling_factor > 2 ? 2 : scaling_factor;

    // Now compute exact width and height of puzzle board.
    int base_width = 4;
    int padding_width = scaling_factor * 4;
    int padding_height = scaling_factor * 2;
    int board_width = (base_width + padding_width + 1) * p.dim + 1;
    int board_height = (1 + padding_height + 1) * p.dim + 1;

    // Determine the top-left corner of board in order to be centred.
    int y = (maxy - board_height) / 2;
    int x = (maxx - board_width) / 2;

    // Print top border.
    move(y, x);
    for (int col = 0; col < p.dim; col++)
    {
        addch('+');
        for (int i = 0; i < base_width + padding_width; i++)
        {
            addch('-');
        }
    }
    addch('+');

    // Print rows.
    for (int row = 0; row < p.dim; row++)
    {
        // Print rows of tiles.
        for (int i = 0; i <= padding_height; i++)
        {
            move(y + (row * (padding_height + 2)) + i + 1, x);
            for (int col = 0; col < p.dim; col++)
            {
                addch('|');
                // Print tile numbers.
                if (i == padding_height / 2)
                {
                    char tile_str[3] = "  ";
                    if (p.board[row][col] != 0)
                    {
                        sprintf(tile_str, "%2i", p.board[row][col]);
                    }
                    int w = base_width + padding_width;
                    int prefix = (w - 2) / 2;
                    int postfix = (w - 1) / 2;

                    for (int j = 0; j < prefix; j++)
                    {
                        addch(' ');
                    }
                    addstr(tile_str);
                    for (int j = 0; j < postfix; j++)
                    {
                        addch(' ');
                    }
                }
                // Print row of spaces above or below tile numbers.
                else
                {
                    for (int j = 0; j < base_width + padding_width; j++)
                    {
                        addch(' ');
                    }
                }
            }
            addch('|');
        }

        // Print a row of border.
        move(y + row * (padding_height + 2) + padding_height + 2, x);
        for (int col = 0; col < p.dim; col++)
        {
            addch('+');
            for (int i = 0; i < base_width + padding_width; i++)
            {
                addch('-');
            }
        }
        addch('+');
    }

    // Now print any message needed. First erase previous message.
    move(y + board_height + 1, 0);
    for (int c = 0; c < maxx; c++)
    {
        addch(' ');
    }
    // Construct new message.
    char message[60] = {'\0'};
    char plural[1] = {'\0'};
    p.move_number != 1 ? strcpy(plural, "s") : strcpy(plural, "");
    // Using p.puzzle_state, determine which message to display.
    switch (p.puzzle_state)
    {
        case UNSOLVED:
            break;

        case SOLVED:
            sprintf(message, "Puzzle solved in %i move%s!", p.move_number,
                    plural);
            break;

        case GOD_MODE:
            sprintf(message, "Solving puzzle automatically!");
            break;

        case BUSY:
            sprintf(message, "Computing moves...");
            break;

        case GOD_SOLVED:
            sprintf(message, "Puzzle solved by computer in %i move%s!",
                    p.move_number, plural);
            break;

        case GOD_SOLVED_OPTIMAL:
            sprintf(message, "Puzzle solved optimally by computer in %i "
                    "move%s!", p.move_number, plural);
            break;

        case THERE_IS_NO_GOD:
            sprintf(message, "God mode unavailable.");
            break;
    }
    mvaddstr(y + board_height + 1, (maxx - strlen(message)) / 2, message);
}

/*
 * Redraws everything on the screen.
 */
void redraw_all(void)
{
    // Reset and clear screen.
    endwin();
    refresh();
    clear();

    // Redraw.
    draw_header_footer();
    draw_board();
}

/*
 * Startup ncurses and returns true iff successful.
 */
bool startup(void)
{
    // Initialize ncurses.
    if (initscr() == NULL)
    {
        return false;
    }

    // Don't echo keyboard input.
    if (noecho() == ERR)
    {
        endwin();
        return false;
    }

    // Disable line buffering, allow ctrl-C signal.
    if (cbreak() == ERR)
    {
        endwin();
        return false;
    }

    // Enable arrow keys.
    if (keypad(stdscr, true) == ERR)
    {
        endwin();
        return false;
    }

    // Hide the cursor if possible.
    curs_set(0);

    // Wait 1000 ms at a time for input.
    timeout(1000);

    return true;
}

/*
 * Handle terminal window size changed signal, if received call redraw_all.
 */
void handle_signal(int signum)
{
    // If the window size changes then redraw everything.
    if (signum == SIGWINCH)
    {
        redraw_all();
    }

    // Re-register this function to handle future signals.
    signal(signum, (void (*)(int)) handle_signal);
}

