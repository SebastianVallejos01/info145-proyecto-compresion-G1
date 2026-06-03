#ifndef PFORDELTA_HPP
#define PFORDELTA_HPP

#include <vector>
#include <cstdint>
#include <algorithm>

// Punto 16: constexpr en lugar de const para constante de tiempo de compilación
constexpr size_t TAMANO_BLOQUE = 128;
// Máximo de outliers por bloque (10% de 128 = 12, redondeado arriba)
constexpr size_t MAX_OUTLIERS  = 13;

// ─────────────────────────────────────────────────────────────────────────────
// Punto 17: structs dentro de un namespace propio para evitar colisiones
// ─────────────────────────────────────────────────────────────────────────────
namespace pfd {

// Punto 11: nombre más descriptivo que "tipo_byte"
enum class AnchoByte : uint8_t {
    U8  = 1,
    U16 = 2,
    U32 = 4
};

// Registro de un outlier: posición local dentro del bloque + valor real
struct Outlier {
    uint8_t  posicion_local; // Máx 127, uint8_t es suficiente
    uint64_t valor;
};

// Punto 9: un solo vector de bytes como almacenamiento subyacente
//          + arreglo fijo de outliers para acceso O(1) por posición local
// Punto 8: tabla de lookup de tamaño fijo para que la resolución de outliers
//          sea O(1) en lugar de O(k)
struct BloquePForDelta {
    AnchoByte bytes_por_elemento;           // Ancho del tipo elegido (1, 2 o 4 bytes)
    uint8_t   bits_por_elemento;            // b real en bits (para análisis teórico)
    uint8_t   num_outliers;                 // Cantidad de outliers en este bloque
    uint8_t   num_elementos;                // Elementos reales (último bloque puede ser < 128)

    // Punto 9: almacenamiento unificado en bytes con casting manual
    std::vector<uint8_t> datos;             // Tamaño: num_elementos * bytes_por_elemento

    // Punto 8: lookup O(1) — índice = posición local, valor = índice en outliers[]
    //          0xFF significa "no es outlier"
    uint8_t lookup_outlier[TAMANO_BLOQUE];

    // Punto 17: arreglo fijo, sin overhead de vector
    Outlier outliers[MAX_OUTLIERS];

    BloquePForDelta() : bytes_por_elemento(AnchoByte::U8),
                        bits_por_elemento(0),
                        num_outliers(0),
                        num_elementos(0) {
        // Inicializamos el lookup: 0xFF = "no es outlier"
        std::fill(lookup_outlier, lookup_outlier + TAMANO_BLOQUE, 0xFF);
    }
};

} // namespace pfd

// ─────────────────────────────────────────────────────────────────────────────
// Clase principal
// ─────────────────────────────────────────────────────────────────────────────
class PForDelta {
private:
    std::vector<pfd::BloquePForDelta> bloques;
    std::vector<uint64_t>             sample;
    size_t                            total_gaps;
    // Punto 15: renombrado a salto_sample para no confundir con b (ancho en bits)
    size_t                            salto_sample;

