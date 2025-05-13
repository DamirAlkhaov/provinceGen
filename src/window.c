#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui/raygui.h>
#include <time.h>
#include "bmpLoader.h"
#include "program.h"



int main(){

    srand(time(NULL));
    char fileName[256] = "eu.bmp";
    char cfileName[256] = "";

    InitWindow(1600, 800, "Hello");

    SetTargetFPS(60);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20); 

    int fileSize;
    bool hasTexture = 0;
    unsigned char *data = NULL;
    Texture txt;

    bool maskBoxEditMode = false;
    bool countryBoxEditMode = false;


    float scaleX;
    float scaleY;
    float scale;

    float pixelSpacing = 40;

    while (!WindowShouldClose()){

        if (IsKeyPressed(KEY_SPACE)) {
            if (strlen(fileName) == 0) {
                puts("Filename is empty!");
            } else {
                WaitTime(0.5);
                puts("DEBUG : Generate map");

                if (hasTexture) {
                    UnloadTexture(txt); // Free previous texture
                }
            
                int attempts = 0;
                while (attempts < 3) {
                    data = program(fileName, cfileName, &fileSize, pixelSpacing);
                    if (data && fileSize > 0) break;
                    
                    attempts++;
                    if (attempts < 3) {
                        printf("Retrying in 100ms (attempt %d/3)\n", attempts + 1);
                        WaitTime(0.1);
                    }
                }

                Image img = LoadImageFromMemory(".bmp", data, fileSize);
                if (img.data == NULL) {
                    printf("LoadImageFromMemory failed\n");
                    continue;
                }
                free(data);
                data = NULL;

                scaleX = (float)800 / img.width;
                scaleY = (float)800 / img.height;
                scale = (scaleX < scaleY) ? scaleX : scaleY; 

                if (!img.data) {
                    printf("Image load failed.\n");
                } else {
                    txt = LoadTextureFromImage(img);
                    UnloadImage(img);  // Safe now
                }
                hasTexture = 1;
            }
        }

        BeginDrawing();

        ClearBackground((Color){0, 0, 0});
        if (hasTexture){
            
            DrawTextureEx(txt, (Vector2){800,0}, 0.0f, scale, WHITE);
        }

        DrawRectangle(0, 0, 800, 800, GRAY);

        char pixelS[256];
        sprintf(pixelS, "Pixel Spacing: %.2f", pixelSpacing);
        DrawText(pixelS, 10, 10, 20, BLACK);

        //pixel spacing slider
        GuiSlider((Rectangle){30, 30, 300, 20}, "15", "300", &pixelSpacing, 15, 300);

        //main mask input
        DrawText("Main mask:", 10, 50, 20, BLACK);
        if (GuiTextBox((Rectangle){10, 70, 400, 30}, fileName, 256, maskBoxEditMode)) {
            maskBoxEditMode = false;
        }
        if (GuiButton((Rectangle){420, 70, 100, 30}, "Edit")){
            maskBoxEditMode = true;
        }

        //countries mask input
        DrawText("Countries mask:", 10, 110, 20, BLACK);
        if (GuiTextBox((Rectangle){10, 130, 400, 30}, cfileName, 256, countryBoxEditMode)) {
            countryBoxEditMode = false;
        }
        if (GuiButton((Rectangle){420, 130, 100, 30}, "Edit")){
            countryBoxEditMode = true;
        }

        DrawText("Press SPACE to generate!", 10, 770, 20, BLACK);
        if (GuiButton((Rectangle){400, 770, 100, 20}, "SAVE")){
            if (hasTexture){
                Image tmp = LoadImageFromTexture(txt);
                ExportImage(tmp, "output_raylib.bmp");
                UnloadImage(tmp);
            }
        }
        EndDrawing();
    }

    if (hasTexture) {
        UnloadTexture(txt);
    }

    CloseWindow();
    return 0;
}