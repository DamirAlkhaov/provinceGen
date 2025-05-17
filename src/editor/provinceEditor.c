#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    Color* colors;
    int size;
    int count;
} COLOR_ARRAY;

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Province Map Click and Highlight");

    // high light shader
    Shader shader = LoadShader(0, "highlight.fs");
    int locSelectedColor = GetShaderLocation(shader, "selectedColor");

    Image background = LoadImage("eu.bmp");

    Image idMapImage = LoadImage("output_raylib.bmp");
    ImageFormat(&idMapImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8);  // ensure RGB

    Texture2D mapTexture = LoadTextureFromImage(background);
    Texture2D idTexture = LoadTextureFromImage(idMapImage);
    Color *idPixels = LoadImageColors(idMapImage);

    Color selectedColor = {0};
    bool provinceSelected = false;
    Texture2D provinceHighlightTexture = {0};

    Camera2D camera = {0};
    camera.target = (Vector2){ 0, 0 };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    SetTargetFPS(60);

    // BORDER OUTLINE GEN
    Image empty = GenImageColor(idMapImage.width, idMapImage.height, BLANK);
    Color *borderPixels = LoadImageColors(empty);

    for (int y = 0; y < empty.height; y++){
        for (int x = 0; x < empty.width; x++){
            int index = y * empty.width + x;
            Color current = idPixels[index];
            int similar = 0;

            for (int by = -1; by <= 1; by++){
                for (int bx = -1; bx <= 1; bx++){
                    if (by == 0 && bx == 0) continue;

                    int ny = y + by;
                    int nx = x + bx;
                    int nIndex = ny * empty.width + nx;

                    if ((nx > 0 && nx < empty.width) && (ny > 0 && ny < empty.height)){
                        if (ColorToInt(current) == ColorToInt(idPixels[nIndex])) similar++;
                    }
                }
            }

            if (similar < 8){
                borderPixels[index] = (Color){0, 0, 0, 50};
            }
        }
    }

    Image borderOutline = {
                        .data = borderPixels,
                        .width = empty.width,
                        .height = empty.height,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    Texture2D borderOutlineTxt = LoadTextureFromImage(borderOutline);
    GenTextureMipmaps(&borderOutlineTxt);
    UnloadImage(borderOutline);
    UnloadImage(empty);
    // ---------------------

    while (!WindowShouldClose())
    {
        // Camera movement
        float moveSpeed = 500 * GetFrameTime();
        if (IsKeyDown(KEY_W)) camera.target.y -= moveSpeed;
        if (IsKeyDown(KEY_S)) camera.target.y += moveSpeed;
        if (IsKeyDown(KEY_A)) camera.target.x -= moveSpeed;
        if (IsKeyDown(KEY_D)) camera.target.x += moveSpeed;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) camera.zoom += wheel * 0.1f;
        if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        if (camera.zoom > 5.f) camera.zoom = 5.f;

        Vector2 mouse = GetMousePosition();
        Vector2 worldMouse = GetScreenToWorld2D(mouse, camera);
        int x = (int)worldMouse.x;
        int y = (int)worldMouse.y;

        Color selected = BLACK;
        if (x >= 0 && y >= 0 && x < mapTexture.width && y < mapTexture.height) {
            selected = idPixels[y * mapTexture.width + x];
        }

        float rgb[3] = {
            selected.r / 255.0f,
            selected.g / 255.0f,
            selected.b / 255.0f
        };
        SetShaderValue(shader, locSelectedColor, rgb, SHADER_UNIFORM_VEC3);

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode2D(camera);

                if (camera.zoom >= 0.3f) {

                    BeginShaderMode(shader);
                        SetShaderValueTexture(shader, GetShaderLocation(shader, "baseMap"), mapTexture);
                        SetShaderValueTexture(shader, GetShaderLocation(shader, "idMap"), idTexture);
                        DrawTexture(mapTexture, 0, 0, WHITE);
                    EndShaderMode();
                    
                    DrawTexture(borderOutlineTxt, 0, 0, WHITE);
                } else {
                    DrawTexture(mapTexture, 0, 0, WHITE);
                }

            EndMode2D();

        EndDrawing();
    }

    if (provinceHighlightTexture.id > 0) UnloadTexture(provinceHighlightTexture);
    UnloadTexture(mapTexture);
    UnloadTexture(borderOutlineTxt);
    UnloadImageColors(idPixels);
    UnloadImage(idMapImage);

    CloseWindow();

    return 0;
}
