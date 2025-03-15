#include "raylib.h"
#include "complex.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

// Utility function to clamp float values between min and max
float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

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

// Color scheme types
typedef enum {
    SCHEME_RAINBOW,
    SCHEME_THERMAL,
    SCHEME_GRAYSCALE,
    SCHEME_BLUEWHITE,
    SCHEME_PURPLEGOLD,
    SCHEME_COUNT // Keep last for counting
} ColorScheme;

// Color scheme names for UI
const char* color_scheme_names[] = {
    "Rainbow",
    "Thermal",
    "Grayscale",
    "Blue-White",
    "Purple-Gold"
};

// Domain coloring parameters with extended color mapping options
typedef struct {
    bool show_phase_lines;
    bool show_modulus_lines;
    bool enhanced_contrast;
    float line_thickness;
    ColorScheme color_scheme;
    float saturation;       // 0.0-1.0
    float value;            // 0.0-1.0
    float contrast_strength; // Magnitude contrast multiplier
    int anti_aliasing;      // Anti-aliasing level (1=none, 2=2x2, 4=4x4)
} ColoringParams;

// Improved phase to color mapping using HSV
Color phase_to_color_hsv(double phase, ColorScheme scheme, float saturation, float value) {
    // Normalize phase to 0-360 degrees for hue
    float hue = (float)(fmod(phase + M_PI, 2*M_PI) * 180.0 / M_PI);
    
    // Adjust color mapping based on scheme
    switch(scheme) {
        case SCHEME_RAINBOW:
            // Full rainbow spectrum (default)
            return ColorFromHSV(hue, saturation, value);
            
        case SCHEME_THERMAL:
            // Red-Yellow-White thermal gradient
            // Map hue from 0-60 (red to yellow)
            hue = fmodf(hue * 1/6 + 360, 60);
            return ColorFromHSV(hue, saturation, value);
            
        case SCHEME_GRAYSCALE: {
            // Pure grayscale using only value
            float grayValue = value * (0.5 + 0.5 * sin(phase));
            return ColorFromHSV(0, 0, grayValue);
        }
            
        case SCHEME_BLUEWHITE: {
            // Cool blue gradient
            hue = 210; // Blue base
            // Vary saturation based on phase
            float blueSat = saturation * (0.5 + 0.5 * sin(phase));
            return ColorFromHSV(hue, blueSat, value);
        }
            
        case SCHEME_PURPLEGOLD: {
            // Purple to gold gradient
            // Map between purple (270) and gold (45)
            float mappedHue = 270 + (phase + M_PI) * (45 - 270) / (2 * M_PI);
            if (mappedHue < 0) mappedHue += 360;
            return ColorFromHSV(mappedHue, saturation, value);
        }
            
        default:
            return ColorFromHSV(hue, saturation, value);
    }
}

// Apply brightness based on magnitude with improved controls
Color apply_brightness(Color color, double magnitude, bool enhanced_contrast, float contrast_strength) {
    // Scale brightness by magnitude using log scale with adjustable strength
    float brightness;
    
    if (enhanced_contrast) {
        // More pronounced contrast for visualizing magnitude changes
        brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude * contrast_strength)));
        brightness = powf(brightness, 0.8); // Enhance contrast
    } else {
        brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude)));
    }
    
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

// Application status for error handling
typedef enum {
    STATUS_OK,
    STATUS_MEMORY_ERROR,
    STATUS_MATH_ERROR,
    STATUS_RENDER_ERROR
} AppStatus;

// Global status message
typedef struct {
    AppStatus status;
    char message[256];
    float display_time; // How long to show the message
    bool active;
} StatusMessage;