    // ─────────────────────────────────────────────────────────────────────────
    // Comprime un bloque de gaps y retorna el BloquePForDelta resultante
    // Punto 12: marcado const ya que no modifica estado del objeto
    // ─────────────────────────────────────────────────────────────────────────
    pfd::BloquePForDelta comprimirBloque(const std::vector<uint64_t>& bloque_gaps) const {
        pfd::BloquePForDelta nuevo_bloque;
        nuevo_bloque.num_elementos = static_cast<uint8_t>(bloque_gaps.size());

        // ── Punto 13: nth_element es O(n) en lugar de sort O(n log n) ────────
        // Copiamos solo para encontrar el percentil 90
        std::vector<uint64_t> copia = bloque_gaps;
        // Punto 1: índice correcto para el 90° percentil (0-indexado)
        //          queremos el valor tal que el 90% de elementos sean <= a él
        size_t indice_p90 = (copia.size() * 9) / 10; // ej: 128*9/10 = 115 → cubre índices 0..114
        if (indice_p90 >= copia.size()) indice_p90 = copia.size() - 1;
        std::nth_element(copia.begin(), copia.begin() + indice_p90, copia.end());
        uint64_t umbral = copia[indice_p90];

        // ── Elegir el tipo más pequeño que contenga el umbral ─────────────────
        if (umbral <= 255ULL) {
            nuevo_bloque.bytes_por_elemento = pfd::AnchoByte::U8;
            // b real: mínimo de bits para representar el umbral
            nuevo_bloque.bits_por_elemento  = (umbral == 0) ? 1
                                            : static_cast<uint8_t>(64 - __builtin_clzll(umbral));
        } else if (umbral <= 65535ULL) {
            nuevo_bloque.bytes_por_elemento = pfd::AnchoByte::U16;
            nuevo_bloque.bits_por_elemento  = static_cast<uint8_t>(64 - __builtin_clzll(umbral));
        } else {
            nuevo_bloque.bytes_por_elemento = pfd::AnchoByte::U32;
            nuevo_bloque.bits_por_elemento  = static_cast<uint8_t>(64 - __builtin_clzll(umbral));
        }

        size_t bpe = static_cast<size_t>(nuevo_bloque.bytes_por_elemento);
        nuevo_bloque.datos.resize(bloque_gaps.size() * bpe, 0);

        // ── Empaquetar gaps ───────────────────────────────────────────────────
        for (size_t i = 0; i < bloque_gaps.size(); ++i) {
            uint64_t gap = bloque_gaps[i];

            if (gap <= umbral) {
                // Gap normal: escribir directamente en datos[]
                // Punto 3: solo escribimos el gap real; los outliers usan el lookup,
                //           NO un valor centinela como 0 que puede ser un gap válido
                escribirEnDatos(nuevo_bloque.datos, i, bpe, gap);
            } else {
                // Outlier: dejar datos[] en 0 (placeholder) y registrar en lookup
                escribirEnDatos(nuevo_bloque.datos, i, bpe, 0ULL);

                // Punto 8: registro en lookup O(1)
                uint8_t idx_outlier = nuevo_bloque.num_outliers;
                nuevo_bloque.lookup_outlier[i] = idx_outlier;
                nuevo_bloque.outliers[idx_outlier].posicion_local = static_cast<uint8_t>(i);
                nuevo_bloque.outliers[idx_outlier].valor           = gap;
                ++nuevo_bloque.num_outliers;
            }
        }
        return nuevo_bloque;
    }

