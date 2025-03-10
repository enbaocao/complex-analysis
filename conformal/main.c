#include "raylib.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Define a complex number structure for calculations
typedef struct {
    float real;
    float imag;
} Complex;

// Complex number operations
Complex complex_create(float real, float imag) {
    Complex c = {real, imag};
    return c;
}

Complex complex_multiply(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

Complex complex_divide(Complex a, Complex b) {
    Complex result;
    float denominator = b.real * b.real + b.imag * b.imag;
    result.real = (a.real * b.real + a.imag * b.imag) / denominator;
    result.imag = (a.imag * b.real - a.real * b.imag) / denominator;
    return result;
}

Complex complex_add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

// Linear interpolation between two complex numbers
Complex complex_lerp(Complex a, Complex b, float t) {
    Complex result;
    result.real = a.real + t * (b.real - a.real);
    result.imag = a.imag + t * (b.imag - a.imag);
    return result;
}

float complex_abs(Complex c) {
    return sqrtf(c.real * c.real + c.imag * c.imag);
}

// Conformal mappings
Complex identity_mapping(Complex z) {
    return z;
}

Complex square_mapping(Complex z) {
    return complex_multiply(z, z);
}

Complex reciprocal_mapping(Complex z) {
    Complex one = {1.0f, 0.0f};
    return complex_divide(one, z);
}

Complex exp_mapping(Complex z) {
    Complex result;
    float exp_real = expf(z.real);
    result.real = exp_real * cosf(z.imag);
    result.imag = exp_real * sinf(z.imag);
    return result;
}

Complex mobius_mapping(Complex z) {
    Complex a = {1.0f, 0.0f};
    Complex b = {0.0f, 1.0f};
    Complex c = {0.0f, 0.0f};
    Complex d = {1.0f, 0.0f};
    
    Complex numerator = complex_add(complex_multiply(a, z), b);
    Complex denominator = complex_add(complex_multiply(c, z), d);
    
    return complex_divide(numerator, denominator);
}

// Coordinate conversions
Vector2 complex_to_screen(Complex z, Vector2 center, float scale) {
    Vector2 screen;
    screen.x = center.x + z.real * scale;
    screen.y = center.y - z.imag * scale;
    return screen;
}

Complex screen_to_complex(Vector2 screen, Vector2 center, float scale) {
    Complex z;
    z.real = (screen.x - center.x) / scale;
    z.imag = (center.y - screen.y) / scale;
    return z;
}

void draw_complex_circle(Complex center, float radius, Color color, Vector2 screen_center, float scale) {
    Vector2 screen_pos = complex_to_screen(center, screen_center, scale);
    DrawCircle(screen_pos.x, screen_pos.y, radius * scale, color);
}

// Input graph types
typedef enum {
    GRID_PATTERN,
    CONCENTRIC_CIRCLES,
    RADIAL_LINES,
    POLAR_GRID
} InputGraphType;

// Structure to hold a point with its mapped value
typedef struct {
    Complex z;       // Source point
    Complex w;       // Mapped point
} MappedPoint;

// Generate points for different graph types
void generate_grid_points(MappedPoint* points, int* count, float spacing, int size, Complex (*mapping)(Complex)) {
    *count = 0;
    
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {
            Complex z = complex_create(i * spacing, j * spacing);
            
            // Skip origin for reciprocal mapping to avoid division by zero
            if (mapping == reciprocal_mapping && i == 0 && j == 0) continue;
            
            Complex w = mapping(z);
            
            // Check if mapped point is within reasonable bounds
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                (*count)++;
            }
        }
    }
}

