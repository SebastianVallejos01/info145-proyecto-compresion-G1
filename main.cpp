#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <chrono>

// Aquí vayan incluyendo sus archivos, dejé esos nombres como ejemplo
// #include "explicita.hpp"   // Caso 1
#include "gap_coding.hpp"  // Caso 2
#include "compresion.hpp"  // Caso 3

using namespace std;

// ====================
// GENERADORES DE DATOS
//=====================

vector<uint32_t> generarDistribucionLineal(size_t n, uint32_t epsilon) {
    vector<uint32_t> A(n);
    mt19937 rng(42); 
    uniform_int_distribution<uint32_t> dist(1, epsilon);

    A[0] = dist(rng);
    for (size_t i = 1; i < n; ++i) {
        A[i] = A[i - 1] + dist(rng);
    }
    return A;
}

vector<uint32_t> generarDistribucionNormal(size_t n, double media, double desviacion) {
    vector<uint32_t> A(n);
    mt19937 rng(42);
    normal_distribution<double> dist(media, desviacion);

    double val = dist(rng);
    A[0] = (val > 0) ? static_cast<uint32_t>(val) : 0;

    for (size_t i = 1; i < n; ++i) {
        double gap = dist(rng);
        uint32_t gap_entero = (gap > 0) ? static_cast<uint32_t>(gap) : 1; // Asegurar crecimiento
        A[i] = A[i - 1] + gap_entero;
    }
    return A;
}

// ===============
// MODO BENCHMARK
// ===============

void runBenchmark() {
    cout << "Ejecutando modo benchmark..." << endl;
    
    // Tamaños incrementales requeridos
    vector<size_t> tamanos = {1000000, 10000000}; 
    size_t numero_busquedas = 10000; // Múltiples búsquedas
    size_t b_parametrico = 128; // Salto para el Sample

    for (size_t n : tamanos) {
        cout << "\n--- Probando tamaño N = " << n << " ---" << endl;
        
        // Generar datos (Ejemplo con Lineal)
        cout << "Generando arreglo lineal..." << endl;
        vector<uint32_t> arreglo_original = generarDistribucionLineal(n, 100);

        // Generamos un set de valores aleatorios para buscar luego
        vector<uint32_t> valores_a_buscar(numero_busquedas);
        mt19937 rng(123);
        uniform_int_distribution<size_t> dist_indices(0, n - 1);
        for(size_t i = 0; i < numero_busquedas; i++) {
            valores_a_buscar[i] = arreglo_original[dist_indices(rng)];
        }

        // Medir tiempo de construcción de Gap-Coding (Caso 2)
        auto start_build = chrono::high_resolution_clock::now();
        GapCoder caso2(arreglo_original, b_parametrico);
        auto end_build = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> build_time = end_build - start_build;

        cout << "[Caso 2] Tiempo de construcción: " << build_time.count() << " ms" << endl;
        cout << "[Caso 2] Espacio utilizado: " << caso2.getEspacioUtilizado() << " bytes" << endl;
        
     // ==========================================
     // INTEGRACIÓN DEL CASO 3: PForDelta
     // ==========================================
     cout << "Comprimiendo gaps con PForDelta (Caso 3)..." << endl;

     // Medimos el tiempo de construcción del Caso 3 usando los gaps del Caso 2
     auto start_build_c3 = chrono::high_resolution_clock::now();
     PForDeltaCoder caso3(caso2.getGC());
     auto end_build_c3 = chrono::high_resolution_clock::now();
     chrono::duration<double, milli> build_time_c3 = end_build_c3 - start_build_c3;

     cout << "[Caso 3] Tiempo de construcción: " << build_time_c3.count() << " ms" << endl;
     cout << "[Caso 3] Espacio utilizado: " << caso3.getEspacioUtilizado() << " bytes" << endl;
     // ==========================================
     
        // Volver inaccesible el arreglo original (Regla del Caso 2)
        arreglo_original.clear();
        arreglo_original.shrink_to_fit();

        // Medir tiempo de múltiples búsquedas
        auto start_search = chrono::high_resolution_clock::now();
        int encontrados = 0;
        for (uint32_t valor : valores_a_buscar) {
            if (caso2.buscar(valor) != -1) {
                encontrados++;
            }
        }
        auto end_search = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> search_time = end_search - start_search;

        cout << "[Caso 2] Tiempo total por " << numero_busquedas << " búsquedas: " << search_time.count() << " ms" << endl;
        cout << "[Caso 2] Elementos encontrados con éxito: " << encontrados << "/" << numero_busquedas << endl;
        
        // ====================================================================
        // Medir tiempo de multiples busquedas para el Caso 3 (PForDelta)
        // ====================================================================
        auto start_search_c3 = chrono::high_resolution_clock::now();
        int encontrados_c3 = 0;
        
        for (uint32_t valor : valores_a_buscar) {
            // Buscamos utilizando la estructura comprimida de PForDelta
            if (caso3.buscar(valor, caso2.getSample(), b_parametrico) != -1) {
                encontrados_c3++;
            }
        }
        
        auto end_search_c3 = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> search_time_c3 = end_search_c3 - start_search_c3;

        cout << "[Caso 3] Tiempo total por " << numero_busquedas << " búsquedas: " << search_time_c3.count() << " ms" << endl;
        cout << "[Caso 3] Elementos encontrados con éxito: " << encontrados_c3 << "/" << numero_busquedas << endl;
        // ====================================================================
    }
}

void runInteractive(const string& filename) {
    cout << "Ejecutando modo interactivo con archivo: " << filename << endl;
    // Aquí irá la lógica para leer el archivo CSV y buscar valores
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso incorrecto. Argumentos validos:" << endl;
        cerr << "  ./main --benchmark" << endl;
        cerr << "  ./main -i ruta/del/archivo.csv" << endl;
        return 1;
    }
    string mode = argv[1];
    if (mode == "--benchmark") {
        runBenchmark();
    } 
    else if (mode == "-i") {
        if (argc < 3) {
            cerr << "Error: Faltó especificar la ruta del archivo CSV." << endl;
            return 1;
        }
        string filename = argv[2];
        runInteractive(filename);
    } 
    else {
        cerr << "Modo no reconocido: " << mode << endl;
        return 1;
    }

    return 0;
}