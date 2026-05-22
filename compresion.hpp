#ifndef COMPRESION_HPP
#define COMPRESION_HPP

#include <vector>
#include <cstdint>
#include <algorithm>
#include <iostream>

// El enunciado especifica bloques de tamaño fijo de 128 elementos
const size_t TAMANO_BLOQUE = 128;

// Estructura para registrar un outlier
struct Outlier {
    size_t posicion_local; // Indice de la anomalia dentro de su bloque (0 a 127)
    uint32_t valor;        // El valor real del gap original
};

// Estructura para almacenar cada bloque comprimido de forma independiente
struct BloquePForDelta {
    // Indicador del tamaño del tipo de dato elegido: 1 = uint8_t, 2 = uint16_t, 4 = uint32_t
    uint8_t tipo_byte; 
    
    // Vectores para alojar los datos comunes del bloque segun corresponda
    std::vector<uint8_t>  datos_u8;
    std::vector<uint16_t> datos_u16;
    std::vector<uint32_t> datos_u32;
    
    // Lista con todos los outliers detectados dentro de este bloque
    std::vector<Outlier> outliers;
};

class PForDeltaCoder {
private:
    std::vector<BloquePForDelta> bloques;
    size_t total_gaps;

public:
    // El constructor recibe el vector con todos los gaps generados en el Caso 2
    PForDeltaCoder(const std::vector<uint32_t>& gaps_originales) {
        total_gaps = gaps_originales.size();
        
        // Recorremos el arreglo original dividiendolo en trozos de 128 elementos
        for (size_t i = 0; i < total_gaps; i += TAMANO_BLOQUE) {
            std::vector<uint32_t> bloque_temporal;
            
            // Construimos manualmente un sub-bloque cuidando de no pasarnos del limite final
            for (size_t j = 0; j < TAMANO_BLOQUE && (i + j) < total_gaps; ++j) {
                bloque_temporal.push_back(gaps_originales[i + j]);
            }
            
            // Comprimimos el bloque temporal y lo guardamos en nuestro contenedor general
            BloquePForDelta bloque_comprimido = comprimirBloque(bloque_temporal);
            bloques.push_back(bloque_comprimido);
        }
    }

    // Funcion para obtener/descomprimir un elemento cualquiera en tiempo constante O(1)
    uint32_t decompress_gap(size_t indice_global) const {
        // Determinamos a que bloque pertenece el indice y su posicion local
        size_t num_bloque = indice_global / TAMANO_BLOQUE;
        size_t idx_local = indice_global % TAMANO_BLOQUE;
        
        const BloquePForDelta& bloque = bloques[num_bloque];
        
        // 1. Buscamos primero en la lista de outliers de este bloque
        for (size_t i = 0; i < bloque.outliers.size(); ++i) {
            if (bloque.outliers[i].posicion_local == idx_local) {
                return bloque.outliers[i].valor; // Si coincide, retornamos el valor real del outlier
            }
        }
        
        // 2. Si no es un outlier, extraemos el elemento del vector correspondiente
        if (bloque.tipo_byte == 1) {
            return bloque.datos_u8[idx_local];
        } else if (bloque.tipo_byte == 2) {
            return bloque.datos_u16[idx_local];
        } else {
            return bloque.datos_u32[idx_local];
        }
    }

    // Calcula de forma transparente cuanta memoria en bytes utiliza esta estructura
    size_t getEspacioUtilizado() const {
        size_t memoria_total = 0;
        
        for (size_t i = 0; i < bloques.size(); ++i) {
            memoria_total += sizeof(bloques[i].tipo_byte);
            memoria_total += bloques[i].datos_u8.capacity() * sizeof(uint8_t);
            memoria_total += bloques[i].datos_u16.capacity() * sizeof(uint16_t);
            memoria_total += bloques[i].datos_u32.capacity() * sizeof(uint32_t);
            memoria_total += bloques[i].outliers.capacity() * sizeof(Outlier);
        }
        
        // Sumamos las variables de control internas de la clase
        memoria_total += sizeof(total_gaps) + (bloques.capacity() * sizeof(BloquePForDelta));
        return memoria_total;
    }
    
