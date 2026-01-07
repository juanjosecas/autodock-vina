# Enhanced Execution Information for AutoDock Vina

## Summary of Changes

This document describes the improvements made to the AutoDock Vina code to provide more information about what is happening during program execution.

## Implemented Changes

### 1. Execution Time Information

A new `done_with_time()` function has been added that displays elapsed time for important operations:

- **Setting up the scoring function** - Shows how long it takes to initialize the scoring system
- **Analyzing the binding site** - Time for energy cache generation
- **Search** - Complete duration of the Monte Carlo search phase
- **Refining results** - Time needed to refine the found results

**Example output:**
```
Setting up the scoring function ... done (elapsed time: 0.234s).
Analyzing the binding site ... done (elapsed time: 2.456s).
Performing search ... done (elapsed time: 45.123s).
Refining results ... done (elapsed time: 12.345s).
```

### 2. Search Space Information

When verbosity is > 1 (default = 2), the program now shows details about the search space:

- **Center** of the search volume (X, Y, Z coordinates)
- **Dimensions** of the volume (in Angstroms)
- **Total volume** of the search space (in Angstroms³)

**Example output:**
```
Search space:
  Center: (15.000, 20.000, 25.000)
  Size: (30.000 x 30.000 x 30.000) Angstrom
  Volume: 27000.0 Angstrom^3
```

### 3. Ligand Information

Detailed information about the ligand being docked is provided:

- **Movable atoms** - Number of atoms that can move during docking
- **Degrees of freedom** - Total number of degrees of freedom for optimization
- **Computed heuristic** - Value used to determine search steps

**Example output:**
```
Ligand information:
  Movable atoms: 45
  Degrees of freedom: 12
  Computed heuristic: 165
```

### 4. Input Files Information

When reading input files, the program now shows which files are being used:

- **Receptor** - PDBQT file of the rigid receptor
- **Flexible residues** - PDBQT file of flexible side chains (if applicable)
- **Ligand** - PDBQT file of the ligand

**Example output:**
```
Input information:
  Receptor: receptor.pdbqt
  Flexible residues: flex.pdbqt
  Ligand: ligand.pdbqt
```

### 5. Search Parameters

During the search phase, the parameters being used are displayed:

- **Number of runs** (exhaustiveness)
- **Steps per run** - Number of Monte Carlo steps per run
- **Number of threads** - CPUs used for parallel search

**Example output:**
```
Search parameters:
  Number of runs: 8
  Steps per run: 5250
  Number of threads: 4
```

### 6. Intermediate Results

Information about results found during the process is provided:

- **Initial results** - Number of conformations found during search
- **Best energy found** - The lowest energy detected
- **Unique conformations** - Number of conformations after removing duplicates

**Example output:**
```
Search produced 160 initial results
Best energy found: -8.456 (kcal/mol)
After refinement, 12 unique conformations
```

### 7. Improvements in Randomize Mode

In randomization mode (--randomize_only), the following was added:

- Explanatory message of the process
- Optionally (verbosity > 2): Progress of clash minimization attempts

**Example output:**
```
Attempting to find low-clash initial conformation...
  Attempt 234/10000: clash penalty = 1.234
Best clash penalty found: 0.567
```

## Usage

All these improvements are controlled by the verbosity level:

- **verbosity = 0**: No output (errors only)
- **verbosity = 1**: Minimal output
- **verbosity = 2**: Normal output with all improvements (DEFAULT)
- **verbosity > 2**: Additional debug information

No changes to command line arguments are required. The default verbosity level is 2, which already includes all additional information.

## Benefits

1. **Better process understanding**: Users can see exactly what the program is doing at each moment
2. **Performance diagnostics**: Execution times help identify bottlenecks
3. **Parameter validation**: Search space and ligand information allows verification that the configuration is correct
4. **Transparency**: Greater visibility of the docking process for educational and research purposes

## Compatibility

These changes are fully backward compatible:
- No modification of command line parameters
- Existing output remains intact
- Only adds additional information when verbosity > 1

## Modified Files

- `src/main/main.cpp`: Added functions and informative messages
- `.gitignore`: Added entries to exclude build files

## Note on Compilation

This code was originally designed for Boost 1.46. To compile with modern Boost versions (1.83+), adjustments in quaternion handling are required that are outside the scope of these informational changes.
