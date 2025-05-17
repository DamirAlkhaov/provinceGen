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

    Image background = LoadImage("eu.bmp");

    Image idMapImage = LoadImage("output_raylib.bmp");
    ImageFormat(&idMapImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8);  // ensure RGB

    Texture2D mapTexture = LoadTextureFromImage(background);
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

    // Highlight map gen
    COLOR_ARRAY colorArray;
    colorArray.colors = malloc(1000 * sizeof(Color));
    if (!colorArray.colors){
        perror("Failed to allocate memory for color array");
        return -1;
    }
    colorArray.size = 1000;
    colorArray.count = 0;
    for (int i = 0; i < idMapImage.width * idMapImage.height && 1 < 0; i++){
        Color current = idPixels[i];

        //ignore black and white
        if (ColorToInt(current) == ColorToInt(BLACK) || ColorToInt(current) == ColorToInt(WHITE) || current.a == 0) continue;

        bool new = true;
        // search if color was already documented
        for (int j = 0; j < colorArray.count; j++){
            if (ColorToInt(colorArray.colors[j]) == ColorToInt(current)){
                new = false;
                break;
            }
        }

        if (new){
            if (colorArray.count + 1 >= colorArray.size){
                int newSize = colorArray.size * 2;
                Color* newArr = realloc(colorArray.colors, newSize * sizeof(Color));
                if (!newArr){
                    perror("Failed to expand colors array");
                    return -1;
                }
                colorArray.colors = newArr;
                colorArray.size = newSize;
            }
            colorArray.colors[colorArray.count++] = current;
            printf("New province: %d\n", colorArray.count);
        }
    }
    printf("Province count: %d\n", colorArray.count);

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

        // Mouse click logic
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            Vector2 worldMouse = GetScreenToWorld2D(mouse, camera);
            int x = (int)worldMouse.x;
            int y = (int)worldMouse.y;

            if (x >= 0 && y >= 0 && x < idMapImage.width && y < idMapImage.height) {
                if (ColorToInt(selectedColor) != ColorToInt(idPixels[y * idMapImage.width + x])){
                    selectedColor = idPixels[y * idMapImage.width + x];
                    provinceSelected = true;

                    if (provinceHighlightTexture.id > 0) {
                        UnloadTexture(provinceHighlightTexture);
                    }

                    // Create mask image
                    Image mask = GenImageColor(idMapImage.width, idMapImage.height, BLANK);
                    Color *maskPixels = LoadImageColors(mask);

                    for (int i = 0; i < idMapImage.width * idMapImage.height; i++) {
                        if (ColorToInt(idPixels[i]) == ColorToInt(selectedColor)) {
                            maskPixels[i] = (Color){255, 255, 255, 120}; // yellow transparent
                        }
                    }

                    Image newMask = {
                        .data = maskPixels,
                        .width = mask.width,
                        .height = mask.height,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
                    };

                    provinceHighlightTexture = LoadTextureFromImage(newMask);
                    UnloadImage(mask); // Frees internal .data pointer
                }
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode2D(camera);

                DrawTexture(mapTexture, 0, 0, WHITE);

                if (provinceSelected) {
                    DrawTexture(provinceHighlightTexture, 0, 0, WHITE);
                }

                if (camera.zoom >= 0.3f) DrawTexture(borderOutlineTxt, 0, 0, WHITE);

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
