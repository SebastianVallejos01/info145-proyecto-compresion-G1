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
        cerr << "Error al abrir archivo de metricas\n";
    }
}

// Función para leer el archivo CSV usando atoll
vector<uint64_t> leerArchivoCSV(const string& ruta){
    vector<uint64_t> datos;
    ifstream archivo(ruta);
    if(!archivo.is_open()){
        cerr << "Error de ejecucion: no se pudo abrir " << ruta << "\n";
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
    cout << "=== MODO BENCHMARK AUTOMATICO ===\n";
    string archivoMetricas = "metricas.csv";
    
    // Inicializar el archivo CSV con los encabezados
    ofstream archivoEncabezado(archivoMetricas);
    if(archivoEncabezado.is_open()){
        archivoEncabezado << "Tamano,Distribucion,Caso,TiempoConstruccion_ms,TiempoBusquedaPromedio_us,Espacio_bytes\n";
        archivoEncabezado.close();
    }

    // Tamaños de escala incremental en potencias de 10
    // Se incluyen tres escalas (10^6, 10^7, 10^8) para obtener suficientes
    // puntos en los gráficos de tiempo vs n y espacio vs n del Hito 2
    vector<size_t> tamanos = {1000000, 10000000, 100000000};
    int numBusquedas = 1000;
    size_t salto_b = 64; // Salto parametrizado para Caso 2 y 3

    // Distintas desviaciones estándar para la distribución normal.
    // Se evalúan tres propuestas: dispersión baja (n/20), media (n/10) y alta (n/4),
    // para observar cómo afecta la distribución de gaps y la compresión resultante.
    // Cada par (stddev, etiqueta) se itera junto con los tamaños de arreglo.
    vector<pair<double, string>> desviaciones = {
        {1.0 / 20.0, "Normal_stddev_bajo"},   // sigma = n/20  → distribución concentrada
        {1.0 / 10.0, "Normal_stddev_medio"},   // sigma = n/10  → distribución media
        {1.0 /  4.0, "Normal_stddev_alto"}     // sigma = n/4   → distribución dispersa
    };

    for(size_t n : tamanos){
        cout << "\nProcesando escala n=" << n << "...\n";

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
        // Se itera sobre las distintas desviaciones estándar definidas arriba
        for (const auto& [factorStd, etiqueta] : desviaciones) {
            double stddev = n * factorStd;
            vector<uint64_t> datosNormales = Generador::generarNormal(n, n / 2.0, stddev);

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

            exportarMetrica(archivoMetricas, n, etiqueta, "Caso 1", t_const_c1, t_busq_c1, c1_normal.obtenerEspacio());
            
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

            exportarMetrica(archivoMetricas, n, etiqueta, "Caso 2", t_const_c2, t_busq_c2, c2_normal.obtenerEspacio());

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

            exportarMetrica(archivoMetricas, n, etiqueta, "Caso 3", t_const_c3, t_busq_c3, c3_normal.obtenerEspacio());
        }
    }
    cout << "\nBenchmark completado. Datos guardados en '" << archivoMetricas << "'.\n";
}

void ejecutarModoArchivo(const string& rutaArchivo){
    cout << "=== MODO ARCHIVO INTERACTIVO ===\n";
    cout << "Cargando datos desde: " << rutaArchivo << "...\n";

    vector<uint64_t> datos = leerArchivoCSV(rutaArchivo);
    if (datos.empty()) {
        cerr << "El archivo esta vacio o no se pudo procesar.\n";
        return;
    }
    cout << "Se cargaron " << datos.size() << " elementos correctamente.\n";
    
    // Construcción de estructuras base para la sesión interactiva
    Caso1    estructuraC1(datos);
    Caso2    estructuraC2(datos, 64);
    PForDelta estructuraC3(datos, 64);

    long long entradaUsuario;
    int estructuraElegida;

    while (true) {
        cout << "\nMENU DE BUSQUEDA INTERACTIVA\n";
        cout << "1. Buscar en Caso 1 (Representacion Explicita)\n";
        cout << "2. Buscar en Caso 2 (Gap-Coding)\n";
        cout << "3. Buscar en Caso 3 (PForDelta)\n";
        cout << "4. Salir\n";
        cout << "Seleccione una estructura (1-4): ";
        cin >> estructuraElegida;

        if (estructuraElegida == 4) break;
        if (estructuraElegida < 1 || estructuraElegida > 3) {
            cout << "Opcion no valida.\n";
            continue;
        }

        cout << "Ingrese el valor entero a buscar: ";
        cin >> entradaUsuario;
        
        uint64_t valorBuscado = static_cast<uint64_t>(entradaUsuario);
        int posicion = -1;
        double tiempoMicro = 0.0;

        auto inicio = high_resolution_clock::now();
        
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
            cout << "-> Valor localizado en la posicion (indice logico): " << posicion << "\n";
        } else {
            cout << "-> Valor no encontrado en la estructura.\n";
        }
        // Punto 19: corregido "nanosegundos" — la medición es con microseconds, no nanoseconds
        cout << "-> Tiempo de busqueda: " << tiempoMicro << " microsegundos.\n";
    }
}

int main(int argc, char* argv[]) {
    // Modo benchmark: ./main --benchmark
    // Modo archivo:   ./main -i <ruta_archivo.csv>
    if (argc < 2) {
        cerr << "Error: Faltan argumentos de ejecucion.\n";
        cerr << "Uso modo benchmark: " << argv[0] << " --benchmark\n";
        cerr << "Uso modo archivo:   " << argv[0] << " -i <ruta_archivo.csv>\n";
        return 1;
    }

    if (strcmp(argv[1], "--benchmark") == 0) {
        ejecutarBenchmark();
    } else if (strcmp(argv[1], "-i") == 0) {
        // El flag -i requiere un segundo argumento con la ruta del archivo
        if (argc < 3) {
            cerr << "Error: El flag -i requiere una ruta de archivo.\n";
            cerr << "Uso modo archivo: " << argv[0] << " -i <ruta_archivo.csv>\n";
            return 1;
        }
        ejecutarModoArchivo(argv[2]);
    } else {
        cerr << "Error: Argumento no reconocido '" << argv[1] << "'.\n";
        cerr << "Uso modo benchmark: " << argv[0] << " --benchmark\n";
        cerr << "Uso modo archivo:   " << argv[0] << " -i <ruta_archivo.csv>\n";
        return 1;
    }

    return 0;
}
