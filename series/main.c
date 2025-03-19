#include "raylib.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800
#define MAX_TERMS 20

// Function types for Taylor/Laurent series
typedef enum {
    FUNC_EXP,
    FUNC_SIN,
    FUNC_LOG,
    FUNC_INVERSE,
    FUNC_COUNT
} FunctionType;

typedef enum {
    SERIES_TAYLOR,
    SERIES_LAURENT
} SeriesType;

typedef enum {
    VIEW_ORIGINAL,
    VIEW_APPROXIMATION,
    VIEW_ERROR,
    VIEW_SPLIT
} ViewMode;

// Function names
const char* function_names[] = {
    "e^z",
    "sin(z)",
    "log(z)",
    "1/z",
};

// Parameters for visualization
typedef struct {
    double centerX;
    double centerY;
    double scale;
    int num_terms;
    FunctionType func_type;
    SeriesType series_type;
    ViewMode view_mode;
    bool show_phase_lines;
    bool show_modulus_lines;
    float line_thickness;
} VisualizationParams;

// Utility function to clamp float values
float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Original functions
double complex eval_original_function(double complex z, FunctionType type, bool *error) {
    *error = false;
    
    // Check for NaN or Inf
    if (isnan(creal(z)) || isnan(cimag(z)) || isinf(creal(z)) || isinf(cimag(z))) {
        *error = true;
        return 0.0;
    }
    
    switch(type) {
        case FUNC_EXP:
            if (creal(z) > 700.0) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            return cexp(z);
            
        case FUNC_SIN:
            return csin(z);
            
        case FUNC_LOG:
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            return clog(z);
            
        case FUNC_INVERSE:
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            return 1.0 / z;
            
        default:
            return z;
    }
}

// Taylor series approximation
double complex eval_taylor_series(double complex z, FunctionType type, int terms, bool *error) {
    *error = false;
    
    // Check for NaN or Inf
    if (isnan(creal(z)) || isnan(cimag(z)) || isinf(creal(z)) || isinf(cimag(z))) {
        *error = true;
        return 0.0;
    }
    
    // Expansion point (usually 0 for Taylor series)
    double complex z0 = 0.0 + 0.0*I;
    double complex sum = 0.0 + 0.0*I;
    
    switch(type) {
        case FUNC_EXP:
            // e^z = sum(z^n / n!)
            sum = 0;
            double complex z_power = 1.0 + 0.0*I;
            double factorial = 1.0;
            
            for (int n = 0; n <= terms; n++) {
                sum += z_power / factorial;
                z_power *= z;
                factorial *= (n + 1);
                
                if (cabs(z_power) > 1e100) {
                    *error = true;
                    return sum;
                }
            }
            return sum;
            
        case FUNC_SIN:
            // sin(z) = sum((-1)^n * z^(2n+1) / (2n+1)!)
            sum = 0;
            for (int n = 0; n <= terms; n++) {
                double complex term = cpow(z, 2*n + 1);
                if (n % 2 == 1) term = -term;
                
                double denom = 1.0;
                for (int k = 1; k <= 2*n + 1; k++) {
                    denom *= k;
                }
                
                sum += term / denom;
                
                if (cabs(term) > 1e100) {
                    *error = true;
                    return sum;
                }
            }
            return sum;
            
        case FUNC_LOG:
            // log(z) around z0=1: sum(((-1)^(n+1)/n) * (z-1)^n)
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            
            // For log, use expansion around z0=1
            z0 = 1.0 + 0.0*I;
            double complex w = z - z0;
            
            sum = 0;
            for (int n = 1; n <= terms; n++) {
                double sign = (n % 2 == 1) ? 1.0 : -1.0;
                double complex term = sign * cpow(w, n) / n;
                sum += term;
                
                if (cabs(term) > 1e100) {
                    *error = true;
                    return sum;
                }
            }
            return sum;
            
        case FUNC_INVERSE:
            // Laurent series for 1/z around z=0: sum((-1)^n * z^(-n-1))
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            
            // For 1/z, we need a Laurent series with negative powers
            // We'll just return 1/z directly for the "Taylor" approximation
            return 1.0 / z;
            
        default:
            return z;
    }
}

