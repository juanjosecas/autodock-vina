# Guía de Uso - Mejoras a la Función de Puntuación

## Resumen Ejecutivo

Se han agregado cuatro nuevos términos de puntuación a AutoDock Vina basados en investigación científica moderna (2010-2024):

1. **Enlaces de Halógeno** - Para compuestos con Cl, Br, I
2. **Apilamiento Pi** - Para interacciones aromático-aromático
3. **Interacciones S-π** - Para azufre-aromático
4. **Desolvatación Mejorada** - Mejor modelado del efecto hidrofóbico

**Estado:** Todos los términos están **deshabilitados por defecto** para mantener compatibilidad.

## ¿Cuándo Usar Estos Términos?

### Enlaces de Halógeno
**Usar para:**
- Ligandos que contienen Cl, Br, o I
- Receptores con residuos ricos en O/N en el sitio de unión
- Validación experimental muestra interacciones halógeno-oxígeno/nitrógeno

**Ejemplos:**
- Inhibidores de tirosina quinasa con Br/Cl
- Compuestos farmacéuticos halogenados
- Ligandos con grupos CF3, CCl3

### Apilamiento Pi
**Usar para:**
- Ligandos con múltiples anillos aromáticos
- Sitios de unión con residuos aromáticos (Phe, Tyr, Trp)
- Intercaladores de ADN
- Inhibidores que se unen a bolsillos hidrofóbicos aromáticos

**Ejemplos:**
- Inhibidores de quinasas
- Ligandos tipo indol, quinolina, naftaleno
- Moléculas planares aromáticas

### Interacciones S-π
**Usar para:**
- Sitios de unión con metioninas
- Ligandos aromáticos que interactúan con residuos de azufre
- Proteínas ricas en cisteína/metionina

**Ejemplos:**
- Proteínas con Met en el sitio activo
- Ligandos que contienen tiofeno, tiazol
- Complejos donde cristalografía muestra Met-aromático

### Desolvatación Mejorada
**Usar para:**
- Ligandos con mezcla de grupos polares e hidrofóbicos
- Predicción más precisa de efectos de entropía
- Casos donde Vina subestima/sobreestima afinidad

**Ejemplos:**
- Ligandos anfifílicos
- Moléculas con muchos grupos OH, NH2
- Péptidos y peptidomiméticos

## Cómo Habilitar los Términos

### Paso 1: Editar el Código Fuente

Abrir `src/lib/everything.cpp` y buscar la sección de términos modernos (líneas ~474-490):

```cpp
// Modern scoring terms (2010-2024) - initially disabled for testing
```

### Paso 2: Cambiar d a 1

Cambiar el primer parámetro de `d` a `1` para el término deseado:

**Antes (deshabilitado):**
```cpp
add(d, new halogen_bond(-0.5, 0.5, cutoff));
```

**Después (habilitado):**
```cpp
add(1, new halogen_bond(-0.5, 0.5, cutoff));
```

### Paso 3: Actualizar Pesos

Los pesos iniciales son estimaciones. Deben agregarse a `src/lib/current_weights.cpp`:

**Archivo actual:** 6 pesos
```cpp
const fl a[] = {-0.035579, -0.005156, 0.840245, -0.035069, -0.587439, 1.923};
```

**Si habilitas 1 término:** 7 pesos (agregar peso al final)
```cpp
const fl a[] = {-0.035579, -0.005156, 0.840245, -0.035069, -0.587439, 1.923, -0.3};
//                                                                            ^nuevo
```

**Si habilitas los 4 términos:** 10 pesos
```cpp
const fl a[] = {-0.035579, -0.005156, 0.840245, -0.035069, -0.587439, 1.923, 
                -0.3, -0.2, -0.15, 0.01};
//              ^halogen ^pi  ^S-π  ^desolv
```

### Paso 4: Recompilar

```bash
cd build/linux/release
make clean
make
```

## Pesos Iniciales Recomendados

Estos son valores conservadores basados en la literatura. **Deben ser optimizados** para tu conjunto de datos específico.