void generate_concentric_circles(MappedPoint* points, int* count, int num_circles, int points_per_circle, Complex (*mapping)(Complex)) {
    *count = 0;
    const float radius_step = 0.5f;
    
    // Add center point (except for reciprocal mapping)
    if (mapping != reciprocal_mapping) {
        Complex z = complex_create(0.0f, 0.0f);
        Complex w = mapping(z);
        if (complex_abs(w) < 100.0f) {
            points[*count].z = z;
            points[*count].w = w;
            (*count)++;
        }
    }
    
    // Generate points on concentric circles
    for (int c = 1; c <= num_circles; c++) {
        float radius = c * radius_step;
        
        for (int p = 0; p < points_per_circle; p++) {
            float angle = p * (2.0f * PI / points_per_circle);
            Complex z = complex_create(radius * cosf(angle), radius * sinf(angle));
            Complex w = mapping(z);
            
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                (*count)++;
            }
        }
    }
}

void generate_radial_lines(MappedPoint* points, int* count, int num_lines, int points_per_line, float max_radius, Complex (*mapping)(Complex)) {
    *count = 0;
    
    // Generate points on radial lines
    for (int l = 0; l < num_lines; l++) {
        float angle = l * (2.0f * PI / num_lines);
        float dx = cosf(angle);
        float dy = sinf(angle);
        
        for (int p = 1; p <= points_per_line; p++) {  // Start from 1 to avoid origin for reciprocal
            float radius = p * (max_radius / points_per_line);
            Complex z = complex_create(radius * dx, radius * dy);
            Complex w = mapping(z);
            
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                (*count)++;
            }
        }
    }
    
    // Add center point (except for reciprocal mapping)
    if (mapping != reciprocal_mapping) {
        Complex z = complex_create(0.0f, 0.0f);
        Complex w = mapping(z);
        if (complex_abs(w) < 100.0f) {
            points[*count].z = z;
            points[*count].w = w;
            (*count)++;
        }
    }
}

void generate_polar_grid(MappedPoint* points, int* count, int num_circles, int num_lines, Complex (*mapping)(Complex)) {
    *count = 0;
    const float radius_step = 0.5f;
    
    // Add center point (except for reciprocal mapping)
    if (mapping != reciprocal_mapping) {
        Complex z = complex_create(0.0f, 0.0f);
        Complex w = mapping(z);
        if (complex_abs(w) < 100.0f) {
            points[*count].z = z;
            points[*count].w = w;
            (*count)++;
        }
    }
    
    // Generate points on concentric circles
    for (int c = 1; c <= num_circles; c++) {
        float radius = c * radius_step;
        
        for (int l = 0; l < num_lines; l++) {
            float angle = l * (2.0f * PI / num_lines);
            Complex z = complex_create(radius * cosf(angle), radius * sinf(angle));
            Complex w = mapping(z);
            
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                (*count)++;
            }
        }
    }
}

