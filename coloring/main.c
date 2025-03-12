#include "raylib.h"
#include "complex.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Screen dimensions
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define TEXT_INPUT_MAX 128

// Complex function types
typedef enum {
    FUNC_IDENTITY,
    FUNC_SQUARE,
    FUNC_CUBE,
    FUNC_SQUARE_MINUS_ONE,
    FUNC_INVERSE,
    FUNC_SIN,
    FUNC_EXP,
    FUNC_TAN,
    FUNC_POLY5_MINUS_Z,
    FUNC_RATIONAL,
    FUNC_CUSTOM
} FunctionType;

typedef struct {
    double complex (*func)(double complex, double);
    const char *name;
} ComplexFunction;

// Function prototypes
double complex identity(double complex z, double t);
double complex square(double complex z, double t);
double complex cube(double complex z, double t);
double complex square_minus_one(double complex z, double t);
double complex inverse(double complex z, double t);
double complex complex_sin(double complex z, double t);
double complex complex_exp(double complex z, double t);
double complex complex_tan(double complex z, double t);
double complex poly5_minus_z(double complex z, double t);
double complex rational(double complex z, double t);
double complex animated_func(double complex z, double t);

Color hsl_to_rgb(float h, float s, float l);
void render_domain_coloring(Image *image, ComplexFunction func, double centerX, double centerY, double scale, double time);

// Global variables
static FunctionType current_function = FUNC_SQUARE;
static char function_input[TEXT_INPUT_MAX] = "";
static bool is_editing_function = false;
static double centerX = 0.0;
static double centerY = 0.0;
static double scale = 100.0; // Pixels per unit in complex plane
static bool is_animating = false;
static double animation_time = 0.0;

// Complex functions
double complex identity(double complex z, double t) {
    return z;
}

double complex square(double complex z, double t) {
    return z * z;
}

double complex cube(double complex z, double t) {
    return z * z * z;
}

double complex square_minus_one(double complex z, double t) {
    return z * z - 1.0;
}

double complex inverse(double complex z, double t) {
    if (cabs(z) < 1e-10) return 0.0; // Avoid division by zero
    return 1.0 / z;
}

double complex complex_sin(double complex z, double t) {
    return csin(z);
}

double complex complex_exp(double complex z, double t) {
    return cexp(z);
}

double complex complex_tan(double complex z, double t) {
    return ctan(z);
}

double complex poly5_minus_z(double complex z, double t) {
    return cpow(z, 5) - z;
}

double complex rational(double complex z, double t) {
    double complex denom = z * z + z + 1.0;
    if (cabs(denom) < 1e-10) return 0.0; // Avoid division by zero
    return 1.0 / denom;
}

double complex animated_func(double complex z, double t) {
    // Example: Rotating function
    double complex rot = cexp(I * t);
    return z * z * rot;
}

// Available functions list
ComplexFunction functions[] = {
    { identity, "z" },
    { square, "z^2" },
    { cube, "z^3" },
    { square_minus_one, "z^2 - 1" },
    { inverse, "1/z" },
    { complex_sin, "sin(z)" },
    { complex_exp, "exp(z)" },
    { complex_tan, "tan(z)" },
    { poly5_minus_z, "z^5 - z" },
    { rational, "1/(z^2 + z + 1)" },
    { animated_func, "z^2 * e^(it)" }
};

// Convert HSL color to RGB
Color hsl_to_rgb(float h, float s, float l) {
    float r, g, b;
    
    if (s == 0) {
        r = g = b = l; // Achromatic
    } else {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        
        float hk = h / 360.0f;
        float tr = hk + 1.0f/3.0f;
        float tg = hk;
        float tb = hk - 1.0f/3.0f;
        
        if (tr < 0) tr += 1.0f;
        if (tr > 1) tr -= 1.0f;
        if (tg < 0) tg += 1.0f;
        if (tg > 1) tg -= 1.0f;
        if (tb < 0) tb += 1.0f;
        if (tb > 1) tb -= 1.0f;
        
        // Calculate RGB components
        r = tr < 1.0f/6.0f ? p + (q - p) * 6 * tr :
            tr < 1.0f/2.0f ? q :
            tr < 2.0f/3.0f ? p + (q - p) * (2.0f/3.0f - tr) * 6 :
            p;
            
        g = tg < 1.0f/6.0f ? p + (q - p) * 6 * tg :
            tg < 1.0f/2.0f ? q :
            tg < 2.0f/3.0f ? p + (q - p) * (2.0f/3.0f - tg) * 6 :
            p;
            
        b = tb < 1.0f/6.0f ? p + (q - p) * 6 * tb :
            tb < 1.0f/2.0f ? q :
            tb < 2.0f/3.0f ? p + (q - p) * (2.0f/3.0f - tb) * 6 :
            p;
    }
    
    return (Color){ (unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), 255 };
}