// Laurent series approximation
double complex eval_laurent_series(double complex z, FunctionType type, int terms, bool *error) {
    *error = false;
    
    // Check for NaN or Inf
    if (isnan(creal(z)) || isnan(cimag(z)) || isinf(creal(z)) || isinf(cimag(z))) {
        *error = true;
        return 0.0;
    }
    
    // For most functions, Laurent is same as Taylor around regular points
    if (type != FUNC_INVERSE && type != FUNC_LOG) {
        return eval_taylor_series(z, type, terms, error);
    }
    
    double complex sum = 0.0 + 0.0*I;
    
    switch(type) {
        case FUNC_LOG:
            // Laurent series for log(z) around z=0:
            // log(z) = log|z| + i*arg(z) = log(r) + i*theta
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            
            sum = clog(z);  // Just use the exact value for now
            return sum;
        
        case FUNC_INVERSE:
            // Laurent series for 1/z around z=0: z^(-1)
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            
            // For 1/z, the Laurent series is exactly 1/z with one term
            return 1.0 / z;
            
        default:
            return eval_taylor_series(z, type, terms, error);
    }
}

// Simplified phase to color mapping using HSV
Color phase_to_color_hsv(double phase, float saturation, float value) {
    // Normalize phase to 0-360 degrees for hue
    float hue = (float)(fmod(phase + M_PI, 2*M_PI) * 180.0 / M_PI);
    
    // Rainbow spectrum
    return ColorFromHSV(hue, saturation, value);
}

// Apply brightness based on magnitude
Color apply_brightness(Color color, double magnitude, float contrast_strength) {
    // Scale brightness by magnitude using log scale
    float brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude * contrast_strength)));
    brightness = Clamp(brightness, 0.0f, 1.0f);
    
    // Apply brightness while preserving color relationships
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

// Get color for error visualization
Color get_error_color(double error, float max_error) {
    float ratio = Clamp(error / max_error, 0.0f, 1.0f);
    
    // Use a gradient: blue (low error) -> green -> yellow -> red (high error)
    if (ratio < 0.25f) {
        // Blue to cyan
        float t = ratio * 4.0f;
        return (Color){ 0, 255 * t, 255, 255 };
    } else if (ratio < 0.5f) {
        // Cyan to green
        float t = (ratio - 0.25f) * 4.0f;
        return (Color){ 0, 255, 255 * (1.0f - t), 255 };
    } else if (ratio < 0.75f) {
        // Green to yellow
        float t = (ratio - 0.5f) * 4.0f;
        return (Color){ 255 * t, 255, 0, 255 };
    } else {
        // Yellow to red
        float t = (ratio - 0.75f) * 4.0f;
        return (Color){ 255, 255 * (1.0f - t), 0, 255 };
    }
}

// Render a function visualization using domain coloring
void render_function(Color *pixels, double complex (*eval_func)(double complex, FunctionType, int, bool*), 
                     VisualizationParams params, int width, int height, int offset_x) {
    float saturation = 0.9f;
    float value = 1.0f;
    float contrast_strength = 1.0f;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Convert pixel to complex plane coordinates
            double re = ((x - width/2) / params.scale) + params.centerX;
            double im = ((height/2 - y) / params.scale) + params.centerY;
            
            // Calculate complex value with error handling
            double complex z = re + im * I;
            bool eval_error = false;
            double complex result = eval_func(z, params.func_type, params.num_terms, &eval_error);
            
            // Set output pixel
            Color color;
            if (eval_error) {
                color = (Color){ 255, 0, 255, 255 }; // Magenta for errors
            } else {
                double magnitude = cabs(result);
                double phase = carg(result);
                
                // Apply color mapping
                color = phase_to_color_hsv(phase, saturation, value);
                color = apply_brightness(color, magnitude, contrast_strength);
                
                // Add phase lines if enabled
                if (params.show_phase_lines) {
                    color = add_phase_lines(color, phase, params.line_thickness);
                }
                
                // Add modulus lines if enabled
                if (params.show_modulus_lines) {
                    color = add_modulus_lines(color, magnitude, params.line_thickness);
                }
            }
            
            // Set pixel in the destination buffer
            pixels[(y * SCREEN_WIDTH) + (x + offset_x)] = color;
        }
    }
}

