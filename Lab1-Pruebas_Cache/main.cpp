// main.cpp
#include "bucle_for.h"
#include "mult_matrices.h"
#include "mult_matrices_bloques.h"

int main() {
    cout << "==== Ejercicio 1: Bucles For ====" << endl;
    init_bucle_for();
    bucle_for_primer();
    bucle_for_segundo();

    cout << "\n==== Ejercicio 2: Multiplicacion clasica ====" << endl;
    mult_matrices_clasica(512);
    mult_matrices_clasica(1024);
    //mult_matrices_clasica(2000);

    cout << "\n==== Ejercicio 3: Multiplicacion por bloques ====" << endl;
    mult_matrices_bloques(512, 64);
    mult_matrices_bloques(1024, 64);
    //mult_matrices_bloques(2000, 128);

    return 0;
}
