# AutoDock Vina - Modernización Controlada

## Resumen

Este documento describe la modernización controlada de AutoDock Vina, realizada con el objetivo de:
- Actualizar el código a C++14 de forma conservadora
- Mejorar la información de salida sin alterar resultados
- Agregar logging estructurado opcional
- Aplicar optimizaciones seguras que no afectan la lógica científica

**IMPORTANTE**: Todos los cambios mantienen la equivalencia numérica y no alteran el algoritmo de docking, scoring function ni criterios de convergencia.

## Cambios Implementados

### 1. Modernización del Sistema de Compilación

#### Actualización a C++14
- **Archivo modificado**: `build/makefile_common`
- **Cambio**: Reemplazado `-ansi` (C++98) por `-std=c++14`
- **Justificación**: Permite usar características modernas de C++ de forma segura y es compatible con Boost moderno

```makefile
# Antes
CC = ${GPP} ${C_PLATFORM} -ansi -Wno-long-long ${C_OPTIONS} $(INCFLAGS)

# Después
CC = ${GPP} ${C_PLATFORM} -std=c++14 -Wno-long-long ${C_OPTIONS} $(INCFLAGS)
```

### 2. Reemplazo de APIs Deprecadas de Boost

#### Timers modernos con std::chrono
- **Archivos modificados**: `src/main/main.cpp`
- **Cambio**: Reemplazado `boost::timer` (deprecado) por `std::chrono::steady_clock`
- **Implementación**: Nueva clase `simple_timer`

```cpp
// Nueva implementación de timer
class simple_timer {
    std::chrono::steady_clock::time_point start_time;
public:
    simple_timer() : start_time(std::chrono::steady_clock::now()) {}
    double elapsed() const {
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;
        return diff.count();
    }
};
```

**Ventajas**:
- Elimina warnings de compilación
- Usa API estándar de C++14
- Mayor precisión en mediciones
- Sin dependencias adicionales

#### Progress Display
- **Archivo modificado**: `src/lib/parallel_progress.h`
- **Cambio**: Reemplazado `boost::progress_display` (deprecado) por implementación propia
- **Implementación**: Nueva clase `simple_progress_display`

### 3. Salida Verbose Mejorada (--verbosity)

#### Nuevo Flag: --verbosity N
- **Valores**: 0 (sin salida), 1 (normal), 2 (verbose, default), 3 (extra verbose)
- **Default**: 2 (comportamiento original sin cambios)

#### Información Adicional con --verbosity 3

Cuando se usa `--verbosity 3`, se muestra para cada modo de unión:

1. **Descomposición Energética Completa**
   ```
   Mode 1:
     Total affinity: -8.234 (kcal/mol)
     Energy contributions (before weighting):
       Gauss 1      : 0.12345
       Gauss 2      : -0.23456
       Repulsion    : 0.34567
       Hydrophobic  : -0.45678
       Hydrogen     : -0.56789
   ```

2. **Energía Intramolecular**
   ```
   Intramolecular energy: 2.345 (kcal/mol)
   ```

3. **Geometría del Ligando**
   ```
   Ligand center of mass: (15.234, 20.456, 25.678)
   Ligand max radius: 5.432 Angstrom
   ```

**Implementación**: Las mediciones geométricas se calculan usando las coordenadas de átomos pesados movibles ya disponibles en el modelo, sin modificar el algoritmo.

### 4. Sistema de Logging Estructurado (--detail_log)

#### Nuevo Flag: --detail_log FILE
- **Propósito**: Genera un log detallado con timestamps precisos
- **Formato**: `[YYYY-MM-DD HH:MM:SS.mmm] [CATEGORY] message`
- **Opcional**: Solo se activa si se especifica el flag

#### Categorías de Log

| Categoría | Descripción | Ejemplo |
|-----------|-------------|---------|
| `START` | Inicio de ejecución | `[2024-01-18 15:30:45.123] [START] AutoDock Vina detailed log started` |
| `DOCKING` | Inicio de docking | `[2024-01-18 15:30:45.234] [DOCKING] Starting docking with random seed: 12345` |
| `PARAMS` | Parámetros de búsqueda | `[2024-01-18 15:30:45.345] [PARAMS] Search parameters - runs: 8, steps: 15750, threads: 4` |
| `REFINE` | Resultados de refinamiento | `[2024-01-18 15:32:10.567] [REFINE] Refinement completed. Found 9 unique conformations. Best energy: -8.234 kcal/mol` |
| `END` | Fin de ejecución | `[2024-01-18 15:32:11.678] [END] AutoDock Vina detailed log ended` |

#### Ejemplo de Log Generado

```
[2024-01-18 15:30:45.123] [START] AutoDock Vina detailed log started
[2024-01-18 15:30:45.234] [DOCKING] Starting docking with random seed: 12345
[2024-01-18 15:30:45.345] [PARAMS] Search parameters - runs: 8, steps: 15750, threads: 4
[2024-01-18 15:32:10.567] [REFINE] Refinement completed. Found 9 unique conformations. Best energy: -8.234 kcal/mol
[2024-01-18 15:32:11.678] [END] AutoDock Vina detailed log ended
```

**Implementación**: Sistema de logging no invasivo que solo escribe a disco si se solicita. Sin impacto en performance cuando no está activo.

### 5. Optimizaciones Seguras

#### Vector reserve()
Agregado `reserve()` en vectores donde el tamaño es conocido de antemano:

1. **Vector de weights** (6 elementos conocidos)
   ```cpp
   flv weights;
   weights.reserve(6); // Pre-allocate for 6 weights
   weights.push_back(weight_gauss1);
   // ... resto de pesos
   ```

2. **Vector de remarks** (num_modes conocido)
   ```cpp
   std::vector<std::string> remarks;
   remarks.reserve(num_modes); // Pre-allocate for expected modes
   ```

**Beneficios**:
- Reduce reallocaciones de memoria
- Mejora localidad de caché
- Sin cambios en comportamiento o resultados

## Uso

### Docking Estándar (Sin Cambios)
```bash
./vina --receptor protein.pdbqt \
       --ligand ligand.pdbqt \
       --center_x 15.0 --center_y 20.0 --center_z 25.0 \
       --size_x 20 --size_y 20 --size_z 20 \
       --out result.pdbqt
```

Comportamiento idéntico a la versión original (verbosity=2 por defecto).

### Con Salida Detallada de Energías
```bash
./vina --receptor protein.pdbqt \
       --ligand ligand.pdbqt \
       --center_x 15.0 --center_y 20.0 --center_z 25.0 \
       --size_x 20 --size_y 20 --size_z 20 \
       --out result.pdbqt \
       --verbosity 3
```

Muestra descomposición energética completa para cada modo.

### Con Logging Estructurado
```bash
./vina --receptor protein.pdbqt \
       --ligand ligand.pdbqt \
       --center_x 15.0 --center_y 20.0 --center_z 25.0 \
       --size_x 20 --size_y 20 --size_z 20 \
       --out result.pdbqt \
       --detail_log docking.log
```

Genera un log detallado con timestamps en `docking.log`.

### Combinación de Opciones
```bash
./vina --receptor protein.pdbqt \
       --ligand ligand.pdbqt \
       --center_x 15.0 --center_y 20.0 --center_z 25.0 \
       --size_x 20 --size_y 20 --size_z 20 \
       --out result.pdbqt \
       --verbosity 3 \
       --detail_log docking.log \
       --log output.log
```

Combina todas las opciones de salida.

## Validación

### Compilación
```bash
cd build/linux/release
make clean
make
```

Compila sin errores ni warnings con C++14.

### Verificación de Funcionamiento
```bash
./vina --version
# Output: AutoDock Vina 1.1.2 (May 11, 2011)

./vina --help
# Muestra todos los flags incluyendo --verbosity y --detail_log
```

### Equivalencia Numérica
Los cambios implementados:
- ✅ NO modifican el algoritmo de búsqueda
- ✅ NO modifican la scoring function
- ✅ NO modifican los criterios de convergencia
- ✅ NO alteran el orden de operaciones críticas
- ✅ Solo agregan información de salida adicional
- ✅ Solo aplican optimizaciones de pre-allocación (reserve)

**Resultado esperado**: Mismos valores de afinidad y RMSD (± tolerancia de punto flotante) comparado con la versión original.

## Restricciones Respetadas

Durante la modernización se respetaron estrictamente las siguientes restricciones:

### ✅ No Cambiar
- Algoritmo de búsqueda (Monte Carlo)
- Scoring function (términos y pesos)
- Orden de operaciones críticas
- Criterios de convergencia
- Arquitectura general del código

### ✅ No Hacer
- Refactorización profunda
- Introducción de dependencias pesadas
- Paralelización adicional
- Alteración de resultados más allá del ruido numérico

### ✅ Solo Permitido
- Cambios locales y conservadores
- Optimizaciones determinísticas
- Mejoras en información de salida
- Uso de características C++14 seguras

## Archivos Modificados

```
build/makefile_common           - Sistema de compilación (C++14)
src/main/main.cpp              - Timer, logging, verbose output, optimizaciones
src/lib/parallel_progress.h    - Progress display moderno
```

Total de archivos modificados: **3**

## Beneficios de la Modernización

1. **Compatibilidad**: Compila con compiladores y librerías modernas
2. **Mantenibilidad**: Usa APIs estándar de C++14 en lugar de APIs deprecadas
3. **Información**: Más detalles sobre el proceso de docking
4. **Debugging**: Logs estructurados con timestamps precisos
5. **Performance**: Optimizaciones seguras (reserve)
6. **Retrocompatibilidad**: Comportamiento por defecto sin cambios

## Próximos Pasos Sugeridos

Las siguientes mejoras son opcionales y podrían agregarse en el futuro:

1. **Propiedades Fisicoquímicas** (--calc-properties)
   - Peso molecular
   - Número de HBD/HBA
   - Enlaces rotables
   - LogP aproximado

2. **Métricas Geométricas Adicionales** (--report-geometry)
   - Distancias ligando-receptor detalladas
   - Número de contactos por rango de distancia
   - Análisis de interacciones

3. **Documentación Extendida**
   - Ejemplos de uso avanzado
   - Guías de interpretación de energías
   - Tutorial de análisis de logs

## Contacto y Soporte

Para reportar problemas o sugerencias relacionadas con estas mejoras:
- Crear un issue en el repositorio de GitHub
- Incluir versión del compilador y sistema operativo
- Adjuntar ejemplo mínimo reproducible si aplica

## Referencias

- AutoDock Vina original: http://vina.scripps.edu
- Documentación C++14: https://en.cppreference.com/w/cpp/14
- Boost.Chrono: https://www.boost.org/doc/libs/release/doc/html/chrono.html

---

**Última actualización**: 2024-01-18  
**Versión de AutoDock Vina**: 1.1.2 (base)  
**Estándar C++**: C++14