// Safely evaluate complex function with error handling
double complex evaluate_function(double complex z, FunctionType type, bool *error) {
    *error = false;
    
    // First check for potential issues
    if (isnan(creal(z)) || isnan(cimag(z)) || isinf(creal(z)) || isinf(cimag(z))) {
        *error = true;
        return 0.0;
    }

    switch(type) {
        case FUNC_EXP:
            // Check for overflow in exp
            if (creal(z) > 700.0) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            return cexp(z);
            
        case FUNC_SIN:
            return csin(z);
            
        case FUNC_TAN: {
            // Check for singularities in tan
            double complex sin_z = csin(z);
            double complex cos_z = ccos(z);
            if (cabs(cos_z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I;
            }
            return sin_z / cos_z;
        }
            
        case FUNC_INVERSE:
            if (cabs(z) < 1e-10) {
                *error = true;
                return HUGE_VAL + HUGE_VAL * I; // Indicate pole
            }
            return 1.0 / z;
            
        case FUNC_SQUARE:
            return z * z;
            
        case FUNC_SQUARE_MINUS_ONE:
            return z * z - 1.0;
            
        case FUNC_POLY5_MINUS_Z: {
            // Use direct calculation for better numerical stability
            double complex z2 = z * z;
            double complex z4 = z2 * z2;
            return z4 * z - z;
        }
            
        default:
            return z; // Identity function as fallback
    }
}

// Render the domain coloring with improved color mapping and anti-aliasing
// Returns true if successful, false on error
bool render_domain_coloring(Color *pixels, FunctionType func_type, double centerX, double centerY, 
                           double scale, ColoringParams params, StatusMessage *status) {
    if (pixels == NULL) {
        if (status) {
            status->status = STATUS_RENDER_ERROR;
            snprintf(status->message, sizeof(status->message), "Render error: NULL pixel buffer");
            status->active = true;
            status->display_time = 5.0f; // Show for 5 seconds
        }
        return false;
    }
    
    // Default HSV parameters if not specified
    float saturation = params.saturation > 0 ? params.saturation : 0.9f;
    float baseValue = params.value > 0 ? params.value : 1.0f;
    float contrastStrength = params.contrast_strength > 0 ? params.contrast_strength : 1.0f;
    int aa_level = params.anti_aliasing > 0 ? params.anti_aliasing : 1;
    
    // Calculate pixel coordinates once per pixel for better performance
    double pixel_width = 1.0 / scale;
    
    // Count errors to avoid excessive error messages
    int error_count = 0;
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float r = 0, g = 0, b = 0;
            int valid_samples = 0;
            
            // Super-sampling for anti-aliasing
            for (int sy = 0; sy < aa_level; sy++) {
                for (int sx = 0; sx < aa_level; sx++) {
                    // Calculate sub-pixel position
                    double sub_x = (double)sx / aa_level;
                    double sub_y = (double)sy / aa_level;
                    
                    // Convert pixel to complex plane coordinates with sub-pixel offset
                    double re = ((x + sub_x) - SCREEN_WIDTH/2) / scale + centerX;
                    double im = ((SCREEN_HEIGHT/2 - y) - sub_y) / scale + centerY;
                    
                    // Calculate complex value with error handling
                    double complex z = re + im * I;
                    bool eval_error = false;
                    double complex result = evaluate_function(z, func_type, &eval_error);
                    
                    if (eval_error) {
                        error_count++;
                        continue; // Skip this sample
                    }
                    
                    // Get magnitude and phase
                    double magnitude = cabs(result);
                    double phase = carg(result);
                    
                    // Apply improved color mapping
                    Color color = phase_to_color_hsv(phase, params.color_scheme, saturation, baseValue);
                    color = apply_brightness(color, magnitude, params.enhanced_contrast, contrastStrength);
                    
                    // Add phase lines (isochromatic lines) if enabled
                    if (params.show_phase_lines) {
                        color = add_phase_lines(color, phase, params.line_thickness);
                    }
                    
                    // Add modulus lines (level curves) if enabled
                    if (params.show_modulus_lines) {
                        color = add_modulus_lines(color, magnitude, params.line_thickness);
                    }
                    
                    // Accumulate color components for averaging
                    r += color.r;
                    g += color.g;
                    b += color.b;
                    valid_samples++;
                }
            }
            
            // Average the color components if we have valid samples
            if (valid_samples > 0) {
                Color final_color = {
                    (unsigned char)(r / valid_samples),
                    (unsigned char)(g / valid_samples),
                    (unsigned char)(b / valid_samples),
                    255
                };
                pixels[y * SCREEN_WIDTH + x] = final_color;
            } else {
                // Mark error points as bright magenta
                pixels[y * SCREEN_WIDTH + x] = (Color){ 255, 0, 255, 255 };
            }
        }
    }
    
    // Set status message if many errors occurred
    if (error_count > 1000 && status) {
        status->status = STATUS_MATH_ERROR;
        snprintf(status->message, sizeof(status->message), 
                "Mathematical errors at %d points - function may have poles or branch cuts in view",
                error_count);
        status->active = true;
        status->display_time = 5.0f;
    }
    
    return true;
}

