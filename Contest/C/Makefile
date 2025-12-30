# Compilación CUDA para Problem C (RTM)
CC = nvcc
EXEC = rtm
FLAGS = -O3 -use_fast_math -arch=sm_70

all: $(EXEC)

$(EXEC): $(EXEC).cu
	$(CC) $(FLAGS) $(EXEC).cu -o $(EXEC)

clean:
	rm -f $(EXEC)
