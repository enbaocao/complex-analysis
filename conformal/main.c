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

// Easing functions for smoother animations
float ease_in_out_cubic(float t) {
    return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

float ease_out_elastic(float t) {
    const float c4 = (2 * PI) / 3;
    return t == 0 ? 0 : t == 1 ? 1 : pow(2, -10 * t) * sin((t * 10 - 0.75) * c4) + 1;
}

// Keyframe-based interpolation between two complex numbers
Complex complex_lerp(Complex a, Complex b, float t) {
    // Apply easing function to create more elegant motion
    float easedT = ease_in_out_cubic(t);
    
    Complex result;
    result.real = a.real + easedT * (b.real - a.real);
    result.imag = a.imag + easedT * (b.imag - a.imag);
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
    const float radius_step = 0.4f;
    
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
    const float radius_step = 0.4f;
    
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
    const int screenWidth = 800;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "animated conformal mapping");
    SetTargetFPS(60);
    
    // View parameters
    Vector2 center = {screenWidth / 2, screenHeight / 2};
    float scale = 60.0f;  // Smaller scale = more zoomed out
    
    // Animation parameters
    float animation_time = 0.0f;
    float animation_speed = 0.01f;  // adjust for speed
    bool animate = false;
    float previous_animation_time = 0.0f;
    
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
    const int gridSize = 15;
    const float gridSpacing = 0.4f;
    const float circleRadius = 0.03f;
    
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
                    generate_concentric_circles(points, &point_count, 15, 48, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, 32, 30, 6.0f, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, 15, 32, mappings[current_mapping]);
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
                    generate_concentric_circles(points, &point_count, 15, 48, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, 32, 30, 6.0f, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, 15, 32, mappings[current_mapping]);
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
                animation_time = 1.0f;  // Stop at 100% instead of looping
                animate = false;       // Turn off animation when complete
            }
        }
        
        // Begin drawing
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Draw title and instructions
        DrawText("animated conformal mapping", 20, 20, 20, WHITE);
        DrawText("left/right: change input graph   up/down: change mapping   space: toggle animation", 20, 50, 15, GRAY);
        DrawText(TextFormat("input: %s    mapping: %s", graph_names[current_graph], mapping_names[current_mapping]), 20, 80, 18, SKYBLUE);
        DrawText(TextFormat("animation: %s   progress: %.0f%%", animate ? "ON" : "OFF", animation_time * 100), 20, 110, 15, GRAY);
        
        // Draw axes
        DrawLine(0, center.y, screenWidth, center.y, (Color){50, 50, 50, 255});
        DrawLine(center.x, 0, center.x, screenHeight, (Color){50, 50, 50, 255});
        
        // Draw all points
        for (int i = 0; i < point_count; i++) {
            // Calculate interpolated position based on animation time
            Complex interpolated = complex_lerp(points[i].z, points[i].w, animation_time);
            
            // Draw the point
            draw_complex_circle(interpolated, circleRadius, SKYBLUE, center, scale);
        }
        
        // Track mouse position in complex coordinates
        Vector2 mouse_pos = GetMousePosition();
        Complex z = screen_to_complex(mouse_pos, center, scale);
        Complex w = mappings[current_mapping](z);
        
        char z_text[50];
        sprintf(z_text, "z = %.2f + %.2fi", z.real, z.imag);
        DrawText(z_text, 20, screenHeight - 40, 15, GRAY);
        
        char w_text[50];
        sprintf(w_text, "f(z) = %.2f + %.2fi", w.real, w.imag);
        DrawText(w_text, 20, screenHeight - 20, 15, GRAY);
        
        // Highlight the current mouse point and its mapping
        Complex interpolated = complex_lerp(z, w, animation_time);
        draw_complex_circle(interpolated, circleRadius * 2.5f, PINK, center, scale);
        
        EndDrawing();
    }
    
    // Clean up
    free(points);
    CloseWindow();
    
    return 0;
}