#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdlib>
//cabeceras
#include "generador.hpp"
#include "caso1.hpp"
#include "caso2.hpp"
#include "pfordelta.hpp"

using namespace std;
using namespace std::chrono;

//función para exportar métricas a un csv
void exportarMetrica(const string& rutaDestino, size_t n, const string& distribucion, const string& caso, double tiempoConstMS, double tiempoBusqMicro, size_t espacioBytes){
    ofstream archivo;
    //Se abre en modo append (ios_base::app) para no borrar los datos anteriores
    archivo.open(rutaDestino, ios_base::app);
    if(archivo.is_open()){
        archivo <<n<<","<<distribucion<<","<<caso<<","
                <<tiempoConstMS<<","<<tiempoBusqMicro<<","<<espacioBytes<<"\n";
        archivo.close();
    } else {
        cerr <<"error"<<endl;
    }
}

//función para leer el archivo csv usando atoll
vector<uint64_t> leerArchivoCSV(const string& ruta){
    vector<uint64_t> datos;
    ifstream archivo(ruta);
    if(!archivo.is_open()){
        cerr<<"error de ejecución"<<ruta<<endl;
        return datos;
    }

    string linea;
    while (getline(archivo,linea)){
        stringstream ss(linea);
        string celda;
        while (getline(ss,celda,',')){
            if(!celda.empty()){
                //conversión estricta con atoll
                long long valorEntero=atoll(celda.c_str());
                datos.push_back(static_cast<uint64_t>(valorEntero));
            }
        }
    }
    archivo.close();
    return datos;
}

void ejecutarBenchmark(){
    cout <<"modo benchmark automático"<<endl;
    string archivoMetricas="metricas.csv";
    //inicializar el archivo CSV con los encabezados
    ofstream archivoEncabezado(archivoMetricas);
    if(archivoEncabezado.is_open()){
        archivoEncabezado<<"Tamano,Distribucion,Caso,tiempoConstruccion_ms,TiempoBusquedaPromedio_us,Espacio_bytes\n";
        archivoEncabezado.close();
    }

    //tamaños de escala incremental en potencias de 10
    vector<size_t> tamanos={1000000,10000000};
    int numBusquedas=1000;

    for(size_t n:tamanos){
        cout<<"\nProcesando escala n="<<n<<"..."<<endl;

        //distribución lineal
        vector<uint64_t> datosLineales = Generador::generarLineal(n,10);

        //Caso 1: Representación Explícita
        auto initC1=high_resolution_clock::now();
        Caso1 c1_lineal(datosLineales);
        auto finC1 = high_resolution_clock::now();
        double t_const_c1 = duration_cast<milliseconds>(finC1 - initC1).count();

        // Medición de búsquedas para Caso 1
        auto initBusqC1 = high_resolution_clock::now();
        for (int i = 0; i < numBusquedas; ++i) {
            c1_lineal.buscar(rand() % (n * 2));
        }

        auto finBusqC1 = high_resolution_clock::now();
        double t_busq_c1 = duration_cast<microseconds>(finBusqC1 - initBusqC1).count() / (double)numBusquedas;

        exportarMetrica(archivoMetricas, n, "Lineal", "Caso 1", t_const_c1, t_busq_c1, c1_lineal.obtenerEspacio());

        // Caso 2: Gap-Coding (Salto b estático de ejemplo)
        int salto_b = 64; 
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
        
        // --- DISTRIBUCIÓN NORMAL ---
        vector<uint64_t> datosNormales = Generador::generarNormal(n, n / 2.0, n / 10.0);
        // Caso 1 Normal
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
        // Caso 2 Normal
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
    }
    cout << "\nBenchmark completado. Datos guardados en '" << archivoMetricas << "'." << endl;
}

void ejecutarModoArchivo(const string& rutaArchivo){
    cout << "MODO ARCHIVO INTERACTIVO" << endl;
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

    long long entradaUsuario;
    int estructuraElegida;

    while (true) {
        cout << "\nMENU DE BUSQUEDA INTERACTIVA" << endl;
        cout << "1. Buscar en Caso 1 (Representacion Explicita)" << endl;
        cout << "2. Buscar en Caso 2 (Gap-Coding)" << endl;
        cout << "3. Salir" << endl;
        cout << "Seleccione una estructura (1-3): ";
        cin >> estructuraElegida;

        if (estructuraElegida == 3) break;
        if (estructuraElegida != 1 && estructuraElegida != 2) {
            cout << "Opcion no válida." << endl;
            continue;
        }

        cout << "Ingrese el valor entero a buscar: ";
        cin >> entradaUsuario;
        
        uint64_t valorBuscado = static_cast<uint64_t>(entradaUsuario);
        int posicion = -1;
        double tiempoMicro = 0.0;

        if (estructuraElegida == 1) {
            auto inicio = high_resolution_clock::now();
            posicion = estructuraC1.buscar(valorBuscado);
            auto fin = high_resolution_clock::now();
            tiempoMicro = duration_cast<microseconds>(fin - inicio).count();
        } else if (estructuraElegida == 2) {
            auto inicio = high_resolution_clock::now();
            posicion = estructuraC2.buscar(valorBuscado);
            auto fin = high_resolution_clock::now();
            tiempoMicro = duration_cast<microseconds>(fin - inicio).count();
        }

        if (posicion != -1) {
            cout << "-> Valor localizado en la posicion: " << posicion << endl;
        } else {
            cout << "-> Valor no encontrado en la estructura." << endl;
        }
        cout << "-> Tiempo de busqueda: " << tiempoMicro << " microsegundos." << endl;
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