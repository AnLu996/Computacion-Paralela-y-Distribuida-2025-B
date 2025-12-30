FLAGS = -O3 -fopenmp -lm -Wall
CC = gcc

all: cyclic_reduction

cyclic_reduction: cyclic_reduction.c
	$(CC) $(FLAGS) cyclic_reduction.c -o cyclic_reduction

clean:
	rm -f cyclic_reduction