// Draw an improved color legend showing the current color scheme
void draw_color_legend(ColorScheme scheme, float saturation, float value) {
    // Draw the legend background
    DrawRectangle(SCREEN_WIDTH - 100, 60, 80, 150, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - 100, 60, 80, 150, BLACK);
    
    // Draw the title
    DrawText("Phase", SCREEN_WIDTH - 90, 65, 18, BLACK);
    
    // Draw color gradient for the current scheme
    for (int i = 0; i < 120; i++) {
        // Convert position to phase (-π to π)
        double phase = M_PI * (2.0 * i / 120.0 - 1.0);
        Color color = phase_to_color_hsv(phase, scheme, saturation, value);
        
        // Draw a line segment with this color
        DrawLine(SCREEN_WIDTH - 90, 90 + i, SCREEN_WIDTH - 30, 90 + i, color);
    }
    
    // Label the ends
    DrawText("-π", SCREEN_WIDTH - 90, 85 + 120, 16, BLACK);
    DrawText("+π", SCREEN_WIDTH - 45, 85 + 120, 16, BLACK);
    
    // Show current scheme name
    DrawText(color_scheme_names[scheme], SCREEN_WIDTH - 90, 215, 12, BLACK);
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
        color = apply_brightness(color, magnitude, false, 1.0f);
        
        // Draw a line segment with this brightness
        DrawLine(SCREEN_WIDTH - 200, 90 + i, SCREEN_WIDTH - 130, 90 + i, color);
    }
    
    // Label the ends
    DrawText("0", SCREEN_WIDTH - 200, 85 + 120, 16, BLACK);
    DrawText("5+", SCREEN_WIDTH - 150, 85 + 120, 16, BLACK);
}

