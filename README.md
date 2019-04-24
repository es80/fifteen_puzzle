# Fifteen Puzzle and Solver

This is a simple ncurses implementation of the
['Game of Fifteen'](https://en.wikipedia.org/wiki/15_puzzle)
puzzle, also known as the fifteen-puzzle or 4x4 sliding tile puzzle. It
inlcudes options to play other square dimensions from 3x3 to 9x9. There is also
a solver which in the case of 3x3 and 4x4 puzzles can generate optimal
solutions.

The code started from a solution to a
[problem set](https://docs.cs50.net/problems/fifteen/fifteen.html) from the
Harvard edX [CS50 Introduction to Computer Science course](https://www.edx.org/course/cs50s-introduction-to-computer-science) and borrows
a small amount of ncurses code from a related
[problem set](http://cdn.cs50.net/2011/fall/psets/4/pset4.pdf).

## Instructions

To make and run locally requires

```
gcc
make
ncurses
```

then to play a game

```
make
./fifteen
```
The arrow keys move tiles whilst 's' and 'r' start new puzzles with either the
standard or a random tile configuration respectively. The numbers '2' to '9'
will change the dimensions of the puzzle between 2x2 and 9x9. Pressing 'g'
calls 'God mode' in which the solver takes over to complete the remainder of
the puzzle.


## Optimal Solutions

Initally the solver produces non-optimal solutions by making 'human-like' moves
to complete the puzzle.

To generate optimal solutions in the 3x3 case, before running `./fifteen`,
first run

```
make generate_dim3_solutions
./generate_dim3_solutions
```

which will produce a small binary file `dim3_solutions.bin`. The solver can
then lookup optimal solutions to any 3x3 puzzle.

To generate optimal solutions in the 4x4 case, before running `./fifteen`,
ensure 33.6MB of free space then run

```
make generate_dim4_heuristics
./generate_dim4_heuristics
```

This will take a few minutes to complete and will generate a database of
heuristic values `dim4_heuristics.bin` to aid the solver.

The optimal solver for 4x4 puzzles works by employing an [iterative deepening A*
search](https://en.wikipedia.org/wiki/Iterative_deepening_A*) using additive
pattern database heuristics. Explanations and references are given in the
source code.

Most random puzzles will be solved in around a second but some puzzles may require 
more time. For example the standard configuration takes at least a minute.

## Standalone 4x4 Solver

The 4x4 solver can also be used as a standalone program which simply reads a
line of numbers between 0 and 15 from standard input and outputs the optimal
solution.

```
$ make standalone_dim4_solver
$ ./standalone_dim4_solver
9 10 11 13 3 0 2 14 15 5 4 7 6 8 12 1
60 moves: 2 14 13 11 10 2 14 13 7 1 12 8 6 15 3 14 13 4 1 7 4 1 5 3 14 13 3 6 15 14 13 9 2 3 1 5 8 15 14 13 9 1 5 10 11 4 7 8 10 11 3 2 1 5 6 10 11 7 8 12
```