int main(void) {
    // Initialize window
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "animated conformal mapping");
    SetTargetFPS(60);
    
    // View parameters
    Vector2 center = {screenWidth / 2, screenHeight / 2};
    float scale = 100.0f;
    
    // Animation parameters
    float animation_time = 0.0f;
    float animation_speed = 0.01f;  // adjust for speed
    bool animate = false;
    
    // Maximum number of points to render
    const int MAX_POINTS = 2000;
    MappedPoint* points = (MappedPoint*)malloc(MAX_POINTS * sizeof(MappedPoint));
    int point_count = 0;
    
    // Input graph parameters
    InputGraphType current_graph = GRID_PATTERN;
    const int graph_count = 4;
    const char* graph_names[graph_count] = {
        "rectangular grid",
        "concentric circles",
        "radial lines",
        "polar grid"
    };
    
    // Grid parameters
    const int gridSize = 10;
    const float gridSpacing = 0.5f;
    const float circleRadius = 0.1f;
    
    // Mapping selection
    int current_mapping = 0;
    const int mapping_count = 5;
    Complex (*mappings[mapping_count])(Complex) = {
        identity_mapping,
        square_mapping,
        reciprocal_mapping,
        exp_mapping,
        mobius_mapping
    };
    
    const char* mapping_names[mapping_count] = {
        "identity: f(z) = z",
        "square: f(z) = z^2",
        "reciprocal: f(z) = 1/z",
        "exponential: f(z) = e^z",
        "möbius: f(z) = (az+b)/(cz+d)"
    };
    
    // Generate initial points
    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update inputs
        
        // Handle input graph selection (left/right)
        if (IsKeyPressed(KEY_RIGHT)) {
            current_graph = (current_graph + 1) % graph_count;
            animation_time = 0.0f;  // Reset animation
            animate = true;
            
            // Generate new points based on selected graph
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, 10, 36, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, 24, 20, 5.0f, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, 10, 24, mappings[current_mapping]);
                    break;
            }
        }
        
        if (IsKeyPressed(KEY_LEFT)) {
            current_graph = (current_graph - 1 + graph_count) % graph_count;
            animation_time = 0.0f;  // Reset animation
            animate = true;
            
            // Generate new points based on selected graph
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, 10, 36, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, 24, 20, 5.0f, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, 10, 24, mappings[current_mapping]);
                    break;
            }
        }
        
        // Handle mapping selection (up/down)
        if (IsKeyPressed(KEY_DOWN)) {
            current_mapping = (current_mapping + 1) % mapping_count;
            animation_time = 0.0f;  // Reset animation
            animate = true;
            
            // Regenerate points with new mapping
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, 10, 36, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, 24, 20, 5.0f, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, 10, 24, mappings[current_mapping]);
                    break;
            }
        }
        
        if (IsKeyPressed(KEY_UP)) {
            current_mapping = (current_mapping - 1 + mapping_count) % mapping_count;
            animation_time = 0.0f;  // Reset animation
            animate = true;
            
            // Regenerate points with new mapping
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, 10, 36, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, 24, 20, 5.0f, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, 10, 24, mappings[current_mapping]);
                    break;
            }
        }
        
        // Toggle animation
        if (IsKeyPressed(KEY_SPACE)) {
            animate = !animate;
            if (animate) {
                animation_time = 0.0f;  // Reset animation time when starting
            }
        }
        
        // Update animation
        if (animate) {
            animation_time += animation_speed;
            if (animation_time > 1.0f) {
                animation_time = 0.0f;  // Loop the animation
            }
        }
        
        // Begin drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw title and instructions
        DrawText("animated conformal mapping", 20, 20, 20, BLACK);
        DrawText("left/right: change input graph   up/down: change mapping   space: toggle animation", 20, 50, 15, DARKGRAY);
        DrawText(TextFormat("input: %s    mapping: %s", graph_names[current_graph], mapping_names[current_mapping]), 20, 80, 18, MAROON);
        DrawText(TextFormat("animation: %s   progress: %.0f%%", animate ? "ON" : "OFF", animation_time * 100), 20, 110, 15, DARKGRAY);
        
        // Draw axes
        DrawLine(0, center.y, screenWidth, center.y, LIGHTGRAY);
        DrawLine(center.x, 0, center.x, screenHeight, LIGHTGRAY);
        
        // Draw all points
        for (int i = 0; i < point_count; i++) {
            // Calculate interpolated position based on animation time
            Complex interpolated = complex_lerp(points[i].z, points[i].w, animation_time);
            
            // Draw the point
            draw_complex_circle(interpolated, circleRadius, ColorAlpha(BLUE, 0.7f), center, scale);
        }
        
        // Track mouse position in complex coordinates
        Vector2 mouse_pos = GetMousePosition();
        Complex z = screen_to_complex(mouse_pos, center, scale);
        Complex w = mappings[current_mapping](z);
        
        char z_text[50];
        sprintf(z_text, "z = %.2f + %.2fi", z.real, z.imag);
        DrawText(z_text, 20, screenHeight - 40, 15, DARKGRAY);
        
        char w_text[50];
        sprintf(w_text, "f(z) = %.2f + %.2fi", w.real, w.imag);
        DrawText(w_text, 20, screenHeight - 20, 15, DARKGRAY);
        
        // Highlight the current mouse point and its mapping
        Complex interpolated = complex_lerp(z, w, animation_time);
        draw_complex_circle(interpolated, circleRadius * 1.5f, RED, center, scale);
        
        EndDrawing();
    }
    
    // Clean up
    free(points);
    CloseWindow();
    
    return 0;
}