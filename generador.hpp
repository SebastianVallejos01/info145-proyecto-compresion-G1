#ifndef GENERADOR_HPP
#define GENERADOR_HPP

#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <cmath>

class Generador {
public:
    // Generación de Distribución Lineal
    // Fórmula basada en: A[0]=rand(), A[i]=A[i-1] + rand() % epsilon
    static std::vector<uint64_t> generarLineal(size_t n, uint64_t epsilon) {
        std::vector<uint64_t> arreglo(n);
        
        // Inicializar el motor de números aleatorios de 64 bits (mt19937_64)
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> distBase(1, 1000); // Semilla inicial
        std::uniform_int_distribution<uint64_t> distEpsilon(1, epsilon);

        if (n > 0) {
            arreglo[0] = distBase(gen);
            for (size_t i = 1; i < n; ++i) {
                arreglo[i] = arreglo[i - 1] + distEpsilon(gen);
            }
        }
        // Ya se genera ordenado por la naturaleza de la suma incremental
        return arreglo;
    }

    // 2. Generación de Distribución Normal (Gaussiana)
    static std::vector<uint64_t> generarNormal(size_t n, double media, double stddev) {
        std::vector<uint64_t> arreglo(n);
        
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::normal_distribution<double> distNormal(media, stddev);

        for (size_t i = 0; i < n; ++i) {
            double valor = distNormal(gen);
            // Aseguramos que los valores sean positivos absolutos
            arreglo[i] = static_cast<uint64_t>(std::abs(valor)); 
        }

        // Esta distribución NO genera los datos ordenados automáticamente, 
        // por lo que debemos ordenarlos explícitamente de menor a mayor.
        std::sort(arreglo.begin(), arreglo.end());
        
        return arreglo;
    }
};

#endif