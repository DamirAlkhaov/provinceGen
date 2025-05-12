#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui/raygui.h>
#include <time.h>
#include "bmpLoader.h"
#include "program.h"



int main(){

    srand(time(NULL));
    char fileName[256];
    char cfileName[256];
    
    puts("land map (.bmp):");
    scanf("%256s", &fileName);

    puts("Would you like to use country mask? 1 / 0");
    int a;
    scanf("%d", &a);
    COUNTRIES_MASK *cmask = NULL;
    if (a){
        puts("country mask (.bmp):");
        scanf("%256s", &cfileName);

        cmask = edgeDetect_ReturnIDMAP(cfileName);
        if (cmask == NULL){
            perror("Error with country mask");
            return -1;
        }
    }

    InitWindow(800, 800, "Hello");

    SetTargetFPS(60);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20); 

    int fileSize;
    bool hasTexture = 0;
    unsigned char *data = NULL;
    Texture txt;

    float scaleX;
    float scaleY;
    float scale;

    float pixelSpacing = 40;

    while (!WindowShouldClose()){

        if (IsKeyDown(KEY_UP)){
            pixelSpacing += 5 * GetFrameTime();
            if (pixelSpacing > 9000){
                pixelSpacing = 9000;
            }
        }
        if (IsKeyDown(KEY_DOWN)){
            pixelSpacing -= 5 * GetFrameTime();
            if (pixelSpacing < 10){
                pixelSpacing = 10;
            }
        }

        if (IsKeyPressed(KEY_SPACE)) {
            puts("DEBUG : Generate map");
            if (hasTexture) {
                UnloadTexture(txt); // Free previous texture
            }
        
            data = program(fileName, cmask, a, &fileSize, pixelSpacing);
            if (!data || fileSize <= 0) {
                printf("program() failed or returned invalid image data\n");
                continue;  // skip drawing
            }

            Image img = LoadImageFromMemory(".bmp", data, fileSize);
            free(data);

            ExportImage(img, "output_raylib.bmp");

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

        BeginDrawing();

        ClearBackground((Color){0, 0, 0});
        if (hasTexture){
            
            DrawTextureEx(txt, (Vector2){0,0}, 0.0f, scale, WHITE);
        }

        DrawText("Press SPACE to generate map", 10, 10, 20, LIGHTGRAY);
        char pixelS[256];
        sprintf(pixelS, "%.0f", pixelSpacing);
        DrawText(pixelS, 10, 30, 20, LIGHTGRAY);
        
        EndDrawing();
    }

    if (hasTexture) {
        UnloadTexture(txt);
    }

    if (a) {
        free(cmask->countryMap);
        free(cmask);
    }

    CloseWindow();
    return 0;
}