// domain coloring for complex functions; interactive controls for function, phase/modulus lines, and aa
#include "raylib.h"
#include "complex.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

static inline unsigned char lerp_byte(unsigned char a, unsigned char b, float t) {
    return (unsigned char)(a + (b - a) * t);
}

static inline float luminance(Color c) {
    return (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b) / 255.0f;
}

static inline Color blend_contrast_line(Color base, float alpha) {
    float lum = luminance(base);
    unsigned char target = (lum > 0.5f) ? 0 : 255;
    Color out = base;
    out.r = lerp_byte(base.r, target, alpha);
    out.g = lerp_byte(base.g, target, alpha);
    out.b = lerp_byte(base.b, target, alpha);
    return out;
}

float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

typedef enum {
    FUNC_EXP,
    FUNC_SIN,
    FUNC_TAN,
    FUNC_INVERSE,
    FUNC_SQUARE,
    FUNC_SQUARE_MINUS_ONE,
    FUNC_POLY5_MINUS_Z,
    FUNC_COUNT
} FunctionType;

const char* function_names[] = {
    "exp(z)",
    "sin(z)",
    "tan(z)",
    "1/z",
    "z^2",
    "z^2 - 1",
    "z^5 - z"
};

typedef struct {
    bool show_phase_lines;
    bool show_modulus_lines;
    bool enhanced_contrast;
    float line_thickness;
    float saturation;
    float value;
    float contrast_strength;
    int anti_aliasing;
} ColoringParams;

Color phase_to_color_hsv(double phase, float saturation, float value) {
    float hue = (float)(fmod(phase + M_PI, 2*M_PI) * 180.0 / M_PI);
    return ColorFromHSV(hue, saturation, value);
}

Color apply_brightness(Color color, double magnitude, bool enhanced_contrast, float contrast_strength) {
    float brightness;
    if (enhanced_contrast) {
        brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude * contrast_strength)));
        brightness = powf(brightness, 0.75f);
    } else {
        brightness = 0.5 * (1.0 - 1.0/(1.0 + log(1.0 + magnitude)));
    }
    brightness = Clamp(brightness, 0.0f, 1.0f);
    color.r = (unsigned char)(color.r * brightness);
    color.g = (unsigned char)(color.g * brightness);
    color.b = (unsigned char)(color.b * brightness);
    return color;
}

Color add_phase_lines(Color color, double phase, float thickness) {
    double phase_mod = fmod(phase + M_PI, M_PI/4);
    if (phase_mod < thickness || phase_mod > M_PI/4 - thickness) {
        color = blend_contrast_line(color, 0.35f);
    }
    return color;
}

Color add_modulus_lines(Color color, double magnitude, float thickness) {
    double log_mag = log(magnitude + 1.0);
    double mod = fmod(log_mag, 1.0);
    if (mod < thickness || mod > 1.0 - thickness) {
        color = blend_contrast_line(color, 0.35f);
    }
    return color;
}

typedef enum {
    STATUS_OK,
    STATUS_MEMORY_ERROR,
    STATUS_MATH_ERROR,
    STATUS_RENDER_ERROR
} AppStatus;

typedef struct {
    AppStatus status;
    char message[256];
    float display_time;
    bool active;
} StatusMessage;

double complex evaluate_function(double complex z, FunctionType type, bool *error) {
    *error = false;
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
            
        case FUNC_TAN: {
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
                return HUGE_VAL + HUGE_VAL * I;
            }
            return 1.0 / z;
            
        case FUNC_SQUARE:
            return z * z;
            
        case FUNC_SQUARE_MINUS_ONE:
            return z * z - 1.0;
            
        case FUNC_POLY5_MINUS_Z: {
            double complex z2 = z * z;
            double complex z4 = z2 * z2;
            return z4 * z - z;
        }
            
        default:
            return z;
    }
}

