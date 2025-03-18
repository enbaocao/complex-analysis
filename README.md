# Complex Analysis Visualizations

Interactive visualizations for complex analysis concepts using raylib and C. These visualizations were created to support learning and teaching complex analysis.

## Current Visualizations

### Domain Coloring
Located in `/coloring/` - Visualizes complex-valued functions using domain coloring techniques. Maps the phase (argument) to color hue and magnitude to brightness.

Features:
- Multiple complex functions (exp, sin, tan, reciprocal, polynomial)
- Phase and modulus lines
- Adjustable contrast and saturation
- Anti-aliasing options
- Pan and zoom interface

### Conformal Mappings
Located in `/conformal/` - Demonstrates how different complex functions transform regular grids and patterns.

Features:
- Various input graph types (rectangular grid, concentric circles, radial lines, polar grid)
- Multiple mapping functions (identity, square, reciprocal, exponential, Möbius)
- Animated transitions between domains
- Mouse tracking to visualize individual point mappings

### Bilinear Transformations
Located in `/bilinear/` - Explores specific conformal maps in the form (az+b)/(cz+d).

Features:
- Visualization of circle and line preserving properties
- Mapping the unit disk to the upper half-plane
- Interactive input type selection

## Suggested Additional Visualizations

### Complex Series Visualization
Visualize how Taylor and Laurent series approximate complex functions with increasing terms.

### Riemann Surfaces
Represent multi-valued functions like square root or logarithm as Riemann surfaces.

### Complex Integration
Visualize contour integration and the residue theorem with interactive paths.

### Complex Dynamical Systems
Explore Julia sets, the Mandelbrot set, and basins of attraction for Newton's method.

### Branch Cuts
Interactive visualization of branch cuts and Riemann sheets.

## Building & Running

### Prerequisites
- A C compiler (gcc/clang)
- [raylib](https://www.raylib.com/) installed on your system

### Compilation
In each project directory:

```bash
gcc main.c -o visualization -lraylib -lm
```

### Running
```bash
./visualization
```

## Controls

Controls vary between visualizations but are displayed in the application window. Common controls include:
- Mouse drag to pan
- Mouse wheel to zoom
- Arrow keys to change functions or input types
- Space to toggle animations

## License
MIT License - See LICENSE file for details.
