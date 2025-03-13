#include "raylib.h"
#include "complex.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

// Super simple RGB hue function - maps phase to rainbow colors
Color phase_to_color(double phase) {
    // Normalize phase to 0-6
    double hue = fmod(phase + M_PI, 2*M_PI) / (M_PI/3);
    int i = (int)hue;
    double f = hue - i;
    
    unsigned char r = 0, g = 0, b = 0;
    
    switch(i) {
        case 0: r = 255; g = f * 255; break;
        case 1: r = (1 - f) * 255; g = 255; break;
        case 2: g = 255; b = f * 255; break;
        case 3: g = (1 - f) * 255; b = 255; break;
        case 4: b = 255; r = f * 255; break;
        case 5: case 6: b = (1 - f) * 255; r = 255; break;
    }
    
    return (Color){r, g, b, 255};
}

int main(void) {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "exp(z) Domain Coloring");
    SetTargetFPS(60);
    
    // Create base image
    Image colorImage = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(colorImage);
    
    // Domain parameters
    double scale = 10.0;  // Start with a smaller scale to see more
    double centerX = 0.0;
    double centerY = 0.0;
    double time = 0.0;
    bool animate = true;
    
    // Render function
    Color *pixels = LoadImageColors(colorImage);
    
    // Image fill loop - initial coloring
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Convert pixel to complex plane coordinates
            double re = (x - SCREEN_WIDTH/2) / scale + centerX;
            double im = (SCREEN_HEIGHT/2 - y) / scale + centerY;
            
            // Calculate exp(z)
            double complex z = re + im * I;
            double complex result = cexp(z);
            
            // Get magnitude and phase
            double magnitude = cabs(result);
            double phase = carg(result);
            
            // Use phase for hue and magnitude for brightness
            Color color = phase_to_color(phase);
            
            // Scale brightness by magnitude
            float brightness = 0.5 * (1.0 - 1.0/(1.0 + log(magnitude)));
            if (brightness > 1.0) brightness = 1.0;
            if (brightness < 0.0) brightness = 0.0;
            
            color.r = (unsigned char)(color.r * brightness);
            color.g = (unsigned char)(color.g * brightness);
            color.b = (unsigned char)(color.b * brightness);
            
            // Set pixel color
            pixels[y * SCREEN_WIDTH + x] = color;
        }
    }
    
    // Update texture and clean up
    UpdateTexture(texture, pixels);
    UnloadImageColors(pixels);
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update animation
        if (animate) {
            time += GetFrameTime();
            if (time > 2*M_PI) time -= 2*M_PI;
            
            // Re-render with new time (animation)
            pixels = LoadImageColors(colorImage);
            
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    // Convert pixel to complex plane coordinates
                    double re = (x - SCREEN_WIDTH/2) / scale + centerX;
                    double im = (SCREEN_HEIGHT/2 - y) / scale + centerY;
                    
                    // Calculate exp(z) with rotation
                    double complex z = re + im * I;
                    double complex rot = cexp(I * time);  // Rotation factor
                    double complex result = cexp(z) * rot;
                    
                    // Get magnitude and phase
                    double magnitude = cabs(result);
                    double phase = carg(result);
                    
                    // Use phase for hue
                    Color color = phase_to_color(phase);
                    
                    // Scale brightness by magnitude
                    float brightness = 0.5 * (1.0 - 1.0/(1.0 + log(magnitude)));
                    if (brightness > 1.0) brightness = 1.0;
                    if (brightness < 0.0) brightness = 0.0;
                    
                    color.r = (unsigned char)(color.r * brightness);
                    color.g = (unsigned char)(color.g * brightness);
                    color.b = (unsigned char)(color.b * brightness);
                    
                    // Set pixel color
                    pixels[y * SCREEN_WIDTH + x] = color;
                }
            }
            
            // Update texture and clean up
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Handle zooming
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scale *= (wheel > 0) ? 1.2 : 0.8;
            
            // Re-render with new scale
            pixels = LoadImageColors(colorImage);
            
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    // Convert pixel to complex plane coordinates
                    double re = (x - SCREEN_WIDTH/2) / scale + centerX;
                    double im = (SCREEN_HEIGHT/2 - y) / scale + centerY;
                    
                    // Calculate exp(z) with rotation
                    double complex z = re + im * I;
                    double complex rot = cexp(I * time);  // Rotation factor
                    double complex result = cexp(z) * rot;
                    
                    // Get magnitude and phase
                    double magnitude = cabs(result);
                    double phase = carg(result);
                    
                    // Use phase for hue
                    Color color = phase_to_color(phase);
                    
                    // Scale brightness by magnitude
                    float brightness = 0.5 * (1.0 - 1.0/(1.0 + log(magnitude)));
                    if (brightness > 1.0) brightness = 1.0;
                    if (brightness < 0.0) brightness = 0.0;
                    
                    color.r = (unsigned char)(color.r * brightness);
                    color.g = (unsigned char)(color.g * brightness);
                    color.b = (unsigned char)(color.b * brightness);
                    
                    // Set pixel color
                    pixels[y * SCREEN_WIDTH + x] = color;
                }
            }
            
            // Update texture and clean up
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Handle toggling animation with space bar
        if (IsKeyPressed(KEY_SPACE)) {
            animate = !animate;
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            
            // Draw current info
            DrawText(TextFormat("Scale: %.2f", scale), 10, 10, 20, BLACK);
            DrawText(TextFormat("Time: %.2f", time), 10, 40, 20, BLACK);
            DrawText("SPACE to toggle animation, mouse wheel to zoom", 10, SCREEN_HEIGHT - 30, 20, BLACK);
            
            if (!animate) {
                DrawText("Animation PAUSED", 10, 70, 20, RED);
            }
        EndDrawing();
    }
    
    // Cleanup
    UnloadTexture(texture);
    UnloadImage(colorImage);
    CloseWindow();
    
    return 0;
}