// Render error visualization
void render_error(Color *pixels, VisualizationParams params, int width, int height, int offset_x) {
    float max_error = 5.0f; // Maximum error to visualize (higher errors will be capped)
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Convert pixel to complex plane coordinates
            double re = ((x - width/2) / params.scale) + params.centerX;
            double im = ((height/2 - y) / params.scale) + params.centerY;
            
            // Calculate complex value with error handling
            double complex z = re + im * I;
            bool eval_error1 = false;
            bool eval_error2 = false;
            
            // Get original function value
            double complex original = eval_original_function(z, params.func_type, &eval_error1);
            
            // Get series approximation
            double complex approximation;
            if (params.series_type == SERIES_TAYLOR) {
                approximation = eval_taylor_series(z, params.func_type, params.num_terms, &eval_error2);
            } else { // SERIES_LAURENT
                approximation = eval_laurent_series(z, params.func_type, params.num_terms, &eval_error2);
            }
            
            // Calculate error and set color
            Color color;
            if (eval_error1 || eval_error2) {
                color = (Color){ 255, 0, 255, 255 }; // Magenta for errors
            } else {
                double error = cabs(original - approximation);
                color = get_error_color(error, max_error);
            }
            
            // Set pixel in the destination buffer
            pixels[(y * SCREEN_WIDTH) + (x + offset_x)] = color;
        }
    }
}

// Draw legends for error visualization
void draw_error_legend(int x, int y) {
    // Draw the legend background
    DrawRectangle(x, y, 220, 40, WHITE);
    DrawRectangleLines(x, y, 220, 40, BLACK);
    
    // Draw the gradient
    for (int i = 0; i < 200; i++) {
        Color color = get_error_color(i / 200.0 * 5.0, 5.0);
        DrawLine(x + 10 + i, y + 15, x + 10 + i, y + 30, color);
    }
    
    // Draw labels
    DrawText("0", x + 10, y + 32, 10, BLACK);
    DrawText("Error", x + 100, y + 3, 15, BLACK);
    DrawText("5+", x + 195, y + 32, 10, BLACK);
}

