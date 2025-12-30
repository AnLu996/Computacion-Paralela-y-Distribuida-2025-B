#!/usr/bin/env bash
set -euo pipefail

# Afinidad de CPU opcional para evitar ruido de scheduler
PIN="taskset -c 0-7"

echo "============================================"
echo "Benchmark de implementaciones (Propias vs Libro)"
echo "============================================"

# ---------- LINKED LIST ----------
echo "impl,ops,threads,time_ms" > resultados_linked_list.csv
for T in 1 2 4 8 12 16; do
    $PIN ./linked_list_lockfree 400000 $T >> resultados_linked_list.csv
    $PIN ./list_mutex_global   400000 $T >> resultados_linked_list.csv
done
echo "✔ resultados_linked_list.csv generado."

# ---------- MATVEC ----------
echo "impl,size,threads,chunk,time_ms" > resultados_matvec.csv
for T in 1 2 4 8 12 16; do
    for CH in 8 32 128; do
        $PIN ./matvec_dynamic 4000 4000 $T $CH >> resultados_matvec.csv
    done
    $PIN ./matvec_static 4000 4000 $T >> resultados_matvec.csv
done
echo "✔ resultados_matvec.csv generado."

# ---------- THREAD SAFETY ----------
echo "impl,threads,N,rounds,time_ms" > resultados_barrier.csv
for T in 1 2 4 8 12 16; do
    $PIN ./barrier_sense_ticket $T 65536 200 >> resultados_barrier.csv
    $PIN ./barrier_condvar      $T           >> resultados_barrier.csv
done
echo "✔ resultados_barrier.csv generado."

echo "============================================"
echo "🏁 Todos los benchmarks completados."
echo "Archivos CSV listos:"
echo " - resultados_linked_list.csv"
echo " - resultados_matvec.csv"
echo " - resultados_barrier.csv"
echo "============================================"
