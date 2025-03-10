#include "raylib.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>

// Define a complex number structure for our calculations
typedef struct {
    float real;
    float imag;
} Complex;

// Function to create a complex number
Complex complex_create(float real, float imag) {
    Complex c = {real, imag};
    return c;
}

// Function to multiply two complex numbers
Complex complex_multiply(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

// Function to divide two complex numbers
Complex complex_divide(Complex a, Complex b) {
    Complex result;
    float denominator = b.real * b.real + b.imag * b.imag;
    result.real = (a.real * b.real + a.imag * b.imag) / denominator;
    result.imag = (a.imag * b.real - a.real * b.imag) / denominator;
    return result;
}

// Function to add two complex numbers
Complex complex_add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

// Function to get the absolute value of a complex number
float complex_abs(Complex c) {
    return sqrtf(c.real * c.real + c.imag * c.imag);
}

// Sample conformal mappings
Complex identity_mapping(Complex z) {
    return z;
}

// f(z) = z^2
Complex square_mapping(Complex z) {
    return complex_multiply(z, z);
}

// f(z) = 1/z
Complex reciprocal_mapping(Complex z) {
    Complex one = {1.0f, 0.0f};
    return complex_divide(one, z);
}

// f(z) = e^z
Complex exp_mapping(Complex z) {
    Complex result;
    float exp_real = expf(z.real);
    result.real = exp_real * cosf(z.imag);
    result.imag = exp_real * sinf(z.imag);
    return result;
}

// Möbius transformation: f(z) = (az + b)/(cz + d)
Complex mobius_mapping(Complex z) {
    // Example values, you can change these
    Complex a = {1.0f, 0.0f};
    Complex b = {0.0f, 1.0f};
    Complex c = {0.0f, 0.0f};
    Complex d = {1.0f, 0.0f};
    
    Complex numerator = complex_add(complex_multiply(a, z), b);
    Complex denominator = complex_add(complex_multiply(c, z), d);
    
    return complex_divide(numerator, denominator);
}

// Convert from complex plane coordinates to screen coordinates
Vector2 complex_to_screen(Complex z, Vector2 center, float scale) {
    Vector2 screen;
    screen.x = center.x + z.real * scale;
    screen.y = center.y - z.imag * scale; // Flipping y-axis because screen coordinates go down
    return screen;
}

// Convert from screen coordinates to complex plane
Complex screen_to_complex(Vector2 screen, Vector2 center, float scale) {
    Complex z;
    z.real = (screen.x - center.x) / scale;
    z.imag = (center.y - screen.y) / scale; // Flipping y-axis
    return z;
}

// Draw a circle in the complex plane
void draw_complex_circle(Complex center, float radius, Color color, Vector2 screen_center, float scale) {
    Vector2 screen_pos = complex_to_screen(center, screen_center, scale);
    DrawCircle(screen_pos.x, screen_pos.y, radius * scale, color);
}

int main(void) {
    // Initialize window
    const int screenWidth = 1200;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Conformal Mapping Visualization");
    SetTargetFPS(60);
    
    // Define view parameters
    Vector2 source_center = {screenWidth / 4, screenHeight / 2};
    Vector2 target_center = {3 * screenWidth / 4, screenHeight / 2};
    float scale = 100.0f;
    
    // Grid parameters
    const int gridSize = 10;
    const float gridSpacing = 0.5f;
    const float circleRadius = 0.1f;
    
    // Currently selected mapping
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
        "Identity: f(z) = z",
        "Square: f(z) = z^2",
        "Reciprocal: f(z) = 1/z",
        "Exponential: f(z) = e^z",
        "Möbius: f(z) = (az+b)/(cz+d)"
    };
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        if (IsKeyPressed(KEY_RIGHT)) {
            current_mapping = (current_mapping + 1) % mapping_count;
        }
        if (IsKeyPressed(KEY_LEFT)) {
            current_mapping = (current_mapping - 1 + mapping_count) % mapping_count;
        }
        
        // Begin drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw title and instructions
        DrawText("Conformal Mapping Visualization", 20, 20, 20, BLACK);
        DrawText("Use LEFT/RIGHT arrow keys to change mapping", 20, 50, 15, DARKGRAY);
        DrawText(mapping_names[current_mapping], 20, 80, 18, MAROON);
        
        // Draw dividing line
        DrawLine(screenWidth / 2, 0, screenWidth / 2, screenHeight, LIGHTGRAY);
        
        // Draw axes
        DrawLine(source_center.x - scale * gridSize, source_center.y, source_center.x + scale * gridSize, source_center.y, LIGHTGRAY);
        DrawLine(source_center.x, source_center.y - scale * gridSize, source_center.x, source_center.y + scale * gridSize, LIGHTGRAY);
        DrawLine(target_center.x - scale * gridSize, target_center.y, target_center.x + scale * gridSize, target_center.y, LIGHTGRAY);
        DrawLine(target_center.x, target_center.y - scale * gridSize, target_center.x, target_center.y + scale * gridSize, LIGHTGRAY);
        
        // Draw source grid of circles
        for (int i = -gridSize; i <= gridSize; i++) {
            for (int j = -gridSize; j <= gridSize; j++) {
                Complex z = complex_create(i * gridSpacing, j * gridSpacing);
                
                // Skip the origin for reciprocal mapping to avoid division by zero
                if (current_mapping == 2 && i == 0 && j == 0) continue;
                
                // Draw source circle
                draw_complex_circle(z, circleRadius, ColorAlpha(BLUE, 0.5f), source_center, scale);
                
                // Apply mapping and draw target circle
                Complex w = mappings[current_mapping](z);
                
                // Check if the mapped point is within reasonable bounds
                if (complex_abs(w) < 100.0f) {
                    draw_complex_circle(w, circleRadius, ColorAlpha(RED, 0.5f), target_center, scale);
                }
            }
        }
        
        // Label the planes
        DrawText("z-plane (source)", source_center.x - 80, source_center.y + scale * gridSize + 20, 15, DARKGRAY);
        DrawText("w-plane (target)", target_center.x - 80, target_center.y + scale * gridSize + 20, 15, DARKGRAY);
        
        // Track mouse position in complex coordinates
        Vector2 mouse_pos = GetMousePosition();
        if (mouse_pos.x < screenWidth / 2) {
            Complex z = screen_to_complex(mouse_pos, source_center, scale);
            Complex w = mappings[current_mapping](z);
            
            char z_text[50];
            sprintf(z_text, "z = %.2f + %.2fi", z.real, z.imag);
            DrawText(z_text, 20, screenHeight - 40, 15, DARKGRAY);
            
            char w_text[50];
            sprintf(w_text, "f(z) = %.2f + %.2fi", w.real, w.imag);
            DrawText(w_text, 20, screenHeight - 20, 15, DARKGRAY);
            
            // Highlight the current point and its mapping
            draw_complex_circle(z, circleRadius * 1.5f, BLUE, source_center, scale);
            if (complex_abs(w) < 100.0f) {
                draw_complex_circle(w, circleRadius * 1.5f, RED, target_center, scale);
                
                // Draw a line connecting the mouse position in the target plane
                Vector2 target_pos = complex_to_screen(w, target_center, scale);
                DrawLine(mouse_pos.x, mouse_pos.y, target_pos.x, target_pos.y, ColorAlpha(GRAY, 0.5f));
            }
        }
        
        EndDrawing();
    }
    
    // De-initialization
    CloseWindow();
    
    return 0;
}