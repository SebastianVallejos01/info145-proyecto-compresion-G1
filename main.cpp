#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdlib>

// Cabeceras
#include "generador.hpp"
#include "caso1.hpp"
#include "caso2.hpp"
#include "pfordelta.hpp"

using namespace std;
using namespace std::chrono;

// Función para exportar métricas a un CSV
void exportarMetrica(const string& rutaDestino, size_t n, const string& distribucion, const string& caso, double tiempoConstMS, double tiempoBusqMicro, size_t espacioBytes){
    ofstream archivo;
    // Se abre en modo append (ios_base::app) para no borrar los datos anteriores
    archivo.open(rutaDestino, ios_base::app);
    if(archivo.is_open()){
        archivo << n << "," << distribucion << "," << caso << ","
                << tiempoConstMS << "," << tiempoBusqMicro << "," << espacioBytes << "\n";
        archivo.close();
    } else {
        cerr << "Error al abrir archivo de metricas" << endl;
    }
}

// Función para leer el archivo CSV usando atoll
vector<uint64_t> leerArchivoCSV(const string& ruta){
    vector<uint64_t> datos;
    ifstream archivo(ruta);
    if(!archivo.is_open()){
        cerr << "Error de ejecucion: no se pudo abrir " << ruta << endl;
        return datos;
    }

    string linea;
    while (getline(archivo,linea)){
        stringstream ss(linea);
        string celda;
        while (getline(ss,celda,',')){
            if(!celda.empty()){
                // Conversión estricta con atoll
                long long valorEntero = atoll(celda.c_str());
                datos.push_back(static_cast<uint64_t>(valorEntero));
            }
        }
    }
    archivo.close();
    return datos;
}

void ejecutarBenchmark(){
    cout << "=== MODO BENCHMARK AUTOMATICO ===" << endl;
    string archivoMetricas = "metricas.csv";
    
    // Inicializar el archivo CSV con los encabezados
    ofstream archivoEncabezado(archivoMetricas);
    if(archivoEncabezado.is_open()){
        archivoEncabezado << "Tamano,Distribucion,Caso,TiempoConstruccion_ms,TiempoBusquedaPromedio_us,Espacio_bytes\n";
        archivoEncabezado.close();
    }

    // Tamaños de escala incremental en potencias de 10
    vector<size_t> tamanos = {1000000, 10000000};
    int numBusquedas = 1000;
    int salto_b = 64; // Salto parametrizado para Caso 2 y 3

    for(size_t n : tamanos){
        cout << "\nProcesando escala n=" << n << "..." << endl;

        // ==========================================
        //         DISTRIBUCION LINEAL
        // ==========================================
        vector<uint64_t> datosLineales = Generador::generarLineal(n, 10);

        // --- Caso 1: Representacion Explicita ---
        auto initC1 = high_resolution_clock::now();
        Caso1 c1_lineal(datosLineales);
        auto finC1 = high_resolution_clock::now();
        double t_const_c1 = duration_cast<milliseconds>(finC1 - initC1).count();

        auto initBusqC1 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c1_lineal.buscar(rand() % (n * 2));
        }
        auto finBusqC1 = high_resolution_clock::now();
        double t_busq_c1 = duration_cast<microseconds>(finBusqC1 - initBusqC1).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Lineal", "Caso 1", t_const_c1, t_busq_c1, c1_lineal.obtenerEspacio());

        // --- Caso 2: Gap-Coding ---
        auto initC2 = high_resolution_clock::now();
        Caso2 c2_lineal(datosLineales, salto_b);
        auto finC2 = high_resolution_clock::now();
        double t_const_c2 = duration_cast<milliseconds>(finC2 - initC2).count();

        auto initBusqC2 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c2_lineal.buscar(rand() % (n * 2));
        }
        auto finBusqC2 = high_resolution_clock::now();
        double t_busq_c2 = duration_cast<microseconds>(finBusqC2 - initBusqC2).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Lineal", "Caso 2", t_const_c2, t_busq_c2, c2_lineal.obtenerEspacio());

        // --- Caso 3: PForDelta ---
        auto initC3 = high_resolution_clock::now();
        PForDelta c3_lineal(datosLineales, salto_b);
        auto finC3 = high_resolution_clock::now();
        double t_const_c3 = duration_cast<milliseconds>(finC3 - initC3).count();

        auto initBusqC3 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c3_lineal.buscar(rand() % (n * 2));
        }
        auto finBusqC3 = high_resolution_clock::now();
        double t_busq_c3 = duration_cast<microseconds>(finBusqC3 - initBusqC3).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Lineal", "Caso 3", t_const_c3, t_busq_c3, c3_lineal.obtenerEspacio());

        
        // ==========================================
        //         DISTRIBUCION NORMAL
        // ==========================================
        vector<uint64_t> datosNormales = Generador::generarNormal(n, n / 2.0, n / 10.0);
        
        // --- Caso 1 Normal ---
        initC1 = high_resolution_clock::now();
        Caso1 c1_normal(datosNormales);
        finC1 = high_resolution_clock::now();
        t_const_c1 = duration_cast<milliseconds>(finC1 - initC1).count();

        initBusqC1 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c1_normal.buscar(rand() % (n * 2));
        }
        finBusqC1 = high_resolution_clock::now();
        t_busq_c1 = duration_cast<microseconds>(finBusqC1 - initBusqC1).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Normal", "Caso 1", t_const_c1, t_busq_c1, c1_normal.obtenerEspacio());
        
        // --- Caso 2 Normal ---
        initC2 = high_resolution_clock::now();
        Caso2 c2_normal(datosNormales, salto_b);
        finC2 = high_resolution_clock::now();
        t_const_c2 = duration_cast<milliseconds>(finC2 - initC2).count();

        initBusqC2 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c2_normal.buscar(rand() % (n * 2));
        }
        finBusqC2 = high_resolution_clock::now();
        t_busq_c2 = duration_cast<microseconds>(finBusqC2 - initBusqC2).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Normal", "Caso 2", t_const_c2, t_busq_c2, c2_normal.obtenerEspacio());

        // --- Caso 3 Normal ---
        initC3 = high_resolution_clock::now();
        PForDelta c3_normal(datosNormales, salto_b);
        finC3 = high_resolution_clock::now();
        t_const_c3 = duration_cast<milliseconds>(finC3 - initC3).count();

        initBusqC3 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c3_normal.buscar(rand() % (n * 2));
        }
        finBusqC3 = high_resolution_clock::now();
        t_busq_c3 = duration_cast<microseconds>(finBusqC3 - initBusqC3).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Normal", "Caso 3", t_const_c3, t_busq_c3, c3_normal.obtenerEspacio());
    }
    cout << "\nBenchmark completado. Datos guardados en '" << archivoMetricas << "'." << endl;
}