    // Funcion de busqueda para el Caso 3
    // Recibe el valor a buscar, el arreglo Sample del Caso 2 y el salto parametrizado 'b'
    int buscar(uint32_t valor, const std::vector<uint32_t>& sample, size_t b) const {
        // Si no hay muestras o el valor es menor que la primera muestra, el numero no existe
        if (sample.empty() || valor < sample[0]) {
            return -1;
        }

        // A. Realizamos la busqueda binaria sobre el Sample para acotar el rango [L, R]
        int low = 0;
        int high = sample.size() - 1;
        int idx_sample = 0;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            
            if (sample[mid] == valor) {
                return mid * b; // Encontrado directamente en la muestra
            } else if (sample[mid] < valor) {
                idx_sample = mid; // Guardamos esta muestra como el limite inferior tentativo
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        // B. Calculamos los limites de búsqueda exacta dentro de PForDelta
        size_t L = idx_sample * b;
        size_t R = L + b;
        if (R > total_gaps) {
            R = total_gaps; // Evitamos salirnos del tamaño del arreglo
        }

        // Partimos con el valor absoluto acumulado de la muestra base
        uint32_t valor_actual = sample[idx_sample];

        // C. Navegacion secuencial descomprimiendo los gaps en formato PForDelta
        for (size_t i = L + 1; i < R; ++i) {
            // En lugar de usar un arreglo directo, llamamos a tu función que recupera el gap original
            valor_actual += decompress_gap(i); 
            
            if (valor_actual == valor) {
                return i; // Encontrado con éxito
            } else if (valor_actual > valor) {
                return -1; // Si nos pasamos del valor buscado, significa que no existe en el arreglo
            }
        }

        return -1; // No se encontro en el rango
    }

private:
    // Proceso interno para analizar y codificar un bloque individual de 128 elementos
    BloquePForDelta comprimirBloque(const std::vector<uint32_t>& bloque_gaps) {
        BloquePForDelta nuevo_bloque;
        
        // 1. Ordenamos una copia para hallar el valor frontera del 90%
        std::vector<uint32_t> copia_ordenada = bloque_gaps;
        std::sort(copia_ordenada.begin(), copia_ordenada.end());
        
        size_t indice_umbral = (copia_ordenada.size() * 90) / 100;
        uint32_t umbral = copia_ordenada[indice_umbral];
        
        // 2. Elegimos el contenedor de menor espacio disponible segun el umbral calculado
        if (umbral <= 255) {
            nuevo_bloque.tipo_byte = 1; // Usara uint8_t (1 byte)
        } else if (umbral <= 65535) {
            nuevo_bloque.tipo_byte = 2; // Usara uint16_t (2 bytes)
        } else {
            nuevo_bloque.tipo_byte = 4; // Usara uint32_t (4 bytes)
        }
        
        // 3. Iteramos el bloque para clasificar elementos normales y separar outliers
        for (size_t i = 0; i < bloque_gaps.size(); ++i) {
            uint32_t gap = bloque_gaps[i];
            
            if (gap <= umbral) {
                // Almacenamos el elemento regular directamente en su tipo correspondiente
                if (nuevo_bloque.tipo_byte == 1) {
                    nuevo_bloque.datos_u8.push_back(static_cast<uint8_t>(gap));
                } else if (nuevo_bloque.tipo_byte == 2) {
                    nuevo_bloque.datos_u16.push_back(static_cast<uint16_t>(gap));
                } else {
                    nuevo_bloque.datos_u32.push_back(gap);
                }
            } else {
                // Es un outlier: insertamos un 0 de relleno en la secuencia principal
                if (nuevo_bloque.tipo_byte == 1) nuevo_bloque.datos_u8.push_back(0);
                else if (nuevo_bloque.tipo_byte == 2) nuevo_bloque.datos_u16.push_back(0);
                else nuevo_bloque.datos_u32.push_back(0);
                
                // Registramos los metadatos completos del outlier por separado
                Outlier registro_outlier;
                registro_outlier.posicion_local = i;
                registro_outlier.valor = gap;
                nuevo_bloque.outliers.push_back(registro_outlier);
            }
        }
        
        return nuevo_bloque;
    }
};

#endif // COMPRESION_HPP