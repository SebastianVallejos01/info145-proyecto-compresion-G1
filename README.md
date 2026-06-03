# INFO145 - Proyecto Semestral: Técnicas de Representación y Compresión en Arreglos Ordenados //código

## Integrantes
//Vicente Paredes
//Marco Peralta
//Alonso Véliz
//Sebastián Vallejos

## Descripción

Este proyecto implementa y compara tres estrategias para representar un arreglo ordenado de gran magnitud:

- Caso 1: Representación explícita con búsqueda binaria estándar (línea base).
- Caso 2: Gap-Coding con índice de muestreo (Sample) para búsqueda eficiente.
- Caso 3: PForDelta — compresión de gaps por bloques de 128 elementos, empaquetando cada gap en el tipo de dato más pequeño que lo contenga (uint8_t, uint16_t o uint32_t). Los outliers (≈10% superior del bloque) se almacenan por separado con acceso O(1) mediante tabla de lookup.

-Nota sobre implementación a nivel de bits: Como simplificación permitida por el enunciado, los gaps se empaquetan en el tipo entero más pequeño que los contenga en lugar de un empaquetado bit a bit estricto. El análisis teórico del número real de bits por elemento se realiza igualmente de forma rigurosa y está disponible mediante el método bitsPromedioporElemento() de la clase PForDelta.

--
## Requisitos
- Compilador g++ con soporte para C++17 o superior.
- Sistema operativo Linux o macOS (o WSL en Windows).
- make instalado.

---

## Estructura del Repositorio

```
.
├── main.cpp          # Punto de entrada del programa
├── caso1.hpp         # Caso 1: Representación explícita
├── caso2.hpp         # Caso 2: Gap-Coding + Sample
├── pfordelta.hpp     # Caso 3: PForDelta
├── generador.hpp     # Generación de datos (distribución lineal y normal)
├── Makefile          # Makefile de compilación
└── README.md         
```

---

## Compilación

```bash
make
```

Esto genera el ejecutable main en el directorio actual usando las flags `-O3 -std=c++17 -Wall`.

Para limpiar los archivos compilados:

```bash

make clean

```

---

## Modos de Ejecución

### Modo Benchmark

Genera automáticamente los arreglos de prueba, construye las tres estructuras y mide tiempos y espacio para tamaños n = 1.000.000,n = 10.000.000 y 'n=100.000.000', con distribuciones lineal y normal.

```bash
./main --benchmark
```

Salida: archivo metricas.csv en el directorio actual con las columnas:

```
Tamano, Distribucion, Caso, TiempoConstruccion_ms, TiempoBusquedaPromedio_us, Espacio_bytes
```

---

### Modo Archivo

Recibe un archivo `.csv` con números enteros, construye las tres estructuras y permite buscar valores de forma interactiva.

```bash
./main -i ruta/del/archivo.csv
```

Formato del archivo CSV: una columna de enteros positivos por línea o separados por coma.

```
```

El programa mostrará un menú para elegir en qué estructura buscar y reportará la posición encontrada y el tiempo de búsqueda en microsegundos.
---

## Rango de Valores Aceptados

El programa trabaja con enteros sin signo de 64 bits (`uint64_t`), usando `atoll` para la conversión desde texto.

- **Rango válido:** `0` a `18.446.744.073.709.551.615` (2⁶⁴ − 1)
- Los valores del archivo **deben estar ordenados de menor a mayor**.
- Valores negativos en el archivo serán convertidos a su equivalente `uint64_t` (comportamiento no definido — evitar).

---

## Archivos de Salida

| Modo       | Archivo generado | Contenido                                              |
|------------|------------------|--------------------------------------------------------|
| benchmark  | `metricas.csv`   | Métricas de tiempo y espacio por caso y distribución   |
| archivo    | _(ninguno)_      | Resultados impresos en consola                         |

---

## Notas de Implementación

- Se recomienda ejecutar con `valgrind` para verificar ausencia de fugas de memoria:
  ```bash
  valgrind --leak-check=full ./main --benchmark
  ```
- El salto del sample (`b`) está fijado en `64` tanto para Caso 2 como Caso 3. Puede modificarse en `main.cpp` en la variable `salto_b`.
- El tamaño de bloque de PForDelta está fijado en `128` elementos (`TAMANO_BLOQUE` en `pfordelta.hpp`).
