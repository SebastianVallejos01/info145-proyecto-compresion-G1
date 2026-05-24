#ifndef PFORDELTA_HPP
#define PFORDELTA_HPP

#include <vector>
#include <cstdint>
#include <algorithm>

// El enunciado especifica bloques de tamaño fijo de 128 elementos
const size_t TAMANO_BLOQUE = 128;

// Estructura para registrar un outlier (Adaptado a 64 bits)
struct Outlier {
    size_t posicion_local; 
    uint64_t valor;        
};

// Estructura para almacenar cada bloque comprimido
struct BloquePForDelta {
    uint8_t tipo_byte; 
    std::vector<uint8_t>  datos_u8;
    std::vector<uint16_t> datos_u16;
    std::vector<uint32_t> datos_u32;
    std::vector<Outlier> outliers;
};

class PForDelta {
private:
    std::vector<BloquePForDelta> bloques;
    std::vector<uint64_t> sample; // Se guarda el sample internamente
    size_t total_gaps;
    size_t salto_b; // Se guarda el salto internamente

    // Proceso interno de tu compañero (Adaptado a 64 bits)
    BloquePForDelta comprimirBloque(const std::vector<uint64_t>& bloque_gaps) {
        BloquePForDelta nuevo_bloque;
        std::vector<uint64_t> copia_ordenada = bloque_gaps;
        std::sort(copia_ordenada.begin(), copia_ordenada.end());
        
        size_t indice_umbral = (copia_ordenada.size() * 90) / 100;
        uint64_t umbral = copia_ordenada[indice_umbral];
        
        if (umbral <= 255) {
            nuevo_bloque.tipo_byte = 1;
        } else if (umbral <= 65535) {
            nuevo_bloque.tipo_byte = 2;
        } else {
            nuevo_bloque.tipo_byte = 4;
        }
        
        for (size_t i = 0; i < bloque_gaps.size(); ++i) {
            uint64_t gap = bloque_gaps[i];
            
            if (gap <= umbral) {
                if (nuevo_bloque.tipo_byte == 1) {
                    nuevo_bloque.datos_u8.push_back(static_cast<uint8_t>(gap));
                } else if (nuevo_bloque.tipo_byte == 2) {
                    nuevo_bloque.datos_u16.push_back(static_cast<uint16_t>(gap));
                } else {
                    nuevo_bloque.datos_u32.push_back(static_cast<uint32_t>(gap));
                }
            } else {
                if (nuevo_bloque.tipo_byte == 1) nuevo_bloque.datos_u8.push_back(0);
                else if (nuevo_bloque.tipo_byte == 2) nuevo_bloque.datos_u16.push_back(0);
                else nuevo_bloque.datos_u32.push_back(0);
                
                Outlier registro_outlier;
                registro_outlier.posicion_local = i;
                registro_outlier.valor = gap;
                nuevo_bloque.outliers.push_back(registro_outlier);
            }
        }
        return nuevo_bloque;
    }

    uint64_t decompress_gap(size_t indice_global) const {
        size_t num_bloque = indice_global / TAMANO_BLOQUE;
        size_t idx_local = indice_global % TAMANO_BLOQUE;
        const BloquePForDelta& bloque = bloques[num_bloque];
        
        for (size_t i = 0; i < bloque.outliers.size(); ++i) {
            if (bloque.outliers[i].posicion_local == idx_local) {
                return bloque.outliers[i].valor; 
            }
        }
        
        if (bloque.tipo_byte == 1) return bloque.datos_u8[idx_local];
        else if (bloque.tipo_byte == 2) return bloque.datos_u16[idx_local];
        else return bloque.datos_u32[idx_local];
    }

public:
    // CONSTRUCTOR ADAPTADO PARA EL MAIN
    PForDelta(const std::vector<uint64_t>& A, int b) {
        salto_b = b;
        if (A.empty()) return;
        
        total_gaps = A.size();
        std::vector<uint64_t> gaps_temporales(total_gaps);

        // Generamos los gaps y el sample internamente como en el Caso 2
        for (size_t i = 0; i < total_gaps; ++i) {
            if (i == 0) gaps_temporales[i] = A[0];
            else gaps_temporales[i] = A[i] - A[i - 1];

            if (i % salto_b == 0) sample.push_back(A[i]);
        }
        for (size_t i = 0; i < total_gaps; i += TAMANO_BLOQUE) {
            std::vector<uint64_t> bloque_temporal;
            for (size_t j = 0; j < TAMANO_BLOQUE && (i + j) < total_gaps; ++j) {
                bloque_temporal.push_back(gaps_temporales[i + j]);
            }
            bloques.push_back(comprimirBloque(bloque_temporal));
        }
    }

    // FUNCION DE BUSQUEDA ADAPTADA (Solo pide el valor)
    int buscar(uint64_t valor) const {
        if (sample.empty() || valor < sample[0]) return -1;

        int low = 0;
        int high = sample.size() - 1;
        int idx_sample = 0;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (sample[mid] == valor) return mid * salto_b; 
            else if (sample[mid] < valor) {
                idx_sample = mid; 
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        size_t L = idx_sample * salto_b;
        size_t R = L + salto_b;
        if (R > total_gaps) R = total_gaps; 

        uint64_t valor_actual = sample[idx_sample];

        for (size_t i = L + 1; i < R; ++i) {
            valor_actual += decompress_gap(i); 
            if (valor_actual == valor) return i; 
            else if (valor_actual > valor) return -1; 
        }
        return -1; 
    }

    // FUNCION DE MEMORIA ADAPTADA
    size_t obtenerEspacio() const {
        size_t memoria_total = sample.capacity() * sizeof(uint64_t);
        for (size_t i = 0; i < bloques.size(); ++i) {
            memoria_total += sizeof(bloques[i].tipo_byte);
            memoria_total += bloques[i].datos_u8.capacity() * sizeof(uint8_t);
            memoria_total += bloques[i].datos_u16.capacity() * sizeof(uint16_t);
            memoria_total += bloques[i].datos_u32.capacity() * sizeof(uint32_t);
            memoria_total += bloques[i].outliers.capacity() * sizeof(Outlier);
        }
        memoria_total += sizeof(total_gaps) + sizeof(salto_b) + (bloques.capacity() * sizeof(BloquePForDelta));
        return memoria_total;
    }
};

#endif