| Término | Peso Inicial | Justificación |
|---------|--------------|---------------|
| halogen_bond | -0.3 | Similar a H-bond pero más débil |
| pi_stacking | -0.2 | Interacción moderadamente favorable |
| sulfur_aromatic | -0.15 | Interacción débil pero significativa |
| desolvation_improved | 0.01 | Pequeña corrección a desolvatación existente |

## Validación y Optimización

### Antes de Usar en Producción

1. **Validación Básica:**
   ```bash
   # Probar con casos conocidos
   vina --receptor protein.pdbqt --ligand ligand.pdbqt --config config.txt
   
   # Comparar con resultados originales
   # Verificar que las energías son razonables
   ```

2. **Benchmarking:**
   - Usar conjunto de validación (ej: PDBbind core set)
   - Calcular correlación con datos experimentales
   - Comparar con Vina original

3. **Optimización de Pesos:**
   - Usar conjunto de entrenamiento diverso (>100 complejos)
   - Optimización por gradiente descendente
   - Validación cruzada
   - Evitar sobreajuste

### Script de Validación Ejemplo

```python
import subprocess
import pandas as pd
from scipy.stats import pearsonr

# Cargar datos experimentales
data = pd.read_csv('experimental_affinities.csv')

predictions = []
for idx, row in data.iterrows():
    # Ejecutar Vina
    cmd = f"vina --receptor {row['receptor']} --ligand {row['ligand']} ..."
    result = subprocess.run(cmd, shell=True, capture_output=True)
    # Parsear resultado
    predicted_affinity = parse_vina_output(result.stdout)
    predictions.append(predicted_affinity)

# Calcular correlación
r, p = pearsonr(data['experimental'], predictions)
print(f"Correlación: {r:.3f}, p-value: {p:.3e}")
```

## Casos de Prueba

### Caso 1: Halogen Bonding
- **Sistema:** PDB ID 3OG7 (tirosina quinasa con ligando bromado)
- **Expectativa:** Mejor puntuación con término de halógeno habilitado
- **Verificación:** Distancia Br...O debe ser ~3.3 Å

### Caso 2: Pi-Stacking
- **Sistema:** PDB ID 1OYT (inhibidor de CDK2 con anillos aromáticos)
- **Expectativa:** Interacciones π-π entre ligando y Phe80
- **Verificación:** Distancia centroide-centroide ~3.6 Å

### Caso 3: S-π
- **Sistema:** Complejos con Met en sitio activo
- **Expectativa:** Mejor modelado de interacciones Met-aromático
- **Verificación:** Geometría Met-S...centroide aromático

## Troubleshooting

### Problema: Error de Compilación
```
error: too many weights
```
**Solución:** Asegurarse que el número de pesos en `current_weights.cpp` coincide con el número de términos habilitados.

### Problema: Energías Inesperadas
**Solución:** 
1. Verificar que los pesos son apropiados (no demasiado grandes/pequeños)
2. Revisar geometría del ligando (clashes, conformaciones extrañas)
3. Comparar con Vina original para ver diferencia

### Problema: Sin Mejora en Correlación
**Solución:**
1. El término puede no ser relevante para tu sistema
2. Pesos pueden necesitar optimización
3. Conjunto de prueba puede ser pequeño (necesita >50 complejos)

## Referencias Rápidas

- **Documentación completa:** `SCORING_IMPROVEMENTS.md` (inglés) o `MEJORAS_PUNTUACION.md` (español)
- **Código fuente:** 
  - Términos: `src/lib/everything.cpp`
  - Pesos: `src/lib/current_weights.cpp`
  - Helpers: `src/lib/atom_constants.h`

## Soporte

Para preguntas:
1. Revisar documentación completa
2. Consultar referencias científicas citadas
3. Contactar equipo de desarrollo de AutoDock Vina

## Licencia

Apache 2.0 - igual que AutoDock Vina original

---

**Versión:** 1.0
**Fecha:** 2026-01-07