void ejecutarModoArchivo(const string& rutaArchivo){
    cout << "=== MODO ARCHIVO INTERACTIVO ===" << endl;
    cout << "Cargando datos desde: " << rutaArchivo << "..." << endl;

    vector<uint64_t> datos = leerArchivoCSV(rutaArchivo);
    if (datos.empty()) {
        cerr << "El archivo esta vacio o no se pudo procesar." << endl;
        return;
    }
    cout << "Se cargaron " << datos.size() << " elementos correctamente." << endl;
    
    // Construcción de estructuras base para la sesión interactiva
    Caso1 estructuraC1(datos);
    Caso2 estructuraC2(datos, 64);
    PForDelta estructuraC3(datos, 64); // Integrado el Caso 3

    long long entradaUsuario;
    int estructuraElegida;

    while (true) {
        cout << "\nMENU DE BUSQUEDA INTERACTIVA" << endl;
        cout << "1. Buscar en Caso 1 (Representacion Explicita)" << endl;
        cout << "2. Buscar en Caso 2 (Gap-Coding)" << endl;
        cout << "3. Buscar en Caso 3 (PForDelta)" << endl;
        cout << "4. Salir" << endl;
        cout << "Seleccione una estructura (1-4): ";
        cin >> estructuraElegida;

        if (estructuraElegida == 4) break;
        if (estructuraElegida < 1 || estructuraElegida > 3) {
            cout << "Opcion no valida." << endl;
            continue;
        }

        cout << "Ingrese el valor entero a buscar: ";
        cin >> entradaUsuario;
        
        uint64_t valorBuscado = static_cast<uint64_t>(entradaUsuario);
        int posicion = -1;
        double tiempoMicro = 0.0;

        auto inicio = high_resolution_clock::now();
        
        // Ruteo de la búsqueda según la opción
        if (estructuraElegida == 1) {
            posicion = estructuraC1.buscar(valorBuscado);
        } else if (estructuraElegida == 2) {
            posicion = estructuraC2.buscar(valorBuscado);
        } else if (estructuraElegida == 3) {
            posicion = estructuraC3.buscar(valorBuscado);
        }

        auto fin = high_resolution_clock::now();
        tiempoMicro = duration_cast<microseconds>(fin - inicio).count();

        if (posicion != -1) {
            cout << "-> Valor localizado en la posicion (indice logico): " << posicion << endl;
        } else {
            cout << "-> Valor no encontrado en la estructura." << endl;
        }
        cout << "-> Tiempo de busqueda: " << tiempoMicro << " nanosegundos." << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Error: Faltan argumentos de ejecucion." << endl;
        cerr << "Uso modo benchmark: " << argv[0] << " --benchmark" << endl;
        cerr << "Uso modo archivo:   " << argv[0] << " <ruta_archivo.csv>" << endl;
        return 1;
    }

    if (strcmp(argv[1], "--benchmark") == 0) {
        ejecutarBenchmark();
    } else {
        ejecutarModoArchivo(argv[1]);
    }

    return 0;
}