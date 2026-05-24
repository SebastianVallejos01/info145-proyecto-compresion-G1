#ifndef CASO1_HPP
#define CASO1_HPP

#include <vector>
#include <cstdint>

class Caso1 {
private:
    std::vector<uint64_t> arreglo;

public:
    Caso1(const std::vector<uint64_t>& datos) {
        arreglo = datos;
    }

    // Búsqueda binaria 
    int buscar(uint64_t valor) {
        int left = 0;
        int right = arreglo.size() - 1;

        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (arreglo[mid] == valor) return mid;
            if (arreglo[mid] < valor) left = mid + 1;
            else right = mid - 1;
        }
        return -1; // No encontrado
    }
    
    // Retorna el espacio ocupado en bytes
    size_t obtenerEspacio() {
        return arreglo.size() * sizeof(uint64_t);
    }
};

#endif