bool render_domain_coloring(Color *pixels, FunctionType func_type, double centerX, double centerY, 
                           double scale, ColoringParams params, StatusMessage *status) {
    if (pixels == NULL) {
        if (status) {
            status->status = STATUS_RENDER_ERROR;
            snprintf(status->message, sizeof(status->message), "Render error: NULL pixel buffer");
            status->active = true;
            status->display_time = 5.0f;
        }
        return false;
    }

    float saturation = params.saturation > 0 ? params.saturation : 0.85f;
    float baseValue = params.value > 0 ? params.value : 0.95f;
    float contrastStrength = params.contrast_strength > 0 ? params.contrast_strength : 1.0f;
    int aa_level = params.anti_aliasing > 0 ? params.anti_aliasing : 1;

    double pixel_width = 1.0 / scale;

    int error_count = 0;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float r = 0, g = 0, b = 0;
            int valid_samples = 0;
            // supersampling aa
            for (int sy = 0; sy < aa_level; sy++) {
                for (int sx = 0; sx < aa_level; sx++) {
                    double sub_x = (double)sx / aa_level;
                    double sub_y = (double)sy / aa_level;
                    double re = ((x + sub_x) - SCREEN_WIDTH/2) / scale + centerX;
                    double im = ((SCREEN_HEIGHT/2 - y) - sub_y) / scale + centerY;
                    double complex z = re + im * I;
                    bool eval_error = false;
                    double complex result = evaluate_function(z, func_type, &eval_error);
                    if (eval_error) {
                        error_count++;
                        continue;
                    }
                    double magnitude = cabs(result);
                    double phase = carg(result);
                    Color color = phase_to_color_hsv(phase, saturation, baseValue);
                    color = apply_brightness(color, magnitude, params.enhanced_contrast, contrastStrength);
                    if (params.show_phase_lines) {
                        color = add_phase_lines(color, phase, params.line_thickness);
                    }
                    if (params.show_modulus_lines) {
                        color = add_modulus_lines(color, magnitude, params.line_thickness);
                    }
                    r += color.r;
                    g += color.g;
                    b += color.b;
                    valid_samples++;
                }
            }
            if (valid_samples > 0) {
                Color final_color = {
                    (unsigned char)(r / valid_samples),
                    (unsigned char)(g / valid_samples),
                    (unsigned char)(b / valid_samples),
                    255
                };
                pixels[y * SCREEN_WIDTH + x] = final_color;
            } else {
                pixels[y * SCREEN_WIDTH + x] = (Color){ 255, 0, 255, 255 };
            }
        }
    }
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

void draw_color_legend(float saturation, float value) {
    DrawRectangle(SCREEN_WIDTH - 100, 60, 80, 190, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - 100, 60, 80, 190, BLACK);
    DrawText("Phase", SCREEN_WIDTH - 90, 65, 18, BLACK);
    for (int i = 0; i < 120; i++) {
        double phase = M_PI * (2.0 * i / 120.0 - 1.0);
        Color color = phase_to_color_hsv(phase, saturation, value);
        DrawLine(SCREEN_WIDTH - 90, 90 + i, SCREEN_WIDTH - 30, 90 + i, color);
    }
    DrawText("-π", SCREEN_WIDTH - 90, 85 + 120, 16, BLACK);
    DrawText("+π", SCREEN_WIDTH - 45, 85 + 120, 16, BLACK);
    DrawText("Rainbow", SCREEN_WIDTH - 90, 235, 12, BLACK);
}