    // Escribe `valor` en `datos` en la posición `idx` usando `bpe` bytes
    static void escribirEnDatos(std::vector<uint8_t>& datos,
                                size_t idx, size_t bpe, uint64_t valor) {
        size_t offset = idx * bpe;
        if (bpe == 1) {
            datos[offset] = static_cast<uint8_t>(valor);
        } else if (bpe == 2) {
            uint16_t v = static_cast<uint16_t>(valor);
            datos[offset]     = static_cast<uint8_t>(v & 0xFF);
            datos[offset + 1] = static_cast<uint8_t>(v >> 8);
        } else { // bpe == 4
            uint32_t v = static_cast<uint32_t>(valor);
            datos[offset]     = static_cast<uint8_t>(v & 0xFF);
            datos[offset + 1] = static_cast<uint8_t>((v >> 8)  & 0xFF);
            datos[offset + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
            datos[offset + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
        }
    }

    // Lee `bpe` bytes desde `datos` en la posición `idx` y retorna el valor
    static uint64_t leerDeDatos(const std::vector<uint8_t>& datos,
                                size_t idx, size_t bpe) {
        size_t offset = idx * bpe;
        if (bpe == 1) {
            return datos[offset];
        } else if (bpe == 2) {
            return static_cast<uint64_t>(datos[offset])
                 | (static_cast<uint64_t>(datos[offset + 1]) << 8);
        } else { // bpe == 4
            return static_cast<uint64_t>(datos[offset])
                 | (static_cast<uint64_t>(datos[offset + 1]) << 8)
                 | (static_cast<uint64_t>(datos[offset + 2]) << 16)
                 | (static_cast<uint64_t>(datos[offset + 3]) << 24);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Decodifica el gap en la posición global `indice_global`
    // Punto 2 + 8: primero lookup O(1), luego acceso directo O(1) a datos[]
    // ─────────────────────────────────────────────────────────────────────────
    uint64_t decompress_gap(size_t indice_global) const {
        size_t num_bloque = indice_global / TAMANO_BLOQUE;
        size_t idx_local  = indice_global % TAMANO_BLOQUE;
        const pfd::BloquePForDelta& bloque = bloques[num_bloque];

        // Punto 8: lookup O(1) — si hay outlier en esta posición, retornarlo
        uint8_t idx_outlier = bloque.lookup_outlier[idx_local];
        if (idx_outlier != 0xFF) {
            return bloque.outliers[idx_outlier].valor;
        }

        // Acceso directo O(1) al dato comprimido
        size_t bpe = static_cast<size_t>(bloque.bytes_por_elemento);
        return leerDeDatos(bloque.datos, idx_local, bpe);
    }

public:
    // ─────────────────────────────────────────────────────────────────────────
    // Constructor
    // Punto 4: parámetro `salto` como size_t para evitar conversión implícita
    // Punto 14: reserve() en sample para evitar realocaciones
    // ─────────────────────────────────────────────────────────────────────────
    PForDelta(const std::vector<uint64_t>& A, size_t salto) {
        salto_sample = salto;
        if (A.empty()) return;

        total_gaps = A.size();

        // Punto 14: reservamos para evitar realocaciones
        size_t num_muestras = (total_gaps + salto_sample - 1) / salto_sample;
        sample.reserve(num_muestras);

        // Construimos gaps y sample en un solo recorrido
        std::vector<uint64_t> gaps_temporales(total_gaps);
        for (size_t i = 0; i < total_gaps; ++i) {
            gaps_temporales[i] = (i == 0) ? A[0] : A[i] - A[i - 1];
            if (i % salto_sample == 0) sample.push_back(A[i]);
        }

        // Comprimimos bloque a bloque
        bloques.reserve((total_gaps + TAMANO_BLOQUE - 1) / TAMANO_BLOQUE);
        for (size_t i = 0; i < total_gaps; i += TAMANO_BLOQUE) {
            size_t fin = std::min(i + TAMANO_BLOQUE, total_gaps);
            std::vector<uint64_t> bloque_temporal(gaps_temporales.begin() + i,
                                                  gaps_temporales.begin() + fin);
            bloques.push_back(comprimirBloque(bloque_temporal));
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Búsqueda: binaria sobre sample → decodificación secuencial sobre bloques
    // ─────────────────────────────────────────────────────────────────────────
    int buscar(uint64_t valor) const {
        if (sample.empty() || valor < sample[0]) return -1;

        // Búsqueda binaria en sample
        int low = 0;
        int high = static_cast<int>(sample.size()) - 1;
        int idx_sample = 0;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (sample[mid] == valor) {
                return static_cast<int>(mid * salto_sample);
            } else if (sample[mid] < valor) {
                idx_sample = mid;
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        // Decodificación secuencial en el intervalo [L, R)
        size_t L = static_cast<size_t>(idx_sample) * salto_sample;
        size_t R = L + salto_sample;
        if (R > total_gaps) R = total_gaps;

        uint64_t valor_actual = sample[idx_sample];

        for (size_t i = L + 1; i < R; ++i) {
            valor_actual += decompress_gap(i);
            if (valor_actual == valor) return static_cast<int>(i);
            if (valor_actual > valor)  return -1;
        }
        return -1;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Espacio en bytes
    // Punto 10: contamos también el overhead de cada objeto vector interno
    // ─────────────────────────────────────────────────────────────────────────
    size_t obtenerEspacio() const {
        // Sample
        size_t memoria = sample.capacity() * sizeof(uint64_t) + sizeof(sample);

        // Vector de bloques (overhead del vector externo)
        memoria += sizeof(bloques) + bloques.capacity() * sizeof(pfd::BloquePForDelta);

        for (const auto& bloque : bloques) {
            // Overhead del vector datos + sus bytes reales
            memoria += sizeof(bloque.datos) + bloque.datos.capacity() * sizeof(uint8_t);
            // lookup_outlier y outliers[] son arreglos fijos, ya contados en sizeof(BloquePForDelta)
        }

        memoria += sizeof(total_gaps) + sizeof(salto_sample);
        return memoria;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Retorna los bits reales promedio por elemento (para el informe)
    // ─────────────────────────────────────────────────────────────────────────
    double bitsPromedioporElemento() const {
        if (bloques.empty()) return 0.0;
        double total_bits = 0.0;
        size_t total_elementos = 0;
        for (const auto& bloque : bloques) {
            total_bits     += bloque.num_elementos * bloque.bits_por_elemento;
            total_elementos += bloque.num_elementos;
        }
        return (total_elementos > 0) ? (total_bits / total_elementos) : 0.0;
    }
};

#endif // PFORDELTA_HPP