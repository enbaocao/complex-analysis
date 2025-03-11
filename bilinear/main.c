#include "raylib.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600
#define DOMAIN_WIDTH 600
#define DOMAIN_HEIGHT 600
#define RANGE_WIDTH 600
#define RANGE_HEIGHT 600
#define DOMAIN_ORIGIN_X (DOMAIN_WIDTH / 2)
#define DOMAIN_ORIGIN_Y (DOMAIN_HEIGHT / 2)
#define RANGE_ORIGIN_X (DOMAIN_WIDTH + RANGE_WIDTH / 2)
#define RANGE_ORIGIN_Y (RANGE_HEIGHT / 2)
#define SCALE 100.0f  // pixels per unit in complex plane

// Transformation parameters
double complex a = 1.0 + 0.0*I;
double complex b = 0.0 + 0.0*I;
double complex c = 0.0 + 0.0*I;
double complex d = 1.0 + 0.0*I;

// Object types to visualize
typedef enum {
    GRID,
    CIRCLE,
    DISK, 
    LINE,
    HALF_PLANE
} ObjectType;

// Complex plane to screen coordinate conversion functions
Vector2 ComplexToScreen(double complex z, Vector2 origin) {
    return (Vector2){ origin.x + creal(z) * SCALE, origin.y - cimag(z) * SCALE };
}

double complex ScreenToComplex(Vector2 pos, Vector2 origin) {
    return ((pos.x - origin.x) / SCALE) + ((origin.y - pos.y) / SCALE) * I;
}

// Bilinear transformation function: w = (az + b) / (cz + d)
double complex BilinearTransform(double complex z) {
    double complex numerator = a * z + b;
    double complex denominator = c * z + d;
    
    // Check for division by zero (poles)
    if (cabs(denominator) < 1e-10) {
        // Return a very large number to indicate "infinity"
        return 1e10 + 1e10*I;
    }
    
    return numerator / denominator;
}

