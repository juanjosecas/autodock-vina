# Mejoras en la Información de Ejecución de AutoDock Vina

## Resumen de Cambios

Este documento describe las mejoras realizadas al código de AutoDock Vina para proporcionar más información sobre lo que está sucediendo durante la ejecución del programa.

## Cambios Implementados

### 1. Información de Tiempo de Ejecución

Se ha añadido una nueva función `done_with_time()` que muestra el tiempo transcurrido para operaciones importantes:

- **Configuración de la función de puntuación** - Muestra cuánto tiempo toma inicializar el sistema de puntuación
- **Análisis del sitio de unión** - Tiempo de generación de la caché de energías
- **Búsqueda** - Duración completa de la fase de búsqueda Monte Carlo
- **Refinamiento** - Tiempo necesario para refinar los resultados encontrados

**Ejemplo de salida:**
```
Setting up the scoring function ... done (elapsed time: 0.234s).
Analyzing the binding site ... done (elapsed time: 2.456s).
Performing search ... done (elapsed time: 45.123s).
Refining results ... done (elapsed time: 12.345s).
```

### 2. Información del Espacio de Búsqueda

Cuando la verbosidad es > 1 (default = 2), el programa ahora muestra detalles sobre el espacio de búsqueda:

- **Centro** del volumen de búsqueda (coordenadas X, Y, Z)
- **Dimensiones** del volumen (en Angstroms)
- **Volumen total** del espacio de búsqueda (en Angstroms³)

**Ejemplo de salida:**
```
Search space:
  Center: (15.000, 20.000, 25.000)
  Size: (30.000 x 30.000 x 30.000) Angstrom^3
  Volume: 27000.0 Angstrom^3
```

### 3. Información del Ligando

Se proporciona información detallada sobre el ligando que se está dockeando:

- **Átomos móviles** - Número de átomos que pueden moverse durante el docking
- **Grados de libertad** - Número total de grados de libertad para la optimización
- **Heurística calculada** - Valor utilizado para determinar los pasos de búsqueda

**Ejemplo de salida:**
```
Ligand information:
  Movable atoms: 45
  Degrees of freedom: 12
  Computed heuristic: 165
```

### 4. Información de Archivos de Entrada

Al leer los archivos de entrada, ahora se muestra qué archivos se están utilizando:

- **Receptor** - Archivo PDBQT del receptor rígido
- **Residuos flexibles** - Archivo PDBQT de las cadenas laterales flexibles (si aplica)
- **Ligando** - Archivo PDBQT del ligando

**Ejemplo de salida:**
```
Input information:
  Receptor: receptor.pdbqt
  Flexible residues: flex.pdbqt
  Ligand: ligand.pdbqt
```

### 5. Parámetros de Búsqueda

Durante la fase de búsqueda, se muestran los parámetros utilizados:

- **Número de ejecuciones** (exhaustiveness)
- **Pasos por ejecución** - Número de pasos Monte Carlo por run
- **Número de hilos** - CPUs utilizados para la búsqueda paralela

**Ejemplo de salida:**
```
Search parameters:
  Number of runs: 8
  Steps per run: 5250
  Number of threads: 4
```

### 6. Resultados Intermedios

Se proporciona información sobre los resultados encontrados durante el proceso:

- **Resultados iniciales** - Número de conformaciones encontradas durante la búsqueda
- **Mejor energía encontrada** - La energía más baja detectada
- **Conformaciones únicas** - Número de conformaciones después de eliminar duplicados

**Ejemplo de salida:**
```
Search produced 160 initial results
Best energy found: -8.456 (kcal/mol)
After refinement, 12 unique conformations
```

### 7. Mejoras en el Modo Randomize

En el modo de randomización (--randomize_only), se añadió:

- Mensaje explicativo del proceso
- Opcionalmente (verbosity > 2): Progreso de los intentos de minimización de choques

**Ejemplo de salida:**
```
Attempting to find low-clash initial conformation...
  Attempt 234/10000: clash penalty = 1.234
Best clash penalty found: 0.567
```

## Uso

Todas estas mejoras están controladas por el nivel de verbosidad:

- **verbosity = 0**: Sin salida (solo errores)
- **verbosity = 1**: Salida mínima
- **verbosity = 2**: Salida normal con todas las mejoras (DEFAULT)
- **verbosity > 2**: Información adicional de debug

No se requieren cambios en la línea de comandos. El nivel de verbosidad por defecto es 2, que ya incluye toda la información adicional.

## Beneficios

1. **Mejor comprensión del proceso**: Los usuarios pueden ver exactamente qué está haciendo el programa en cada momento
2. **Diagnóstico de rendimiento**: Los tiempos de ejecución ayudan a identificar cuellos de botella
3. **Validación de parámetros**: La información del espacio de búsqueda y ligando permite verificar que la configuración es correcta
4. **Transparencia**: Mayor visibilidad del proceso de docking para fines educativos y de investigación

## Compatibilidad

Estos cambios son totalmente compatibles con versiones anteriores:
- No se modifican parámetros de línea de comandos
- La salida existente se mantiene intacta
- Solo se añade información adicional cuando verbosity > 1

## Archivos Modificados

- `src/main/main.cpp`: Añadidas funciones y mensajes informativos
- `.gitignore`: Añadidas entradas para excluir archivos de compilación

## Nota sobre Compilación

Este código fue diseñado originalmente para Boost 1.46. Para compilar con versiones modernas de Boost (1.83+), se requieren ajustes en el manejo de quaternions que están fuera del alcance de estos cambios informativos.
