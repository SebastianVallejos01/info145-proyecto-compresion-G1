#ifndef CASO2_HPP
#define CASO2_HPP

#include <vector>
#include <cstdint>

class Caso2 {
private:
    std::vector<uint64_t> GC;      
    std::vector<uint64_t> sample;  
    int salto_b;                   

public:
    // Constructor base
    Caso2(const std::vector<uint64_t>& A, int b) {
        salto_b = b;
        if (A.empty()) return;

        // Estructura inicial temporal para que el main funcione
        GC.push_back(A[0]);
        for (size_t i = 1; i < A.size(); ++i) {
            GC.push_back(A[i] - A[i - 1]);
        }
        for (size_t i = 0; i < A.size(); i += salto_b) {
            sample.push_back(A[i]);
        }
    }

    // reescribir esta búsqueda con la lógica de rangos
    int buscar(uint64_t valor) {
        //-1 temporalmente para compilar
        return -1; 
    }

    size_t obtenerEspacio() {
        return (GC.size() * sizeof(uint64_t)) + (sample.size() * sizeof(uint64_t));
    }
};

#endif