int main(void) {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Complex Series Visualization");
    SetTargetFPS(60);
    
    // Create image for rendering
    Image colorImage = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(colorImage);
    
    // Set initial parameters
    VisualizationParams params = {
        .centerX = 0.0,
        .centerY = 0.0,
        .scale = 100.0,
        .num_terms = 5,
        .func_type = FUNC_EXP,
        .series_type = SERIES_TAYLOR,
        .view_mode = VIEW_SPLIT,
        .show_phase_lines = true,
        .show_modulus_lines = true,
        .line_thickness = 0.05f
    };
    
    // UI elements
    Rectangle termButton = { 10, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle functionButton = { 170, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle seriesTypeButton = { 330, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle viewModeButton = { 490, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle phaseLineButton = { 10, SCREEN_HEIGHT - 70, 150, 30 };
    Rectangle modulusLineButton = { 170, SCREEN_HEIGHT - 70, 150, 30 };
    Rectangle resetButton = { 490, SCREEN_HEIGHT - 70, 150, 30 };
    
    Color *pixels = LoadImageColors(colorImage);
    if (!pixels) {
        printf("Error: Failed to load image colors\n");
        UnloadImage(colorImage);
        CloseWindow();
        return 1;
    }
    
    // Initial render
    if (params.view_mode == VIEW_SPLIT) {
        // Left side: original function
        render_function(pixels, eval_original_function, params, SCREEN_WIDTH/2, SCREEN_HEIGHT, 0);
        
        // Right side: series approximation or error
        if (params.series_type == SERIES_TAYLOR) {
            render_function(pixels, eval_taylor_series, params, SCREEN_WIDTH/2, SCREEN_HEIGHT, SCREEN_WIDTH/2);
        } else {
            render_function(pixels, eval_laurent_series, params, SCREEN_WIDTH/2, SCREEN_HEIGHT, SCREEN_WIDTH/2);
        }
    } else if (params.view_mode == VIEW_ERROR) {
        render_error(pixels, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    } else if (params.view_mode == VIEW_ORIGINAL) {
        render_function(pixels, eval_original_function, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    } else { // VIEW_APPROXIMATION
        if (params.series_type == SERIES_TAYLOR) {
            render_function(pixels, eval_taylor_series, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        } else {
            render_function(pixels, eval_laurent_series, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        }
    }
    
    UpdateTexture(texture, pixels);
    
    // Main game loop
    while (!WindowShouldClose()) {
        bool needsUpdate = false;
        
        // Handle mouse drag for panning
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && GetMouseY() < SCREEN_HEIGHT - 120) {
            Vector2 delta = GetMouseDelta();
            params.centerX -= delta.x / params.scale;
            params.centerY += delta.y / params.scale;
            needsUpdate = true;
        }
        
        // Handle zooming
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            params.scale *= (wheel > 0) ? 1.2 : 0.8;
            needsUpdate = true;
        }
        
        // Handle buttons
        if (CheckCollisionPointRec(GetMousePosition(), termButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.num_terms = (params.num_terms % MAX_TERMS) + 1; // Cycle through 1-MAX_TERMS
            needsUpdate = true;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), functionButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.func_type = (params.func_type + 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), seriesTypeButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.series_type = (params.series_type == SERIES_TAYLOR) ? SERIES_LAURENT : SERIES_TAYLOR;
            needsUpdate = true;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), viewModeButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.view_mode = (params.view_mode + 1) % 4; // Cycle through view modes
            needsUpdate = true;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), phaseLineButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.show_phase_lines = !params.show_phase_lines;
            needsUpdate = true;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), modulusLineButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.show_modulus_lines = !params.show_modulus_lines;
            needsUpdate = true;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), resetButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            params.centerX = 0.0;
            params.centerY = 0.0;
            params.scale = 100.0;
            needsUpdate = true;
        }
        
        // Handle keyboard shortcuts
        if (IsKeyPressed(KEY_UP)) {
            if (params.num_terms < MAX_TERMS) params.num_terms++;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_DOWN)) {
            if (params.num_terms > 1) params.num_terms--;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
            params.func_type = (params.func_type + (IsKeyPressed(KEY_RIGHT) ? 1 : FUNC_COUNT - 1)) % FUNC_COUNT;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_T)) {
            params.series_type = SERIES_TAYLOR;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_L)) {
            params.series_type = SERIES_LAURENT;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_V)) {
            params.view_mode = (params.view_mode + 1) % 4;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_P)) {
            params.show_phase_lines = !params.show_phase_lines;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_M)) {
            params.show_modulus_lines = !params.show_modulus_lines;
            needsUpdate = true;
        }
        
        if (IsKeyPressed(KEY_R)) {
            params.centerX = 0.0;
            params.centerY = 0.0;
            params.scale = 100.0;
            needsUpdate = true;
        }
        
        // Update rendering if needed
        if (needsUpdate) {
            // Clear previous render
            memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Color));
            
            if (params.view_mode == VIEW_SPLIT) {
                // Left side: original function
                render_function(pixels, eval_original_function, params, SCREEN_WIDTH/2, SCREEN_HEIGHT, 0);
                
                // Right side: series approximation
                if (params.series_type == SERIES_TAYLOR) {
                    render_function(pixels, eval_taylor_series, params, SCREEN_WIDTH/2, SCREEN_HEIGHT, SCREEN_WIDTH/2);
                } else {
                    render_function(pixels, eval_laurent_series, params, SCREEN_WIDTH/2, SCREEN_HEIGHT, SCREEN_WIDTH/2);
                }
            } else if (params.view_mode == VIEW_ERROR) {
                render_error(pixels, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
            } else if (params.view_mode == VIEW_ORIGINAL) {
                render_function(pixels, eval_original_function, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
            } else { // VIEW_APPROXIMATION
                if (params.series_type == SERIES_TAYLOR) {
                    render_function(pixels, eval_taylor_series, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
                } else {
                    render_function(pixels, eval_laurent_series, params, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
                }
            }
            
            UpdateTexture(texture, pixels);
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            
            // Draw UI labels based on view mode
            if (params.view_mode == VIEW_SPLIT) {
                DrawText("Original Function", 10, 10, 20, WHITE);
                DrawText(params.series_type == SERIES_TAYLOR ? "Taylor Approximation" : "Laurent Series", SCREEN_WIDTH/2 + 10, 10, 20, WHITE);
                
                // Draw a dividing line
                DrawLine(SCREEN_WIDTH/2, 0, SCREEN_WIDTH/2, SCREEN_HEIGHT - 120, WHITE);
            } else if (params.view_mode == VIEW_ERROR) {
                DrawText("Error Magnitude", 10, 10, 20, WHITE);
                draw_error_legend(SCREEN_WIDTH - 240, 10);
            } else if (params.view_mode == VIEW_ORIGINAL) {
                DrawText("Original Function", 10, 10, 20, WHITE);
            } else { // VIEW_APPROXIMATION
                DrawText(params.series_type == SERIES_TAYLOR ? "Taylor Approximation" : "Laurent Series", 10, 10, 20, WHITE);
            }
            
            // Always display function and terms
            DrawText(TextFormat("Function: %s   Terms: %d", function_names[params.func_type], params.num_terms), 
                     10, 40, 20, WHITE);
            
            // Draw UI buttons
            DrawRectangleRec(termButton, LIGHTGRAY);
            DrawText(TextFormat("Terms: %d/%d", params.num_terms, MAX_TERMS), termButton.x + 10, termButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(functionButton, LIGHTGRAY);
            DrawText(TextFormat("Function: %s", function_names[params.func_type]), functionButton.x + 10, functionButton.y + 5, 14, BLACK);
            
            DrawRectangleRec(seriesTypeButton, LIGHTGRAY);
            DrawText(TextFormat("Series: %s", params.series_type == SERIES_TAYLOR ? "Taylor" : "Laurent"), 
                     seriesTypeButton.x + 10, seriesTypeButton.y + 5, 20, BLACK);
                     
            DrawRectangleRec(viewModeButton, LIGHTGRAY);
            const char* viewNames[] = {"Original", "Approximation", "Error", "Split"};
            DrawText(TextFormat("View: %s", viewNames[params.view_mode]), viewModeButton.x + 10, viewModeButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(phaseLineButton, params.show_phase_lines ? SKYBLUE : LIGHTGRAY);
            DrawText("Phase Lines", phaseLineButton.x + 10, phaseLineButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(modulusLineButton, params.show_modulus_lines ? SKYBLUE : LIGHTGRAY);
            DrawText("Modulus Lines", modulusLineButton.x + 10, modulusLineButton.y + 5, 20, BLACK);
            
            DrawRectangleRec(resetButton, LIGHTGRAY);
            DrawText("Reset View", resetButton.x + 30, resetButton.y + 5, 20, BLACK);
            
            // Draw help
            DrawText("Up/Down: Change terms, Left/Right: Change function", 10, SCREEN_HEIGHT - 150, 16, DARKGRAY);
            DrawText("T: Taylor series, L: Laurent series, V: Change view", 10, SCREEN_HEIGHT - 170, 16, DARKGRAY);
            DrawText("P: Toggle phase lines, M: Toggle modulus lines, R: Reset view", 10, SCREEN_HEIGHT - 190, 16, DARKGRAY);
            DrawText("Mouse drag: pan view, Mouse wheel: zoom in/out", 10, SCREEN_HEIGHT - 210, 16, DARKGRAY);
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadImageColors(pixels);
    UnloadTexture(texture);
    UnloadImage(colorImage);
    CloseWindow();
    
    return 0;
}
