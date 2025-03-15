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

// Domain coloring parameters
typedef struct {
    bool show_phase_lines;
    bool show_modulus_lines;
    bool enhanced_contrast;
    float line_thickness;
} ColoringParams;

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
Color apply_brightness(Color color, double magnitude, bool enhanced_contrast) {
    // Scale brightness by magnitude using log scale
    float brightness;
    
    if (enhanced_contrast) {
        // More pronounced contrast for visualizing magnitude changes
        brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude)));
        brightness = powf(brightness, 0.8); // Enhance contrast
    } else {
        brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude)));
    }
    
    if (brightness > 1.0) brightness = 1.0;
    if (brightness < 0.0) brightness = 0.0;
    
    color.r = (unsigned char)(color.r * brightness);
    color.g = (unsigned char)(color.g * brightness);
    color.b = (unsigned char)(color.b * brightness);
    
    return color;
}

// Add phase lines to highlight equal-argument curves
Color add_phase_lines(Color color, double phase, float thickness) {
    // Highlight phase lines (isochromatic lines)
    double phase_mod = fmod(phase + M_PI, M_PI/4);
    if (phase_mod < thickness || phase_mod > M_PI/4 - thickness) {
        color.r = (color.r + 255) / 2;
        color.g = (color.g + 255) / 2;
        color.b = (color.b + 255) / 2;
    }
    return color;
}

// Add modulus lines to highlight equal-magnitude curves
Color add_modulus_lines(Color color, double magnitude, float thickness) {
    // Highlight magnitude lines (level curves)
    double log_mag = log(magnitude + 1.0);
    double mod = fmod(log_mag, 1.0);
    if (mod < thickness || mod > 1.0 - thickness) {
        color.r = (color.r + 255) / 2;
        color.g = (color.g + 255) / 2;
        color.b = (color.b + 255) / 2;
    }
    return color;
}

// Evaluate complex function based on type
double complex evaluate_function(double complex z, FunctionType type) {
    switch(type) {
        case FUNC_EXP:
            return cexp(z);
        case FUNC_SIN:
            return csin(z);
        case FUNC_TAN:
            return ctan(z);
        case FUNC_INVERSE:
            if (cabs(z) < 1e-10) return 0.0; // Avoid division by zero
            return 1.0 / z;
        case FUNC_SQUARE:
            return z * z;
        case FUNC_SQUARE_MINUS_ONE:
            return z * z - 1.0;
        case FUNC_POLY5_MINUS_Z:
            return cpow(z, 5) - z;
        default:
            return z; // Identity function as fallback
    }
}

// Render the domain coloring for a function
void render_domain_coloring(Color *pixels, FunctionType func_type, double centerX, double centerY, 
                           double scale, ColoringParams params) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Convert pixel to complex plane coordinates
            double re = (x - SCREEN_WIDTH/2) / scale + centerX;
            double im = (SCREEN_HEIGHT/2 - y) / scale + centerY;
            
            // Calculate complex value
            double complex z = re + im * I;
            double complex result = evaluate_function(z, func_type);
            
            // Get magnitude and phase
            double magnitude = cabs(result);
            double phase = carg(result);
            
            // Use phase for hue and apply brightness based on magnitude
            Color color = phase_to_color(phase);
            color = apply_brightness(color, magnitude, params.enhanced_contrast);
            
            // Add phase lines (isochromatic lines) if enabled
            if (params.show_phase_lines) {
                color = add_phase_lines(color, phase, params.line_thickness);
            }
            
            // Add modulus lines (level curves) if enabled
            if (params.show_modulus_lines) {
                color = add_modulus_lines(color, magnitude, params.line_thickness);
            }
            
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

// Draw a magnitude legend
void draw_magnitude_legend() {
    // Draw the legend background
    DrawRectangle(SCREEN_WIDTH - 210, 60, 100, 150, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - 210, 60, 100, 150, BLACK);
    
    // Draw the title
    DrawText("Magnitude", SCREEN_WIDTH - 200, 65, 18, BLACK);
    
    // Draw brightness gradient
    for (int i = 0; i < 120; i++) {
        // Convert position to magnitude (0 to 5)
        double magnitude = 5.0 * (120.0 - i) / 120.0;
        Color color = WHITE;
        color = apply_brightness(color, magnitude, false);
        
        // Draw a line segment with this brightness
        DrawLine(SCREEN_WIDTH - 200, 90 + i, SCREEN_WIDTH - 130, 90 + i, color);
    }
    
    // Label the ends
    DrawText("0", SCREEN_WIDTH - 200, 85 + 120, 16, BLACK);
    DrawText("5+", SCREEN_WIDTH - 150, 85 + 120, 16, BLACK);
}

