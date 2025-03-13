#include "raylib.h"
#include "complex.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

// Function types
typedef enum {
    FUNC_EXP,
    FUNC_SIN,
    FUNC_TAN,
    FUNC_INVERSE,
    FUNC_SQUARE,
    FUNC_SQUARE_MINUS_ONE,
    FUNC_POLY5_MINUS_Z,
    FUNC_COUNT // Always keep last for counting
} FunctionType;

// Function names
const char* function_names[] = {
    "exp(z)",
    "sin(z)",
    "tan(z)",
    "1/z",
    "z^2",
    "z^2 - 1",
    "z^5 - z"
};

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

// Apply brightness based on magnitude
Color apply_brightness(Color color, double magnitude) {
    // Scale brightness by magnitude using log scale
    float brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude)));
    if (brightness > 1.0) brightness = 1.0;
    if (brightness < 0.0) brightness = 0.0;
    
    color.r = (unsigned char)(color.r * brightness);
    color.g = (unsigned char)(color.g * brightness);
    color.b = (unsigned char)(color.b * brightness);
    
    return color;
}

// Evaluate complex function based on type
double complex evaluate_function(double complex z, FunctionType type, double time) {
    double complex rot = cexp(I * time); // Rotation factor for animation
    
    switch(type) {
        case FUNC_EXP:
            return cexp(z) * rot;
        case FUNC_SIN:
            return csin(z) * rot;
        case FUNC_TAN:
            return ctan(z);
        case FUNC_INVERSE:
            if (cabs(z) < 1e-10) return 0.0; // Avoid division by zero
            return 1.0 / z;
        case FUNC_SQUARE:
            return z * z * rot;
        case FUNC_SQUARE_MINUS_ONE:
            return z * z - 1.0;
        case FUNC_POLY5_MINUS_Z:
            return cpow(z, 5) - z;
        default:
            return z; // Identity function as fallback
    }
}

// Render the domain coloring for a function
void render_domain_coloring(Color *pixels, FunctionType func_type, double centerX, double centerY, double scale, double time) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Convert pixel to complex plane coordinates
            double re = (x - SCREEN_WIDTH/2) / scale + centerX;
            double im = (SCREEN_HEIGHT/2 - y) / scale + centerY;
            
            // Calculate complex value
            double complex z = re + im * I;
            double complex result = evaluate_function(z, func_type, time);
            
            // Get magnitude and phase
            double magnitude = cabs(result);
            double phase = carg(result);
            
            // Use phase for hue and apply brightness based on magnitude
            Color color = phase_to_color(phase);
            color = apply_brightness(color, magnitude);
            
            // Set pixel color
            pixels[y * SCREEN_WIDTH + x] = color;
        }
    }
}

// Draw a legend showing the phase-to-color mapping
void draw_phase_legend() {
    // Draw the legend background
    DrawRectangle(SCREEN_WIDTH - 100, 60, 80, 150, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - 100, 60, 80, 150, BLACK);
    
    // Draw the title
    DrawText("Phase", SCREEN_WIDTH - 90, 65, 18, BLACK);
    
    // Draw color gradient
    for (int i = 0; i < 120; i++) {
        // Convert position to phase (-π to π)
        double phase = M_PI * (2.0 * i / 120.0 - 1.0);
        Color color = phase_to_color(phase);
        
        // Draw a line segment with this color
        DrawLine(SCREEN_WIDTH - 90, 90 + i, SCREEN_WIDTH - 30, 90 + i, color);
    }
    
    // Label the ends
    DrawText("-π", SCREEN_WIDTH - 90, 85 + 120, 16, BLACK);
    DrawText("+π", SCREEN_WIDTH - 45, 85 + 120, 16, BLACK);
}

int main(void) {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Complex Domain Coloring");
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
    FunctionType current_function = FUNC_EXP;
    
    // Initial render
    Color *pixels = LoadImageColors(colorImage);
    render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
    UpdateTexture(texture, pixels);
    UnloadImageColors(pixels);
    
    // UI elements
    Rectangle functionButton = { 10, SCREEN_HEIGHT - 70, 240, 30 };
    Rectangle animateButton = { 10, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle resetButton = { 170, SCREEN_HEIGHT - 110, 150, 30 };
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update animation
        if (animate) {
            time += GetFrameTime() * 0.5; // Slower animation
            if (time > 2*M_PI) time -= 2*M_PI;
            
            // Re-render with new time
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Handle mouse drag for panning
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && GetMouseY() > 100) {
            Vector2 delta = GetMouseDelta();
            centerX -= delta.x / scale;
            centerY += delta.y / scale;
            
            // Re-render with new center
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Handle zooming
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scale *= (wheel > 0) ? 1.2 : 0.8;
            
            // Re-render with new scale
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Handle function button click
        if (CheckCollisionPointRec(GetMousePosition(), functionButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            current_function = (current_function + 1) % FUNC_COUNT;
            
            // Re-render with new function
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Handle animate button click
        if (CheckCollisionPointRec(GetMousePosition(), animateButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            animate = !animate;
        }
        
        // Handle reset button click
        if (CheckCollisionPointRec(GetMousePosition(), resetButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            centerX = 0.0;
            centerY = 0.0;
            scale = 10.0;
            time = 0.0;
            
            // Re-render with reset parameters
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Toggle animation with space
        if (IsKeyPressed(KEY_SPACE)) {
            animate = !animate;
        }
        
        // Change function with left/right arrows
        if (IsKeyPressed(KEY_RIGHT)) {
            current_function = (current_function + 1) % FUNC_COUNT;
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        if (IsKeyPressed(KEY_LEFT)) {
            current_function = (current_function + FUNC_COUNT - 1) % FUNC_COUNT;
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, time);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            
            // Draw current info
            DrawText(TextFormat("Scale: %.2f", scale), 10, 10, 20, BLACK);
            DrawText(TextFormat("Time: %.2f", time), 10, 40, 20, BLACK);
            
            // Draw phase legend
            draw_phase_legend();
            
            // Draw UI buttons
            DrawRectangleRec(functionButton, LIGHTGRAY);
            DrawText(TextFormat("Function: %s", function_names[current_function]), 
                     functionButton.x + 10, functionButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(animateButton, LIGHTGRAY);
            DrawText(animate ? "Stop Animation" : "Start Animation", 
                     animateButton.x + 10, animateButton.y + 5, 20, BLACK);
                     
            DrawRectangleRec(resetButton, LIGHTGRAY);
            DrawText("Reset View", resetButton.x + 10, resetButton.y + 5, 20, BLACK);
            
            // Draw help
            DrawText("Left/Right arrows: change function", 10, SCREEN_HEIGHT - 150, 16, DARKGRAY);
            DrawText("Mouse drag: pan view", 10, SCREEN_HEIGHT - 170, 16, DARKGRAY);
            DrawText("Mouse wheel: zoom in/out", 10, SCREEN_HEIGHT - 190, 16, DARKGRAY);
            DrawText("SPACE: toggle animation", 10, SCREEN_HEIGHT - 210, 16, DARKGRAY);
            
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