// UI function to handle color scheme changes - returns true if update needed
bool handle_color_scheme_button(Rectangle *colorSchemeButton, ColoringParams *params) {
    DrawRectangleRec(*colorSchemeButton, LIGHTGRAY);
    DrawText(TextFormat("Color: %s", color_scheme_names[params->color_scheme]), 
             colorSchemeButton->x + 10, colorSchemeButton->y + 5, 20, BLACK);
             
    // Handle color scheme button click
    if (CheckCollisionPointRec(GetMousePosition(), *colorSchemeButton) && 
        IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        params->color_scheme = (params->color_scheme + 1) % SCHEME_COUNT;
        return true; // Signal that we need to update the rendering
    }
    
    return false;
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
    
    // Extended coloring parameters with improved color mapping
    ColoringParams coloring_params = {
        .show_phase_lines = true,
        .show_modulus_lines = true,
        .enhanced_contrast = true,
        .line_thickness = 0.05f,
        .color_scheme = SCHEME_RAINBOW,
        .saturation = 0.9f,
        .value = 1.0f,
        .contrast_strength = 1.0f,
        .anti_aliasing = 1  // Default: no anti-aliasing (can be 1, 2, or 4)
    };
    
    // Status message for error handling
    StatusMessage status_message = {
        .status = STATUS_OK,
        .message = "",
        .display_time = 0.0f,
        .active = false
    };
    
    // Initial render with error handling
    Color *pixels = LoadImageColors(colorImage);
    if (pixels == NULL) {
        printf("Error: Failed to load image colors\n");
        UnloadImage(colorImage);
        CloseWindow();
        return 1;
    }
    
    if (!render_domain_coloring(pixels, current_function, centerX, centerY, scale, coloring_params, &status_message)) {
        printf("Error: Failed to render domain coloring\n");
        UnloadImageColors(pixels);
        UnloadImage(colorImage);
        CloseWindow();
        return 1;
    }
    
    UpdateTexture(texture, pixels);
    UnloadImageColors(pixels);
    
    // UI elements
    Rectangle functionButton = { 10, SCREEN_HEIGHT - 70, 240, 30 };
    Rectangle phaseLineButton = { 10, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle modulusLineButton = { 170, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle contrastButton = { 330, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle resetButton = { 490, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle colorSchemeButton = { 330, SCREEN_HEIGHT - 70, 240, 30 }; // New button for color scheme
    Rectangle antiAliasingButton = { 580, SCREEN_HEIGHT - 110, 170, 30 }; // Anti-aliasing toggle button
    
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
        
        // Handle color scheme button
        if (handle_color_scheme_button(&colorSchemeButton, &coloring_params)) {
            needsUpdate = true;
        }
        
        // Handle anti-aliasing button
        if (CheckCollisionPointRec(GetMousePosition(), antiAliasingButton) && 
            IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            // Cycle through anti-aliasing levels: 1 -> 2 -> 4 -> 1
            coloring_params.anti_aliasing = (coloring_params.anti_aliasing == 1) ? 2 : 
                                            (coloring_params.anti_aliasing == 2) ? 4 : 1;
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
            coloring_params.color_scheme = SCHEME_RAINBOW;
            coloring_params.anti_aliasing = 1; // Reset to no anti-aliasing
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
        
        // Toggle color scheme with 'S'
        if (IsKeyPressed(KEY_S)) {
            coloring_params.color_scheme = (coloring_params.color_scheme + 1) % SCHEME_COUNT;
            needsUpdate = true;
        }
        
        // Toggle anti-aliasing with 'A'
        if (IsKeyPressed(KEY_A)) {
            coloring_params.anti_aliasing = (coloring_params.anti_aliasing == 1) ? 2 : 
                                          (coloring_params.anti_aliasing == 2) ? 4 : 1;
            needsUpdate = true;
        }
        
        // Adjust saturation with '[' and ']'
        if (IsKeyPressed(KEY_LEFT_BRACKET)) {
            coloring_params.saturation = Clamp(coloring_params.saturation - 0.1f, 0.0f, 1.0f);
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            coloring_params.saturation = Clamp(coloring_params.saturation + 0.1f, 0.0f, 1.0f);
            needsUpdate = true;
        }
        
        // Adjust contrast strength with '-' and '='
        if (IsKeyPressed(KEY_MINUS)) {
            coloring_params.contrast_strength = Clamp(coloring_params.contrast_strength - 0.2f, 0.2f, 5.0f);
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_EQUAL)) {
            coloring_params.contrast_strength = Clamp(coloring_params.contrast_strength + 0.2f, 0.2f, 5.0f);
            needsUpdate = true;
        }
        
        // Update rendering if needed
        if (needsUpdate) {
            pixels = LoadImageColors(colorImage);
            if (pixels != NULL) {
                // Clear status message if it was showing a render error
                if (status_message.status == STATUS_RENDER_ERROR) {
                    status_message.active = false;
                }
                
                // Render with error handling
                if (!render_domain_coloring(pixels, current_function, centerX, centerY, scale, 
                                          coloring_params, &status_message)) {
                    status_message.status = STATUS_RENDER_ERROR;
                    snprintf(status_message.message, sizeof(status_message.message), 
                             "Error rendering - check function at current view");
                    status_message.active = true;
                    status_message.display_time = 5.0f;
                }
                
                UpdateTexture(texture, pixels);
                UnloadImageColors(pixels);
            } else {
                // Handle memory allocation error
                status_message.status = STATUS_MEMORY_ERROR;
                snprintf(status_message.message, sizeof(status_message.message), 
                         "Memory allocation error - try reducing quality settings");
                status_message.active = true;
                status_message.display_time = 5.0f;
            }
        }
        
        // Update status message timer
        if (status_message.active) {
            status_message.display_time -= GetFrameTime();
            if (status_message.display_time <= 0) {
                status_message.active = false;
            }
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            
            // Draw current info
            DrawText(TextFormat("Scale: %.2f", scale), 10, 10, 20, BLACK);
            DrawText(TextFormat("Center: (%.2f, %.2f)", centerX, centerY), 10, 40, 20, BLACK);
            
            // Draw legends with updated color scheme
            draw_color_legend(coloring_params.color_scheme, coloring_params.saturation, coloring_params.value);
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
            
            // Draw the color scheme button (handled by handle_color_scheme_button)
            handle_color_scheme_button(&colorSchemeButton, &coloring_params);
            
            // Draw anti-aliasing button
            DrawRectangleRec(antiAliasingButton, LIGHTGRAY);
            DrawText(TextFormat("AA: %dx", coloring_params.anti_aliasing), 
                     antiAliasingButton.x + 20, antiAliasingButton.y + 5, 20, BLACK);
            
            // Draw color adjustment info
            DrawText(TextFormat("Sat: %.1f", coloring_params.saturation), 580, SCREEN_HEIGHT - 70, 16, BLACK);
            DrawText(TextFormat("Contrast: %.1f", coloring_params.contrast_strength), 580, SCREEN_HEIGHT - 50, 16, BLACK);
            
            // Draw status message if active
            if (status_message.active) {
                // Choose color based on status type
                Color msgColor;
                switch(status_message.status) {
                    case STATUS_MEMORY_ERROR:
                        msgColor = RED;
                        break;
                    case STATUS_MATH_ERROR:
                        msgColor = ORANGE;
                        break;
                    case STATUS_RENDER_ERROR:
                        msgColor = RED;
                        break;
                    default:
                        msgColor = WHITE;
                }
                
                // Draw background
                DrawRectangle(SCREEN_WIDTH/2 - MeasureText(status_message.message, 20)/2 - 10,
                             10, MeasureText(status_message.message, 20) + 20, 40, Fade(BLACK, 0.7f));
                            
                // Draw message
                DrawText(status_message.message, 
                         SCREEN_WIDTH/2 - MeasureText(status_message.message, 20)/2,
                         20, 20, msgColor);
            }
            
            // Draw help
            DrawText("Left/Right arrows: change function", 10, SCREEN_HEIGHT - 150, 16, DARKGRAY);
            DrawText("P: toggle phase lines, M: toggle modulus lines", 10, SCREEN_HEIGHT - 170, 16, DARKGRAY);
            DrawText("C: toggle enhanced contrast, S: change color scheme", 10, SCREEN_HEIGHT - 190, 16, DARKGRAY);
            DrawText("[/]: adjust saturation, -/=: adjust contrast", 10, SCREEN_HEIGHT - 210, 16, DARKGRAY);
            DrawText("A: cycle anti-aliasing (1x→2x→4x→1x)", 10, SCREEN_HEIGHT - 230, 16, DARKGRAY);
            DrawText("Mouse drag: pan view, Mouse wheel: zoom in/out", 10, SCREEN_HEIGHT - 250, 16, DARKGRAY);
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadTexture(texture);
    UnloadImage(colorImage);
    CloseWindow();
    
    return 0;
}