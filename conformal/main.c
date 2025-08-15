// animated conformal mapping; interpolate input graphs through selected complex maps
#include "raylib.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    float real;
    float imag;
} Complex;

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

float ease_in_out_cubic(float t) {
    return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

float ease_out_elastic(float t) {
    const float c4 = (2 * PI) / 3;
    return t == 0 ? 0 : t == 1 ? 1 : pow(2, -10 * t) * sin((t * 10 - 0.75) * c4) + 1;
}

Complex complex_lerp(Complex a, Complex b, float t) {
    float easedT = ease_in_out_cubic(t);
    
    Complex result;
    result.real = a.real + easedT * (b.real - a.real);
    result.imag = a.imag + easedT * (b.imag - a.imag);
    return result;
}

float complex_abs(Complex c) {
    return sqrtf(c.real * c.real + c.imag * c.imag);
}

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

typedef enum {
    GRID_PATTERN,
    CONCENTRIC_CIRCLES,
    RADIAL_LINES,
    POLAR_GRID
} InputGraphType;

typedef struct {
    Complex z;
    Complex w;
    int connections[8];
    int num_connections;
} MappedPoint;

int find_point_index(MappedPoint* points, int count, Complex z, float epsilon) {
    for (int i = 0; i < count; i++) {
        if (complex_abs(complex_create(points[i].z.real - z.real, points[i].z.imag - z.imag)) < epsilon) {
            return i;
        }
    }
    return -1;
}

void add_connection(MappedPoint* points, int idx1, int idx2) {
    if (idx1 < 0 || idx2 < 0) return;
    
    // Check if connection already exists
    for (int i = 0; i < points[idx1].num_connections; i++) {
        if (points[idx1].connections[i] == idx2) return;
    }
    
    // Add connection if there's space
    if (points[idx1].num_connections < 8) {
        points[idx1].connections[points[idx1].num_connections++] = idx2;
    }
    
    // Add reverse connection
    if (points[idx2].num_connections < 8) {
        points[idx2].connections[points[idx2].num_connections++] = idx1;
    }
}

void generate_grid_points(MappedPoint* points, int* count, float spacing, int size, Complex (*mapping)(Complex)) {
    *count = 0;
    
    // First, generate all points
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
                points[*count].num_connections = 0;
                (*count)++;
            }
        }
    }
    
    // Then, add connections between adjacent grid points
    const float epsilon = 0.01f;
    for (int idx = 0; idx < *count; idx++) {
        Complex z = points[idx].z;
        
        // Connect to adjacent points (right, up, left, down)
        Complex neighbors[4] = {
            complex_create(z.real + spacing, z.imag),
            complex_create(z.real, z.imag + spacing),
            complex_create(z.real - spacing, z.imag),
            complex_create(z.real, z.imag - spacing)
        };
        
        for (int n = 0; n < 4; n++) {
            int neighbor_idx = find_point_index(points, *count, neighbors[n], epsilon);
            if (neighbor_idx >= 0) {
                add_connection(points, idx, neighbor_idx);
            }
        }
    }
}

