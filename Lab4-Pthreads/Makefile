# =======================
#   Makefile principal
# =======================
CXX = g++
CC  = gcc
CXXFLAGS = -O2 -std=c++17 -pthread
CFLAGS   = -O2 -std=c11 -pthread

# =======================
#   Ejecutables
# =======================
PROPIAS = linked_list_lockfree matvec_dynamic barrier_sense_ticket
LIBRO   = list_mutex_global matvec_static barrier_condvar
ALL     = $(PROPIAS) $(LIBRO)

all: $(ALL)

# ---------- PROPIAS ----------
linked_list_lockfree: propia/linked_list_lockfree.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

matvec_dynamic: propia/matvec_dynamic_pthreads.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

barrier_sense_ticket: propia/barrier_sense_ticket.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# ---------- LIBRO ----------
list_mutex_global: libro/list_mutex_global.c
	$(CC) $(CFLAGS) $< -o $@

matvec_static: libro/matvec_static.c
	$(CC) $(CFLAGS) $< -o $@

barrier_condvar: libro/barrier_condvar.c
	$(CC) $(CFLAGS) $< -o $@

# ---------- UTILIDADES ----------
clean:
	rm -f $(ALL) *.csv *.png

run: all
	@chmod +x run_bench.sh && ./run_bench.sh

.PHONY: all clean run