int main(void) {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Complex Domain Coloring");
    SetTargetFPS(60);
    
    // Create base image
    Image colorImage = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(colorImage);
    
    // Domain parameters
    double scale = 100.0;  // Larger scale to see more detail initially
    double centerX = 0.0;
    double centerY = 0.0;
    FunctionType current_function = FUNC_EXP;
    
    // Coloring parameters
    ColoringParams coloring_params = {
        .show_phase_lines = true,
        .show_modulus_lines = true,
        .enhanced_contrast = true,
        .line_thickness = 0.05f
    };
    
    // Initial render
    Color *pixels = LoadImageColors(colorImage);
    render_domain_coloring(pixels, current_function, centerX, centerY, scale, coloring_params);
    UpdateTexture(texture, pixels);
    UnloadImageColors(pixels);
    
    // UI elements
    Rectangle functionButton = { 10, SCREEN_HEIGHT - 70, 240, 30 };
    Rectangle phaseLineButton = { 10, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle modulusLineButton = { 170, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle contrastButton = { 330, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle resetButton = { 490, SCREEN_HEIGHT - 110, 150, 30 };
    
    // Main game loop
    while (!WindowShouldClose()) {
        bool needsUpdate = false;
        
        // Handle mouse drag for panning
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && GetMouseY() > 20 && GetMouseY() < SCREEN_HEIGHT - 120) {
            Vector2 delta = GetMouseDelta();
            centerX -= delta.x / scale;
            centerY += delta.y / scale;
            needsUpdate = true;
        }
        
        // Handle zooming
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scale *= (wheel > 0) ? 1.2 : 0.8;
            needsUpdate = true;
        }
        
        // Handle function button click
        if (CheckCollisionPointRec(GetMousePosition(), functionButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            current_function = (current_function + 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        
        // Handle phase lines button click
        if (CheckCollisionPointRec(GetMousePosition(), phaseLineButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.show_phase_lines = !coloring_params.show_phase_lines;
            needsUpdate = true;
        }
        
        // Handle modulus lines button click
        if (CheckCollisionPointRec(GetMousePosition(), modulusLineButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.show_modulus_lines = !coloring_params.show_modulus_lines;
            needsUpdate = true;
        }
        
        // Handle contrast button click
        if (CheckCollisionPointRec(GetMousePosition(), contrastButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.enhanced_contrast = !coloring_params.enhanced_contrast;
            needsUpdate = true;
        }
        
        // Handle reset button click
        if (CheckCollisionPointRec(GetMousePosition(), resetButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            centerX = 0.0;
            centerY = 0.0;
            scale = 100.0;
            coloring_params.show_phase_lines = true;
            coloring_params.show_modulus_lines = true;
            coloring_params.enhanced_contrast = true;
            needsUpdate = true;
        }
        
        // Change function with left/right arrows
        if (IsKeyPressed(KEY_RIGHT)) {
            current_function = (current_function + 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_LEFT)) {
            current_function = (current_function + FUNC_COUNT - 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        
        // Toggle phase lines with 'P'
        if (IsKeyPressed(KEY_P)) {
            coloring_params.show_phase_lines = !coloring_params.show_phase_lines;
            needsUpdate = true;
        }
        
        // Toggle modulus lines with 'M'
        if (IsKeyPressed(KEY_M)) {
            coloring_params.show_modulus_lines = !coloring_params.show_modulus_lines;
            needsUpdate = true;
        }
        
        // Toggle enhanced contrast with 'C'
        if (IsKeyPressed(KEY_C)) {
            coloring_params.enhanced_contrast = !coloring_params.enhanced_contrast;
            needsUpdate = true;
        }
        
        // Update rendering if needed
        if (needsUpdate) {
            pixels = LoadImageColors(colorImage);
            render_domain_coloring(pixels, current_function, centerX, centerY, scale, coloring_params);
            UpdateTexture(texture, pixels);
            UnloadImageColors(pixels);
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            
            // Draw current info
            DrawText(TextFormat("Scale: %.2f", scale), 10, 10, 20, BLACK);
            DrawText(TextFormat("Center: (%.2f, %.2f)", centerX, centerY), 10, 40, 20, BLACK);
            
            // Draw legends
            draw_phase_legend();
            draw_magnitude_legend();
            
            // Draw UI buttons
            DrawRectangleRec(functionButton, LIGHTGRAY);
            DrawText(TextFormat("Function: %s", function_names[current_function]), 
                     functionButton.x + 10, functionButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(phaseLineButton, coloring_params.show_phase_lines ? SKYBLUE : LIGHTGRAY);
            DrawText("Phase Lines", phaseLineButton.x + 10, phaseLineButton.y + 5, 20, BLACK);
                     
            DrawRectangleRec(modulusLineButton, coloring_params.show_modulus_lines ? SKYBLUE : LIGHTGRAY);
            DrawText("Modulus Lines", modulusLineButton.x + 10, modulusLineButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(contrastButton, coloring_params.enhanced_contrast ? SKYBLUE : LIGHTGRAY);
            DrawText("Enhanced Contrast", contrastButton.x + 10, contrastButton.y + 5, 18, BLACK);
            
            DrawRectangleRec(resetButton, LIGHTGRAY);
            DrawText("Reset View", resetButton.x + 30, resetButton.y + 5, 20, BLACK);
            
            // Draw help
            DrawText("Left/Right arrows: change function", 10, SCREEN_HEIGHT - 150, 16, DARKGRAY);
            DrawText("P: toggle phase lines, M: toggle modulus lines", 10, SCREEN_HEIGHT - 170, 16, DARKGRAY);
            DrawText("C: toggle enhanced contrast", 10, SCREEN_HEIGHT - 190, 16, DARKGRAY);
            DrawText("Mouse drag: pan view", 10, SCREEN_HEIGHT - 210, 16, DARKGRAY);
            DrawText("Mouse wheel: zoom in/out", 10, SCREEN_HEIGHT - 230, 16, DARKGRAY);
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadTexture(texture);
    UnloadImage(colorImage);
    CloseWindow();
    
    return 0;
}