void generate_concentric_circles(MappedPoint* points, int* count, int num_circles, int points_per_circle, Complex (*mapping)(Complex)) {
    *count = 0;
    const float radius_step = 0.4f;
    int center_idx = -1;
    
    if (mapping != reciprocal_mapping) {
        Complex z = complex_create(0.0f, 0.0f);
        Complex w = mapping(z);
        if (complex_abs(w) < 100.0f) {
            points[*count].z = z;
            points[*count].w = w;
            points[*count].num_connections = 0;
            center_idx = *count;
            (*count)++;
        }
    }
    
    for (int c = 1; c <= num_circles; c++) {
        float radius = c * radius_step;
        int circle_start_idx = *count;
        
        for (int p = 0; p < points_per_circle; p++) {
            float angle = p * (2.0f * PI / points_per_circle);
            Complex z = complex_create(radius * cosf(angle), radius * sinf(angle));
            Complex w = mapping(z);
            
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                points[*count].num_connections = 0;
                (*count)++;
            }
        }
        
        int circle_end_idx = *count - 1;
        for (int i = circle_start_idx; i <= circle_end_idx; i++) {
            int next_idx = (i == circle_end_idx) ? circle_start_idx : i + 1;
            add_connection(points, i, next_idx);
            
            if (c == 1 && center_idx >= 0) {
                add_connection(points, i, center_idx);
            }
            
            if (c > 1 && (i - circle_start_idx) % 4 == 0) {
                for (int prev = circle_start_idx - points_per_circle; prev < circle_start_idx; prev++) {
                    if (prev >= 0) {
                        float angle1 = atan2f(points[i].z.imag, points[i].z.real);
                        float angle2 = atan2f(points[prev].z.imag, points[prev].z.real);
                        if (fabsf(angle1 - angle2) < 0.1f) {
                            add_connection(points, i, prev);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void generate_radial_lines(MappedPoint* points, int* count, int num_lines, int points_per_line, float max_radius, Complex (*mapping)(Complex)) {
    *count = 0;
    int center_idx = -1;
    int line_start_indices[64] = {0};
    
    if (mapping != reciprocal_mapping) {
        Complex z = complex_create(0.0f, 0.0f);
        Complex w = mapping(z);
        if (complex_abs(w) < 100.0f) {
            points[*count].z = z;
            points[*count].w = w;
            points[*count].num_connections = 0;
            center_idx = *count;
            (*count)++;
        }
    }
    
    for (int l = 0; l < num_lines; l++) {
        float angle = l * (2.0f * PI / num_lines);
        float dx = cosf(angle);
        float dy = sinf(angle);
        
        line_start_indices[l] = *count;
        
        for (int p = 1; p <= points_per_line; p++) {
            float radius = p * (max_radius / points_per_line);
            Complex z = complex_create(radius * dx, radius * dy);
            Complex w = mapping(z);
            
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                points[*count].num_connections = 0;
                
                if (p > 1) {
                    add_connection(points, *count, *count - 1);
                } else if (center_idx >= 0) {
                    add_connection(points, *count, center_idx);
                }
                
                (*count)++;
            }
        }
    }
    
    for (int l = 0; l < num_lines; l++) {
        int next_l = (l + 1) % num_lines;
        
        for (int p = 0; p < points_per_line; p++) {
            int idx1 = line_start_indices[l] + p;
            int idx2 = line_start_indices[next_l] + p;
            
            if (idx1 < *count && idx2 < *count) {
                add_connection(points, idx1, idx2);
            }
        }
    }
}

void generate_polar_grid(MappedPoint* points, int* count, int num_circles, int num_lines, Complex (*mapping)(Complex)) {
    *count = 0;
    const float radius_step = 0.4f;
    int center_idx = -1;
    int circle_start_indices[32] = {0};
    
    if (mapping != reciprocal_mapping) {
        Complex z = complex_create(0.0f, 0.0f);
        Complex w = mapping(z);
        if (complex_abs(w) < 100.0f) {
            points[*count].z = z;
            points[*count].w = w;
            points[*count].num_connections = 0;
            center_idx = *count;
            (*count)++;
        }
    }
    
    for (int c = 1; c <= num_circles; c++) {
        float radius = c * radius_step;
        circle_start_indices[c-1] = *count;
        
        for (int l = 0; l < num_lines; l++) {
            float angle = l * (2.0f * PI / num_lines);
            Complex z = complex_create(radius * cosf(angle), radius * sinf(angle));
            Complex w = mapping(z);
            
            if (complex_abs(w) < 100.0f) {
                points[*count].z = z;
                points[*count].w = w;
                points[*count].num_connections = 0;
                (*count)++;
            }
        }
    }
    
    for (int c = 1; c <= num_circles; c++) {
        int circle_start = circle_start_indices[c-1];
        int circle_end = (c < num_circles) ? circle_start_indices[c] - 1 : *count - 1;
        int points_in_circle = circle_end - circle_start + 1;
        
        for (int i = circle_start; i <= circle_end; i++) {
            int next_idx = (i == circle_end) ? circle_start : i + 1;
            add_connection(points, i, next_idx);
            
            if (c > 1) {
                int prev_circle_start = circle_start_indices[c-2];
                int angle_idx = (i - circle_start) % points_in_circle;
                int prev_idx = prev_circle_start + angle_idx;
                
                if (prev_idx >= 0 && prev_idx < *count) {
                    add_connection(points, i, prev_idx);
                }
            } else if (center_idx >= 0) {
                add_connection(points, i, center_idx);
            }
        }
    }
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "animated conformal mapping");
    SetTargetFPS(60);
    
    Vector2 center = {screenWidth / 2, screenHeight / 2};
    float scale = 60.0f;
    
    float animation_time = 0.0f;
    float animation_speed = 0.01f;
    bool animate = false;
    float previous_animation_time = 0.0f;
    
    const int MAX_POINTS = 6000;
    MappedPoint* points = (MappedPoint*)malloc(MAX_POINTS * sizeof(MappedPoint));
    int point_count = 0;
    
    InputGraphType current_graph = GRID_PATTERN;
    const int graph_count = 4;
    const char* graph_names[graph_count] = {
        "rectangular grid",
        "concentric circles",
        "radial lines",
        "polar grid"
    };
    
    const int gridSize = 25;          // half-extent in steps from origin
    const float gridSpacing = 0.25f;  // spacing of grid points
    const float circleRadius = 0.015f;

    const int circlesCount = 24;
    const int pointsPerCircle = 96;
    const int radialLines = 64;
    const int pointsPerRadial = 40;
    const float radialMaxRadius = 6.0f;
    const int polarCircles = 24;
    const int polarLines = 64;
    
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
    
    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
    
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_RIGHT)) {
            current_graph = (current_graph + 1) % graph_count;
            animation_time = 0.0f;
            animate = true;
            
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, circlesCount, pointsPerCircle, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, radialLines, pointsPerRadial, radialMaxRadius, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, polarCircles, polarLines, mappings[current_mapping]);
                    break;
            }
        }
        
        if (IsKeyPressed(KEY_LEFT)) {
            current_graph = (current_graph - 1 + graph_count) % graph_count;
            animation_time = 0.0f;
            animate = true;
            
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, circlesCount, pointsPerCircle, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, radialLines, pointsPerRadial, radialMaxRadius, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, polarCircles, polarLines, mappings[current_mapping]);
                    break;
            }
        }
        
        if (IsKeyPressed(KEY_DOWN)) {
            current_mapping = (current_mapping + 1) % mapping_count;
            animation_time = 0.0f;
            animate = true;
            
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, circlesCount, pointsPerCircle, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, radialLines, pointsPerRadial, radialMaxRadius, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, polarCircles, polarLines, mappings[current_mapping]);
                    break;
            }
        }
        
        if (IsKeyPressed(KEY_UP)) {
            current_mapping = (current_mapping - 1 + mapping_count) % mapping_count;
            animation_time = 0.0f;
            animate = true;
            
            switch (current_graph) {
                case GRID_PATTERN:
                    generate_grid_points(points, &point_count, gridSpacing, gridSize, mappings[current_mapping]);
                    break;
                case CONCENTRIC_CIRCLES:
                    generate_concentric_circles(points, &point_count, circlesCount, pointsPerCircle, mappings[current_mapping]);
                    break;
                case RADIAL_LINES:
                    generate_radial_lines(points, &point_count, radialLines, pointsPerRadial, radialMaxRadius, mappings[current_mapping]);
                    break;
                case POLAR_GRID:
                    generate_polar_grid(points, &point_count, polarCircles, polarLines, mappings[current_mapping]);
                    break;
            }
        }
        
        if (IsKeyPressed(KEY_SPACE)) {
            animate = !animate;
            if (animate) {
                animation_time = 0.0f;
            }
        }
        
        if (animate) {
            animation_time += animation_speed;
            if (animation_time > 1.0f) {
                animation_time = 1.0f;
                animate = false;
            }
        }
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        DrawText("animated conformal mapping", 20, 20, 20, WHITE);
        DrawText("left/right: change input graph   up/down: change mapping   space: toggle animation", 20, 50, 15, GRAY);
        DrawText(TextFormat("input: %s    mapping: %s", graph_names[current_graph], mapping_names[current_mapping]), 20, 80, 18, SKYBLUE);
        DrawText(TextFormat("animation: %s   progress: %.0f%%", animate ? "ON" : "OFF", animation_time * 100), 20, 110, 15, GRAY);
        
        DrawLine(0, center.y, screenWidth, center.y, (Color){50, 50, 50, 255});
        DrawLine(center.x, 0, center.x, screenHeight, (Color){50, 50, 50, 255});
        
        for (int i = 0; i < point_count; i++) {
            Complex p1 = complex_lerp(points[i].z, points[i].w, animation_time);
            Vector2 screen_p1 = complex_to_screen(p1, center, scale);
            
            for (int c = 0; c < points[i].num_connections; c++) {
                int connect_idx = points[i].connections[c];
                if (i < connect_idx) {
                    Complex p2 = complex_lerp(points[connect_idx].z, points[connect_idx].w, animation_time);
                    Vector2 screen_p2 = complex_to_screen(p2, center, scale);
                    
                    DrawLineEx(screen_p1, screen_p2, 1.0f, (Color){30, 30, 80, 255});
                }
            }
        }
        
        for (int i = 0; i < point_count; i++) {
            Complex interpolated = complex_lerp(points[i].z, points[i].w, animation_time);
            draw_complex_circle(interpolated, circleRadius, SKYBLUE, center, scale);
        }
        
        Vector2 mouse_pos = GetMousePosition();
        Complex z = screen_to_complex(mouse_pos, center, scale);
        Complex w = mappings[current_mapping](z);
        
        char z_text[50];
        sprintf(z_text, "z = %.2f + %.2fi", z.real, z.imag);
        DrawText(z_text, 20, screenHeight - 40, 15, GRAY);
        
        char w_text[50];
        sprintf(w_text, "f(z) = %.2f + %.2fi", w.real, w.imag);
        DrawText(w_text, 20, screenHeight - 20, 15, GRAY);
        
        Complex interpolated = complex_lerp(z, w, animation_time);
        draw_complex_circle(interpolated, circleRadius * 2.5f, PINK, center, scale);
        
        EndDrawing();
    }
    
    free(points);
    CloseWindow();
    
    return 0;
}