// Draw a grid in complex plane
void DrawComplexGrid(Vector2 origin, bool transformed) {
    // Draw axes
    DrawLine(0, origin.y, DOMAIN_WIDTH, origin.y, GRAY);
    DrawLine(origin.x, 0, origin.x, DOMAIN_HEIGHT, GRAY);
    
    // Draw grid lines
    for (float x = -5.0f; x <= 5.0f; x += 1.0f) {
        if (fabs(x) < 0.01f) continue; // Skip the axes as we already drew them
        
        // Vertical grid lines
        Vector2 start = { origin.x + x * SCALE, 0 };
        Vector2 end = { origin.x + x * SCALE, DOMAIN_HEIGHT };
        
        if (transformed) {
            // Transform the line by sampling points
            for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
                double complex z1 = ScreenToComplex((Vector2){ start.x, start.y + t * (end.y - start.y) }, 
                                                   (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
                double complex w1 = BilinearTransform(z1);
                
                double complex z2 = ScreenToComplex((Vector2){ start.x, start.y + (t + 0.01f) * (end.y - start.y) }, 
                                                   (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
                double complex w2 = BilinearTransform(z2);
                
                Vector2 p1 = ComplexToScreen(w1, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
                Vector2 p2 = ComplexToScreen(w2, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
                
                // Only draw if within visible range
                if (fabs(creal(w1)) < 10.0 && fabs(cimag(w1)) < 10.0 &&
                    fabs(creal(w2)) < 10.0 && fabs(cimag(w2)) < 10.0) {
                    DrawLineV(p1, p2, DARKGREEN);
                }
            }
        } else {
            DrawLineV(start, end, LIGHTGRAY);
        }
        
        // Horizontal grid lines
        start = (Vector2){ 0, origin.y - x * SCALE };
        end = (Vector2){ DOMAIN_WIDTH, origin.y - x * SCALE };
        
        if (transformed) {
            // Transform the line by sampling points
            for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
                double complex z1 = ScreenToComplex((Vector2){ start.x + t * (end.x - start.x), start.y }, 
                                                   (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
                double complex w1 = BilinearTransform(z1);
                
                double complex z2 = ScreenToComplex((Vector2){ start.x + (t + 0.01f) * (end.x - start.x), start.y }, 
                                                   (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
                double complex w2 = BilinearTransform(z2);
                
                Vector2 p1 = ComplexToScreen(w1, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
                Vector2 p2 = ComplexToScreen(w2, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
                
                // Only draw if within visible range
                if (fabs(creal(w1)) < 10.0 && fabs(cimag(w1)) < 10.0 &&
                    fabs(creal(w2)) < 10.0 && fabs(cimag(w2)) < 10.0) {
                    DrawLineV(p1, p2, DARKBLUE);
                }
            }
        } else {
            DrawLineV(start, end, LIGHTGRAY);
        }
    }
}

// Draw a circle in the complex plane
void DrawComplexCircle(Vector2 origin, Vector2 center, float radius, Color color, bool transformed) {
    if (!transformed) {
        DrawCircleLines(center.x, center.y, radius * SCALE, color);
    } else {
        // Transform the circle by sampling points along it
        for (float theta = 0.0f; theta < 2.0f * PI; theta += 0.05f) {
            double complex z1 = ScreenToComplex((Vector2){ 
                center.x + radius * SCALE * cosf(theta),
                center.y - radius * SCALE * sinf(theta) 
            }, (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
            
            double complex z2 = ScreenToComplex((Vector2){ 
                center.x + radius * SCALE * cosf(theta + 0.05f),
                center.y - radius * SCALE * sinf(theta + 0.05f) 
            }, (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
            
            double complex w1 = BilinearTransform(z1);
            double complex w2 = BilinearTransform(z2);
            
            Vector2 p1 = ComplexToScreen(w1, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
            Vector2 p2 = ComplexToScreen(w2, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
            
            // Only draw if within visible range
            if (fabs(creal(w1)) < 10.0 && fabs(cimag(w1)) < 10.0 &&
                fabs(creal(w2)) < 10.0 && fabs(cimag(w2)) < 10.0) {
                DrawLineV(p1, p2, color);
            }
        }
    }
}

// Draw a disk (filled circle) in the complex plane
void DrawComplexDisk(Vector2 origin, Vector2 center, float radius, Color color, bool transformed) {
    if (!transformed) {
        DrawCircle(center.x, center.y, radius * SCALE, ColorAlpha(color, 0.3f));
        DrawCircleLines(center.x, center.y, radius * SCALE, color);
    } else {
        // We'll visualize the transformed disk by transforming a grid of points
        // This is a simple approach; more sophisticated methods exist
        int resolution = 30;
        
        for (int i = -resolution; i <= resolution; i++) {
            for (int j = -resolution; j <= resolution; j++) {
                Vector2 point = {
                    center.x + (i * radius * SCALE) / resolution,
                    center.y + (j * radius * SCALE) / resolution
                };
                
                float dist = sqrt(pow(point.x - center.x, 2) + pow(point.y - center.y, 2));
                if (dist <= radius * SCALE) {
                    double complex z = ScreenToComplex(point, (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
                    double complex w = BilinearTransform(z);
                    
                    if (fabs(creal(w)) < 10.0 && fabs(cimag(w)) < 10.0) {
                        Vector2 transformedPoint = ComplexToScreen(w, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
                        DrawPixel(transformedPoint.x, transformedPoint.y, ColorAlpha(color, 0.5f));
                    }
                }
            }
        }
    }
}

// Draw a line in the complex plane
void DrawComplexLine(Vector2 origin, Vector2 start, Vector2 end, Color color, bool transformed) {
    if (!transformed) {
        DrawLineV(start, end, color);
    } else {
        // Transform the line by sampling points
        for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
            Vector2 point1 = {
                start.x + t * (end.x - start.x),
                start.y + t * (end.y - start.y)
            };
            
            Vector2 point2 = {
                start.x + (t + 0.01f) * (end.x - start.x),
                start.y + (t + 0.01f) * (end.y - start.y)
            };
            
            double complex z1 = ScreenToComplex(point1, (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
            double complex z2 = ScreenToComplex(point2, (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
            
            double complex w1 = BilinearTransform(z1);
            double complex w2 = BilinearTransform(z2);
            
            Vector2 p1 = ComplexToScreen(w1, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
            Vector2 p2 = ComplexToScreen(w2, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
            
            // Only draw if within visible range
            if (fabs(creal(w1)) < 10.0 && fabs(cimag(w1)) < 10.0 &&
                fabs(creal(w2)) < 10.0 && fabs(cimag(w2)) < 10.0) {
                DrawLineV(p1, p2, color);
            }
        }
    }
}

// Draw a half-plane in the complex plane
void DrawComplexHalfPlane(Vector2 origin, Vector2 lineStart, Vector2 lineEnd, Color color, bool transformed) {
    if (!transformed) {
        // Draw the boundary line
        DrawLineV(lineStart, lineEnd, color);
        
        // Calculate the normal vector to the line (points to the half-plane side)
        Vector2 tangent = { lineEnd.x - lineStart.x, lineEnd.y - lineStart.y };
        Vector2 normal = { -tangent.y, tangent.x };
        float normalLength = sqrt(normal.x * normal.x + normal.y * normal.y);
        normal.x /= normalLength;
        normal.y /= normalLength;
        
        // Fill the half-plane with a semi-transparent color
        // This is a simple approach by drawing lines perpendicular to the boundary
        float stepSize = 10.0f;
        float lineLength = 1000.0f; // Make it long enough to cover the screen
        
        for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
            Vector2 point = {
                lineStart.x + t * (lineEnd.x - lineStart.x),
                lineStart.y + t * (lineEnd.y - lineStart.y)
            };
            
            Vector2 endPoint = {
                point.x + normal.x * lineLength,
                point.y + normal.y * lineLength
            };
            
            DrawLineV(point, endPoint, ColorAlpha(color, 0.1f));
        }
    } else {
        // Transform the half-plane boundary
        DrawComplexLine(origin, lineStart, lineEnd, color, true);
        
        // Calculate the normal vector (pointing to the half-plane side)
        Vector2 tangent = { lineEnd.x - lineStart.x, lineEnd.y - lineStart.y };
        Vector2 normal = { -tangent.y, tangent.x };
        float normalLength = sqrt(normal.x * normal.x + normal.y * normal.y);
        normal.x /= normalLength;
        normal.y /= normalLength;
        
        // Transform a grid of points on the half-plane side
        int resolution = 30;
        float maxDist = 5.0f * SCALE;
        
        for (int i = 0; i <= resolution; i++) {
            float t = (float)i / resolution;
            Vector2 linePoint = {
                lineStart.x + t * (lineEnd.x - lineStart.x),
                lineStart.y + t * (lineEnd.y - lineStart.y)
            };
            
            for (int j = 1; j <= resolution / 2; j++) {
                float distance = (float)j / (resolution / 2) * maxDist;
                Vector2 point = {
                    linePoint.x + normal.x * distance,
                    linePoint.y + normal.y * distance
                };
                
                double complex z = ScreenToComplex(point, (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y });
                double complex w = BilinearTransform(z);
                
                if (fabs(creal(w)) < 10.0 && fabs(cimag(w)) < 10.0) {
                    Vector2 transformedPoint = ComplexToScreen(w, (Vector2){ RANGE_ORIGIN_X, RANGE_ORIGIN_Y });
                    DrawPixel(transformedPoint.x, transformedPoint.y, ColorAlpha(color, 0.5f));
                }
            }
        }
    }
}

// Special function to visualize mapping the unit disk to the upper half-plane
void PresetUnitDiskToUpperHalfPlane() {
    // The mapping is z -> i(1+z)/(1-z)
    a = 0.0 + 1.0*I;
    b = 0.0 + 1.0*I;
    c = 1.0 + 0.0*I;
    d = -1.0 + 0.0*I;
}

// Another special case: mapping a half-plane to a disk
void PresetUpperHalfPlaneToUnitDisk() {
    // The mapping is z -> (z-i)/(z+i)
    a = 1.0 + 0.0*I;
    b = 0.0 - 1.0*I;
    c = 1.0 + 0.0*I;
    d = 0.0 + 1.0*I;
}

int main(void) {
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bilinear Mapping Visualizer");
    SetTargetFPS(60);
    
    // Variables for parameters
    bool showGrid = true;
    bool showUnitCircle = true;
    bool showUnitDisk = false;
    bool showCustomCircle = false;
    bool showCustomLine = false;
    bool showHalfPlane = false;
    Vector2 circleCenter = { DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y };
    float circleRadius = 1.0f;
    Vector2 lineStart = { DOMAIN_ORIGIN_X - 2.0f * SCALE, DOMAIN_ORIGIN_Y };
    Vector2 lineEnd = { DOMAIN_ORIGIN_X + 2.0f * SCALE, DOMAIN_ORIGIN_Y };
    
    // Define the half-plane boundary (horizontal line across the origin)
    Vector2 halfPlaneStart = { 0, DOMAIN_ORIGIN_Y };
    Vector2 halfPlaneEnd = { DOMAIN_WIDTH, DOMAIN_ORIGIN_Y };
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        
        // Toggle visualization options
        if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
        if (IsKeyPressed(KEY_C)) showUnitCircle = !showUnitCircle;
        if (IsKeyPressed(KEY_D)) showUnitDisk = !showUnitDisk;
        if (IsKeyPressed(KEY_X)) showCustomCircle = !showCustomCircle;
        if (IsKeyPressed(KEY_L)) showCustomLine = !showCustomLine;
        if (IsKeyPressed(KEY_H)) showHalfPlane = !showHalfPlane;
        
        // Preset transformations
        if (IsKeyPressed(KEY_ONE)) PresetUnitDiskToUpperHalfPlane();
        if (IsKeyPressed(KEY_TWO)) PresetUpperHalfPlaneToUnitDisk();
        
        // Modify transformation parameters with keyboard
        float step = 0.1f;
        if (IsKeyDown(KEY_LEFT_SHIFT)) step = 0.01f;
        
        // Adjust real parts
        if (IsKeyDown(KEY_Q)) a = creal(a) + step + cimag(a)*I;
        if (IsKeyDown(KEY_A)) a = creal(a) - step + cimag(a)*I;
        if (IsKeyDown(KEY_W)) b = creal(b) + step + cimag(b)*I;
        if (IsKeyDown(KEY_S)) b = creal(b) - step + cimag(b)*I;
        if (IsKeyDown(KEY_E)) c = creal(c) + step + cimag(c)*I;
        if (IsKeyDown(KEY_D)) c = creal(c) - step + cimag(c)*I;
        if (IsKeyDown(KEY_R)) d = creal(d) + step + cimag(d)*I;
        if (IsKeyDown(KEY_F)) d = creal(d) - step + cimag(d)*I;
        
        // Adjust imaginary parts
        if (IsKeyDown(KEY_T)) a = creal(a) + (cimag(a) + step)*I;
        if (IsKeyDown(KEY_G)) a = creal(a) + (cimag(a) - step)*I;
        if (IsKeyDown(KEY_Y)) b = creal(b) + (cimag(b) + step)*I;
        if (IsKeyDown(KEY_H)) b = creal(b) + (cimag(b) - step)*I;
        if (IsKeyDown(KEY_U)) c = creal(c) + (cimag(c) + step)*I;
        if (IsKeyDown(KEY_J)) c = creal(c) + (cimag(c) - step)*I;
        if (IsKeyDown(KEY_I)) d = creal(d) + (cimag(d) + step)*I;
        if (IsKeyDown(KEY_K)) d = creal(d) + (cimag(d) - step)*I;
        
        // Reset parameters
        if (IsKeyPressed(KEY_SPACE)) {
            a = 1.0 + 0.0*I;
            b = 0.0 + 0.0*I;
            c = 0.0 + 0.0*I;
            d = 1.0 + 0.0*I;
        }
        
        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw domain (z-plane)
        DrawRectangleLines(0, 0, DOMAIN_WIDTH, DOMAIN_HEIGHT, DARKGRAY);
        DrawText("Domain (z-plane)", 10, 10, 20, DARKGRAY);
        
        // Draw range (w-plane)
        DrawRectangleLines(DOMAIN_WIDTH, 0, RANGE_WIDTH, RANGE_HEIGHT, DARKGRAY);
        DrawText("Range (w-plane)", DOMAIN_WIDTH + 10, 10, 20, DARKGRAY);
        
        // Draw bilinear mapping formula
        char formula[100];
        sprintf(formula, "w = (%.1f%+.1fi)z + (%.1f%+.1fi)", creal(a), cimag(a), creal(b), cimag(b));
        DrawText(formula, 10, DOMAIN_HEIGHT + 10, 20, DARKGRAY);
        
        char formula2[100];
        sprintf(formula2, "    (%.1f%+.1fi)z + (%.1f%+.1fi)", creal(c), cimag(c), creal(d), cimag(d));
        DrawText(formula2, 10, DOMAIN_HEIGHT + 40, 20, DARKGRAY);
        
        DrawLine(10, DOMAIN_HEIGHT + 30, 300, DOMAIN_HEIGHT + 30, DARKGRAY);
        
        // Draw controls help
        DrawText("Controls: G-grid, C-unit circle, D-unit disk, X-custom circle, L-custom line, H-half plane", 
                 10, DOMAIN_HEIGHT + 70, 15, DARKGRAY);
        DrawText("Presets: 1-disk to half-plane, 2-half-plane to disk, SPACE-reset", 
                 10, DOMAIN_HEIGHT + 90, 15, DARKGRAY);
        
        // Draw grids
        if (showGrid) {
            DrawComplexGrid((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, false);
            DrawComplexGrid((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, true);
        }
        
        // Draw unit circle
        if (showUnitCircle) {
            DrawComplexCircle((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                             (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                             1.0f, RED, false);
            DrawComplexCircle((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                             (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                             1.0f, RED, true);
        }
        
        // Draw unit disk
        if (showUnitDisk) {
            DrawComplexDisk((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                           (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                           1.0f, BLUE, false);
            DrawComplexDisk((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                           (Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                           1.0f, BLUE, true);
        }
        
        // Draw custom circle
        if (showCustomCircle) {
            DrawComplexCircle((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                             circleCenter, circleRadius, GREEN, false);
            DrawComplexCircle((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                             circleCenter, circleRadius, GREEN, true);
        }
        
        // Draw custom line
        if (showCustomLine) {
            DrawComplexLine((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                           lineStart, lineEnd, PURPLE, false);
            DrawComplexLine((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                           lineStart, lineEnd, PURPLE, true);
        }
        
        // Draw half-plane
        if (showHalfPlane) {
            DrawComplexHalfPlane((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                                halfPlaneStart, halfPlaneEnd, ORANGE, false);
            DrawComplexHalfPlane((Vector2){ DOMAIN_ORIGIN_X, DOMAIN_ORIGIN_Y }, 
                                halfPlaneStart, halfPlaneEnd, ORANGE, true);
        }
        
        EndDrawing();
    }
    
    // De-Initialization
    CloseWindow();
    
    return 0;
}