void draw_magnitude_legend() {
    DrawRectangle(SCREEN_WIDTH - 210, 60, 100, 170, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - 210, 60, 100, 170, BLACK);
    DrawText("Magnitude", SCREEN_WIDTH - 200, 65, 18, BLACK);
    for (int i = 0; i < 120; i++) {
        double magnitude = 5.0 * (120.0 - i) / 120.0;
        Color color = WHITE;
        color = apply_brightness(color, magnitude, false, 1.0f);
        DrawLine(SCREEN_WIDTH - 200, 90 + i, SCREEN_WIDTH - 130, 90 + i, color);
    }
    DrawText("0", SCREEN_WIDTH - 200, 85 + 120, 16, BLACK);
    DrawText("5+", SCREEN_WIDTH - 150, 85 + 120, 16, BLACK);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Complex Domain Coloring");
    SetTargetFPS(60);
    Image colorImage = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(colorImage);
    double scale = 100.0;  // Larger scale to see more detail initially
    double centerX = 0.0;
    double centerY = 0.0;
    FunctionType current_function = FUNC_EXP;
    ColoringParams coloring_params = {
        .show_phase_lines = true,
        .show_modulus_lines = true,
        .enhanced_contrast = true,
        .line_thickness = 0.05f,
        .saturation = 0.9f,
        .value = 1.0f,
        .contrast_strength = 1.0f,
        .anti_aliasing = 1  // Default: no anti-aliasing (can be 1, 2, or 4)
    };
    StatusMessage status_message = {
        .status = STATUS_OK,
        .message = "",
        .display_time = 0.0f,
        .active = false
    };
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
    Rectangle functionButton = { 10, SCREEN_HEIGHT - 70, 240, 30 };
    Rectangle phaseLineButton = { 10, SCREEN_HEIGHT - 110, 160, 30 };
    Rectangle modulusLineButton = { 180, SCREEN_HEIGHT - 110, 190, 30 };
    Rectangle contrastButton = { 380, SCREEN_HEIGHT - 110, 210, 30 };
    Rectangle resetButton = { 600, SCREEN_HEIGHT - 110, 150, 30 };
    Rectangle antiAliasingButton = { 330, SCREEN_HEIGHT - 70, 240, 30 };
    while (!WindowShouldClose()) {
        bool needsUpdate = false;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && GetMouseY() > 20 && GetMouseY() < SCREEN_HEIGHT - 120) {
            Vector2 delta = GetMouseDelta();
            centerX -= delta.x / scale;
            centerY += delta.y / scale;
            needsUpdate = true;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scale *= (wheel > 0) ? 1.2 : 0.8;
            needsUpdate = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), functionButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            current_function = (current_function + 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), phaseLineButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.show_phase_lines = !coloring_params.show_phase_lines;
            needsUpdate = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), modulusLineButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.show_modulus_lines = !coloring_params.show_modulus_lines;
            needsUpdate = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), contrastButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.enhanced_contrast = !coloring_params.enhanced_contrast;
            needsUpdate = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), antiAliasingButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            coloring_params.anti_aliasing = (coloring_params.anti_aliasing == 1) ? 2 : 
                                            (coloring_params.anti_aliasing == 2) ? 4 : 1;
            needsUpdate = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), resetButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            centerX = 0.0;
            centerY = 0.0;
            scale = 100.0;
            coloring_params.show_phase_lines = true;
            coloring_params.show_modulus_lines = true;
            coloring_params.enhanced_contrast = true;
            coloring_params.anti_aliasing = 1;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            current_function = (current_function + 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_LEFT)) {
            current_function = (current_function + FUNC_COUNT - 1) % FUNC_COUNT;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_P)) {
            coloring_params.show_phase_lines = !coloring_params.show_phase_lines;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_M)) {
            coloring_params.show_modulus_lines = !coloring_params.show_modulus_lines;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_C)) {
            coloring_params.enhanced_contrast = !coloring_params.enhanced_contrast;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_A)) {
            coloring_params.anti_aliasing = (coloring_params.anti_aliasing == 1) ? 2 : 
                                          (coloring_params.anti_aliasing == 2) ? 4 : 1;
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET)) {
            coloring_params.saturation = Clamp(coloring_params.saturation - 0.1f, 0.0f, 1.0f);
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            coloring_params.saturation = Clamp(coloring_params.saturation + 0.1f, 0.0f, 1.0f);
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_MINUS)) {
            coloring_params.contrast_strength = Clamp(coloring_params.contrast_strength - 0.2f, 0.2f, 5.0f);
            needsUpdate = true;
        }
        if (IsKeyPressed(KEY_EQUAL)) {
            coloring_params.contrast_strength = Clamp(coloring_params.contrast_strength + 0.2f, 0.2f, 5.0f);
            needsUpdate = true;
        }
        if (needsUpdate) {
            pixels = LoadImageColors(colorImage);
            if (pixels != NULL) {
                if (status_message.status == STATUS_RENDER_ERROR) {
                    status_message.active = false;
                }
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
                status_message.status = STATUS_MEMORY_ERROR;
                snprintf(status_message.message, sizeof(status_message.message), 
                         "Memory allocation error - try reducing quality settings");
                status_message.active = true;
                status_message.display_time = 5.0f;
            }
        }
        if (status_message.active) {
            status_message.display_time -= GetFrameTime();
            if (status_message.display_time <= 0) {
                status_message.active = false;
            }
        }
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);
            DrawText(TextFormat("Scale: %.2f", scale), 10, 10, 20, WHITE);
            DrawText(TextFormat("Center: (%.2f, %.2f)", centerX, centerY), 10, 40, 20, WHITE);
            draw_color_legend(coloring_params.saturation, coloring_params.value);
            draw_magnitude_legend();
            DrawRectangleRec(functionButton, LIGHTGRAY);
            DrawText(TextFormat("Function: %s", function_names[current_function]), 
                     functionButton.x + 10, functionButton.y + 5, 20, BLACK);
            DrawRectangleRec(phaseLineButton, coloring_params.show_phase_lines ? SKYBLUE : LIGHTGRAY);
            DrawText("Phase Lines", phaseLineButton.x + 10, phaseLineButton.y + 5, 20, BLACK);
            DrawRectangleRec(modulusLineButton, coloring_params.show_modulus_lines ? SKYBLUE : LIGHTGRAY);
            DrawText("Modulus Lines", modulusLineButton.x + 10, modulusLineButton.y + 5, 20, BLACK);
            DrawRectangleRec(contrastButton, coloring_params.enhanced_contrast ? SKYBLUE : LIGHTGRAY);
            DrawText("Enhanced Contrast", contrastButton.x + 10, contrastButton.y + 5, 20, BLACK);
            DrawRectangleRec(resetButton, LIGHTGRAY);
            DrawText("Reset View", resetButton.x + 30, resetButton.y + 5, 20, BLACK);
            DrawRectangleRec(antiAliasingButton, LIGHTGRAY);
            DrawText(TextFormat("AA: %dx", coloring_params.anti_aliasing), 
                     antiAliasingButton.x + 20, antiAliasingButton.y + 5, 20, BLACK);
            DrawText(TextFormat("Sat: %.1f", coloring_params.saturation), 580, SCREEN_HEIGHT - 70, 16, BLACK);
            DrawText(TextFormat("Contrast: %.1f", coloring_params.contrast_strength), 580, SCREEN_HEIGHT - 50, 16, BLACK);
            if (status_message.active) {
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
                DrawRectangle(SCREEN_WIDTH/2 - MeasureText(status_message.message, 20)/2 - 10,
                             10, MeasureText(status_message.message, 20) + 20, 40, Fade(BLACK, 0.7f));
                DrawText(status_message.message, 
                         SCREEN_WIDTH/2 - MeasureText(status_message.message, 20)/2,
                         20, 20, msgColor);
            }
            DrawText("Left/Right arrows: change function", 10, SCREEN_HEIGHT - 150, 16, WHITE);
            DrawText("P: toggle phase lines, M: toggle modulus lines", 10, SCREEN_HEIGHT - 170, 16, WHITE);
            DrawText("C: toggle enhanced contrast", 10, SCREEN_HEIGHT - 190, 16, WHITE);
            DrawText("[/]: adjust saturation, -/=: adjust contrast", 10, SCREEN_HEIGHT - 210, 16, WHITE);
            DrawText("A: cycle anti-aliasing (1x→2x→4x→1x)", 10, SCREEN_HEIGHT - 230, 16, WHITE);
            DrawText("Mouse drag: pan view, Mouse wheel: zoom in/out", 10, SCREEN_HEIGHT - 250, 16, WHITE);
            
        EndDrawing();
    }
    UnloadTexture(texture);
    UnloadImage(colorImage);
    CloseWindow();
    return 0;
}