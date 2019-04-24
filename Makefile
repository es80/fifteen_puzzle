# Compiler
CC = clang

# Compiler flags.
CFLAGS = -ggdb3 -O0 -std=c11 -Wall -Werror

# Name for main executable.
EXE = fifteen

# space-separated list of header files.
HDRS = fifteen.h dim4.h

# Space-separated list of libraries prefixed with -l
LIBS = -lncurses

# Space-separated list of source files.
SRCS = fifteen.c general_solver.c logic.c dim4_solver.c

# Automatically generated list of object files.
OBJS = $(SRCS:.c=.o)

# Default target.
$(EXE): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Dependencies.
$(OBJS): $(HDRS) Makefile

# Other targets.
generate_dim3_solutions: generate_dim3_solutions.c
	$(CC) $(CFLAGS) -o $@ generate_dim3_solutions.c
generate_dim4_heuristics: generate_dim4_heuristics.c dim4.h
	$(CC) $(CFLAGS) -o $@ generate_dim4_heuristics.c
standalone_dim4_solver: standalone_dim4_solver.c dim4.h
	$(CC) $(CFLAGS) -o $@ standalone_dim4_solver.c

clean:
	rm -f core $(EXE) *.o generate_dim3_solutions generate_dim4_heuristics standalone_dim4_solver

