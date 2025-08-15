# complex analysis visualizations

hands-on, realtime visuals for core ideas in complex analysis. built in c with raylib. welcoming to poke, tweak, and learn from.

## gallery

<p align="center">
  <img src="images/circle-to-half-plane.png" width="360" alt="circle to half-plane"/><br/>
  <em>bilinear: circle-to-half-plane</em>
</p>

<p align="center">
  <img src="images/exponential-taylor-approx.png" width="360" alt="exponential taylor approximation"/><br/>
  <em>series: exponential taylor approx</em>
</p>

<p align="center">
  <img src="images/fifth-power-z-minus-z-coloring.png" width="360" alt="fifth power z minus z domain coloring"/><br/>
  <em>coloring: fifth power z minus z</em>
</p>

<p align="center">
  <img src="images/log-taylor-approx.png" width="360" alt="log taylor approximation"/><br/>
  <em>series: log taylor approx</em>
</p>

<p align="center">
  <img src="images/radial-lines-reciprocal-mapping.png" width="360" alt="radial lines reciprocal mapping"/><br/>
  <em>conformal: radial lines, reciprocal</em>
</p>

## current visualizations

### domain coloring
located in `coloring/`. domain coloring for complex-valued functions: hue = phase, brightness = magnitude.

### conformal mappings
located in `conformal/`. watch grids morph under mappings.

### bilinear transformations
located in `bilinear/`. möbius maps: (az+b)/(cz+d).

## future additions
complex series visualization (taylor/laurent approximations), riemann surfaces (multi‑valued sqrt/log), complex integration (contours, residues), complex dynamical systems (julia/mandelbrot/newton basins), branch cuts (riemann sheets).

## build & run

### prerequisites
- a c compiler (clang/gcc)
- raylib and pkg-config

macos (homebrew):
```bash
brew install raylib pkg-config
```

### compile (repo root)
build all apps:

```bash
mkdir -p bin
for d in bilinear coloring conformal series; do \
  cc "$d/main.c" -std=c11 -o "bin/$d" $(pkg-config --cflags --libs raylib); \
done
```

### run
```bash
./bin/bilinear
./bin/coloring
./bin/conformal
./bin/series
```

### recreate the gallery shots
- bilinear → input: unit circle, transform: circle to half-plane
- series → function: exp, split or error view; increase terms
- coloring → function: z^5 - z (cycle with ←/→)
- series → function: log, taylor/laurent view
- conformal → input: radial lines, mapping: reciprocal

 
