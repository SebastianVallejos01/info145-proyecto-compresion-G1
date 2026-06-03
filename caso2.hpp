#ifndef CASO2_HPP
#define CASO2_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

class Caso2 {
private:
    // Arreglo GC
    std::vector<uint64_t> GC;

    // Arreglo Sample
    std::vector<uint64_t> Sample;

    // Variable para saber cada cuántos elementos tomamos una muestra
    size_t b;

public:
    // Recibe el arreglo original A y construye GC y Sample
    // Consistencia con PForDelta: parámetro salto como size_t (no int implícito)
    Caso2(const std::vector<uint64_t>& A, size_t salto) {
        b = salto;
        size_t n = A.size();

        // Reservamos memoria para evitar realocaciones costosas
        GC.reserve(n);

        // Reservamos también el Sample para evitar realocaciones
        size_t num_muestras = (n + b - 1) / b;
        Sample.reserve(num_muestras);

        for (size_t i = 0; i < n; ++i) {
            // Calculamos y guardamos el Gap
            if (i == 0) {
                GC.push_back(A[0]); // El primer elemento se guarda tal cual
            } else {
                GC.push_back(A[i] - A[i - 1]); // Los demás son la diferencia con el anterior
            }
            // Guardamos en Sample cada vez que el índice sea múltiplo de b
            if (i % b == 0) {
                Sample.push_back(A[i]);
            }
        }
    }

    // Retorna el índice donde está el elemento, o -1 si no existe
    int buscar(uint64_t valor) const {
        // El arreglo está vacío o el valor es menor al mínimo posible
        if (Sample.empty() || valor < Sample[0]) {
            return -1;
        }

        // Búsqueda binaria en Sample
        int low  = 0;
        int high = static_cast<int>(Sample.size()) - 1;
        int idx_sample = 0;

        while (low <= high) {
            int mid = low + (high - low) / 2;

            if (Sample[mid] == valor) {
                // Caso ideal: el valor es exactamente una de nuestras muestras
                return static_cast<int>(mid * b);
            } else if (Sample[mid] < valor) {
                // El valor podría estar más adelante, guardamos este índice
                idx_sample = mid;
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        // Decodificación secuencial en GC
        // Calculamos los índices reales [L, R) en base al salto b
        size_t L = static_cast<size_t>(idx_sample) * b;

        // R es el límite superior (la siguiente muestra o el final del arreglo GC)
        size_t R = L + b;
        if (R > GC.size()) {
            R = GC.size();
        }

        // Reconstruimos a partir de la muestra encontrada
        uint64_t valor_actual = Sample[idx_sample];

        // Iteramos desde L+1 hasta R-1 sumando los gaps
        for (size_t i = L + 1; i < R; ++i) {
            valor_actual += GC[i]; // Decodificamos el gap actual

            if (valor_actual == valor) {
                return static_cast<int>(i);
            } else if (valor_actual > valor) {
                // Si nos pasamos, el número no está en el arreglo
                return -1;
            }
        }

        return -1; // Por si termina el ciclo sin encontrarlo
    }

    // Función auxiliar para saber cuánta memoria ocupa la estructura
    size_t obtenerEspacio() const {
        return sizeof(GC)     + (GC.capacity()     * sizeof(uint64_t))
             + sizeof(Sample) + (Sample.capacity() * sizeof(uint64_t))
             + sizeof(b);
    }
};

#endif // CASO2_HPP