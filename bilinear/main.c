#include "raylib.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define ORIGIN_X (SCREEN_WIDTH / 2)
#define ORIGIN_Y (SCREEN_HEIGHT / 2)
#define SCALE 50.0f  // pixels per unit in complex plane

// Input type options
typedef enum {
    INPUT_LINES,
    INPUT_CIRCLE
} InputType;

// Transformation type options
typedef enum {
    TRANSFORM_IDENTITY,
    TRANSFORM_CIRCLE_AND_LINE_PRESERVING,
    TRANSFORM_CIRCLE_TO_HALFPLANE
} TransformType;

// Global state
InputType currentInput = INPUT_LINES;
TransformType currentTransform = TRANSFORM_IDENTITY;

// Transform parameters for different mappings
double complex a, b, c, d;

// Complex plane to screen coordinate conversion functions
Vector2 ComplexToScreen(double complex z) {
    return (Vector2){ ORIGIN_X + creal(z) * SCALE, ORIGIN_Y - cimag(z) * SCALE };
}

double complex ScreenToComplex(Vector2 pos) {
    return ((pos.x - ORIGIN_X) / SCALE) + ((ORIGIN_Y - pos.y) / SCALE) * I;
}

// Update the bilinear transform parameters based on the selected transformation
void UpdateTransformParameters() {
    switch (currentTransform) {
        case TRANSFORM_IDENTITY:
            a = 1.0 + 0.0*I;
            b = 0.0 + 0.0*I;
            c = 0.0 + 0.0*I;
            d = 1.0 + 0.0*I;
            break;
            
        case TRANSFORM_CIRCLE_AND_LINE_PRESERVING:
            // Maps unit circle to another circle (shifted and scaled)
            // Also preserves the property that lines map to lines
            a = 2.0 + 0.0*I;
            b = 1.0 + 0.5*I;
            c = 0.0 + 0.0*I;
            d = 1.0 + 0.0*I;
            break;
            
        case TRANSFORM_CIRCLE_TO_HALFPLANE:
            // Maps unit circle to upper half-plane: z → i(1+z)/(1-z)
            a = 0.0 + 1.0*I;
            b = 0.0 + 1.0*I;
            c = 1.0 + 0.0*I;
            d = -1.0 + 0.0*I;
            break;
    }
}

// Apply the bilinear transformation
double complex BilinearTransform(double complex z) {
    double complex numerator = a * z + b;
    double complex denominator = c * z + d;
    
    // Check for division by zero
    if (cabs(denominator) < 1e-10) {
        return 1e10 + 1e10*I;  // Return "infinity"
    }
    
    return numerator / denominator;
}

// Draw horizontal lines
void DrawHorizontalLines() {
    // Draw coordinate axes
    DrawLine(0, ORIGIN_Y, SCREEN_WIDTH, ORIGIN_Y, DARKGRAY);
    DrawLine(ORIGIN_X, 0, ORIGIN_X, SCREEN_HEIGHT, DARKGRAY);
    
    // Draw horizontal lines from Im(z) = -5 to Im(z) = 5
    for (int i = -5; i <= 5; i++) {
        if (i == 0) continue;  // Skip the x-axis which we already drew
        
        Color lineColor = BLUE;
        
        // Original line (dotted)
        Vector2 start = ComplexToScreen(-10.0 + 0.0*I + i*I);
        Vector2 end = ComplexToScreen(10.0 + 0.0*I + i*I);
        
        for (float t = 0.0f; t < 1.0f; t += 0.02f) {
            Vector2 p1 = {
                start.x + t * (end.x - start.x),
                start.y + t * (end.y - start.y)
            };
            
            Vector2 p2 = {
                start.x + (t + 0.01f) * (end.x - start.x),
                start.y + (t + 0.01f) * (end.y - start.y)
            };
            
            DrawLineV(p1, p2, ColorAlpha(lineColor, 0.5f));
        }
        
        // Transformed line
        if (currentTransform != TRANSFORM_IDENTITY) {
            for (float x = -10.0f; x <= 10.0f; x += 0.1f) {
                double complex z1 = x + i*I;
                double complex z2 = (x + 0.1f) + i*I;
                
                double complex w1 = BilinearTransform(z1);
                double complex w2 = BilinearTransform(z2);
                
                if (cabs(w1) < 100.0 && cabs(w2) < 100.0) {
                    Vector2 p1 = ComplexToScreen(w1);
                    Vector2 p2 = ComplexToScreen(w2);
                    DrawLineV(p1, p2, RED);
                }
            }
        }
    }
}