// Render domain coloring to image
void render_domain_coloring(Image *image, ComplexFunction func, double centerX, double centerY, double scale, double time) {
    Color *pixels = GetImageData(*image);
    
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            // Convert pixel coordinates to complex plane
            double re = (x - image->width / 2) / scale + centerX;
            double im = (image->height / 2 - y) / scale + centerY;
            
            // Compute the complex value
            double complex z = re + im * I;
            double complex result = func.func(z, time);
            
            // Extract magnitude and phase
            double magnitude = cabs(result);
            double phase = carg(result);
            
            // Convert magnitude to brightness using logarithmic scale
            double logMagnitude = log(1 + magnitude);
            double normalizedLogMagnitude = 1.0 - 1.0 / (1.0 + logMagnitude * 0.2);
            
            // Convert phase to hue (0-360)
            float hue = fmod((phase / M_PI + 1.0) * 180.0, 360.0);
            
            // Use HSL for coloring
            float saturation = 0.9f;
            float lightness = 0.5f * normalizedLogMagnitude;
            
            // Set pixel color
            pixels[y * image->width + x] = hsl_to_rgb(hue, saturation, lightness);
        }
    }
    
    UpdateTexture2D(LoadTextureFromImage(*image), pixels);
    free(pixels);
}

int main(void) {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Complex Domain Coloring");
    SetTargetFPS(60);
    
    // Create image for rendering
    Image colorImage = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(colorImage);
    
    // Create UI elements
    Rectangle functionButton = { 20, 20, 200, 30 };
    Rectangle animateButton = { 240, 20, 150, 30 };
    Rectangle resetButton = { 410, 20, 150, 30 };
    Rectangle textEditBox = { 20, 60, SCREEN_WIDTH - 40, 30 };
    
    // Render initial function
    render_domain_coloring(&colorImage, functions[current_function], centerX, centerY, scale, animation_time);
    UpdateTexture2D(texture, GetImageData(colorImage));
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        if (is_animating) {
            animation_time += GetFrameTime();
            if (animation_time > 2 * M_PI) animation_time -= 2 * M_PI;
            
            // Re-render with new time
            render_domain_coloring(&colorImage, functions[current_function], centerX, centerY, scale, animation_time);
            UpdateTexture2D(texture, GetImageData(colorImage));
        }
        
        // Handle mouse drag for panning
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !is_editing_function && 
            GetMouseY() > 100) { // Don't pan when clicking UI elements
            Vector2 delta = GetMouseDelta();
            centerX -= delta.x / scale;
            centerY += delta.y / scale;
            
            // Re-render
            render_domain_coloring(&colorImage, functions[current_function], centerX, centerY, scale, animation_time);
            UpdateTexture2D(texture, GetImageData(colorImage));
        }
        
        // Handle mouse wheel for zooming
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            double zoom_factor = wheel > 0 ? 1.1 : 0.9;
            scale *= zoom_factor;
            
            // Re-render
            render_domain_coloring(&colorImage, functions[current_function], centerX, centerY, scale, animation_time);
            UpdateTexture2D(texture, GetImageData(colorImage));
        }
        
        // Handle function button click
        if (CheckCollisionPointRec(GetMousePosition(), functionButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            current_function = (current_function + 1) % (sizeof(functions) / sizeof(functions[0]));
            
            // Re-render
            render_domain_coloring(&colorImage, functions[current_function], centerX, centerY, scale, animation_time);
            UpdateTexture2D(texture, GetImageData(colorImage));
        }
        
        // Handle animate button click
        if (CheckCollisionPointRec(GetMousePosition(), animateButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            is_animating = !is_animating;
        }
        
        // Handle reset button click
        if (CheckCollisionPointRec(GetMousePosition(), resetButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            centerX = 0.0;
            centerY = 0.0;
            scale = 100.0;
            animation_time = 0.0;
            
            // Re-render
            render_domain_coloring(&colorImage, functions[current_function], centerX, centerY, scale, animation_time);
            UpdateTexture2D(texture, GetImageData(colorImage));
        }
        
        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            
            // Draw the domain coloring
            DrawTexture(texture, 0, 0, WHITE);
            
            // Draw UI
            DrawRectangleRec(functionButton, LIGHTGRAY);
            DrawText(TextFormat("Function: %s", functions[current_function].name), 
                     functionButton.x + 10, functionButton.y + 5, 20, DARKGRAY);
            
            DrawRectangleRec(animateButton, LIGHTGRAY);
            DrawText(is_animating ? "Stop Animation" : "Start Animation", 
                     animateButton.x + 10, animateButton.y + 5, 20, DARKGRAY);
                     
            DrawRectangleRec(resetButton, LIGHTGRAY);
            DrawText("Reset View", resetButton.x + 10, resetButton.y + 5, 20, DARKGRAY);
            
            // Draw instructions
            DrawText("Left mouse drag to pan, mouse wheel to zoom", 20, SCREEN_HEIGHT - 40, 20, DARKGRAY);
            
            // Draw animation time if animating
            if (is_animating) {
                DrawText(TextFormat("t = %.2f", animation_time), 20, SCREEN_HEIGHT - 70, 20, DARKGRAY);
            }
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadTexture(texture);
    UnloadImage(colorImage);
    CloseWindow();
    
    return 0;
}