// Draw unit circle
void DrawUnitCircle() {
    // Draw coordinate axes
    DrawLine(0, ORIGIN_Y, SCREEN_WIDTH, ORIGIN_Y, DARKGRAY);
    DrawLine(ORIGIN_X, 0, ORIGIN_X, SCREEN_HEIGHT, DARKGRAY);
    
    // Draw original unit circle (dotted)
    float radius = 1.0f;
    DrawCircleLines(ORIGIN_X, ORIGIN_Y, radius * SCALE, ColorAlpha(BLUE, 0.5f));
    
    // For the disk, lightly shade the interior
    if (currentTransform == TRANSFORM_IDENTITY) {
        DrawCircle(ORIGIN_X, ORIGIN_Y, radius * SCALE, ColorAlpha(BLUE, 0.1f));
    }
    
    // Draw transformed circle
    if (currentTransform != TRANSFORM_IDENTITY) {
        // Transform the circle by sampling points along it
        for (float theta = 0.0f; theta < 2.0f * PI; theta += 0.05f) {
            double complex z1 = radius * cosf(theta) + radius * sinf(theta) * I;
            double complex z2 = radius * cosf(theta + 0.05f) + radius * sinf(theta + 0.05f) * I;
            
            double complex w1 = BilinearTransform(z1);
            double complex w2 = BilinearTransform(z2);
            
            if (cabs(w1) < 100.0 && cabs(w2) < 100.0) {
                Vector2 p1 = ComplexToScreen(w1);
                Vector2 p2 = ComplexToScreen(w2);
                DrawLineV(p1, p2, RED);
            }
        }
        
        // For disk visualization, sample points inside the circle
        if (currentTransform == TRANSFORM_CIRCLE_TO_HALFPLANE) {
            int resolution = 30;
            for (int i = -resolution; i <= resolution; i++) {
                for (int j = -resolution; j <= resolution; j++) {
                    float x = (float)i / resolution;
                    float y = (float)j / resolution;
                    
                    float distSq = x*x + y*y;
                    if (distSq <= 1.0f) {
                        double complex z = x + y*I;
                        double complex w = BilinearTransform(z);
                        
                        if (cabs(w) < 100.0) {
                            Vector2 p = ComplexToScreen(w);
                            DrawPixel(p.x, p.y, ColorAlpha(RED, 0.2f));
                        }
                    }
                }
            }
        }
    }
}

int main(void) {
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bilinear Mapping Visualizer");
    SetTargetFPS(60);
    
    // Initialize transform parameters
    UpdateTransformParameters();
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        
        // Handle input to change the input type
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
            currentInput = (currentInput == INPUT_LINES) ? INPUT_CIRCLE : INPUT_LINES;
        }
        
        // Handle input to change the transformation
        if (IsKeyPressed(KEY_UP)) {
            currentTransform = (currentTransform + 1) % 3;
            UpdateTransformParameters();
        }
        if (IsKeyPressed(KEY_DOWN)) {
            currentTransform = (currentTransform + 2) % 3;  // +2 is same as -1 with modulo 3
            UpdateTransformParameters();
        }
        
        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw the selected input with the selected transformation
        if (currentInput == INPUT_LINES) {
            DrawHorizontalLines();
        } else {
            DrawUnitCircle();
        }
        
        // Display current input type
        const char* inputText = (currentInput == INPUT_LINES) ? "Input: Horizontal Lines" : "Input: Unit Circle";
        DrawText(inputText, 10, 10, 20, DARKGRAY);
        
        // Display current transformation
        const char* transformText;
        switch (currentTransform) {
            case TRANSFORM_IDENTITY:
                transformText = "Transform: Identity";
                break;
            case TRANSFORM_CIRCLE_AND_LINE_PRESERVING:
                transformText = "Transform: Circle/Line Preserving";
                break;
            case TRANSFORM_CIRCLE_TO_HALFPLANE:
                transformText = "Transform: Circle to Half-Plane";
                break;
        }
        DrawText(transformText, 10, 40, 20, DARKGRAY);
        
        // Display the bilinear mapping formula
        char formula[100];
        sprintf(formula, "w = (%.1f%+.1fi)z + (%.1f%+.1fi)", creal(a), cimag(a), creal(b), cimag(b));
        DrawText(formula, 10, SCREEN_HEIGHT - 60, 20, DARKGRAY);
        
        char formula2[100];
        sprintf(formula2, "    (%.1f%+.1fi)z + (%.1f%+.1fi)", creal(c), cimag(c), creal(d), cimag(d));
        DrawText(formula2, 10, SCREEN_HEIGHT - 30, 20, DARKGRAY);
        
        DrawLine(10, SCREEN_HEIGHT - 45, 300, SCREEN_HEIGHT - 45, DARKGRAY);
        
        // Display controls
        DrawText("Controls: Left/Right - Change Input, Up/Down - Change Transform", 
                 10, SCREEN_HEIGHT - 90, 15, DARKGRAY);
        
        EndDrawing();
    }
    
    // De-Initialization
    CloseWindow();
    
    return 0;
}