#include <stdlib.h>
#include <stdio.h>
#include "bmpLoader.h"

//check if provided file is .bmp
int bmpCheck(FILE *f){
    fseek(f, 0, SEEK_SET);
    if (fgetc(f) == 'B' && fgetc(f) == 'M'){
        fseek(f, 0, SEEK_SET);
        return 1;
    } else {
        return 0;
    }
}

BMP* bmpLoad(char fileName[256]) {
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    if (!bmpCheck(file)) {
        fclose(file);
        return NULL;
    }
    
    BMP* bmp = malloc(sizeof(BMP));
    if (!bmp) {
        fclose(file);
        return NULL;
    }

    if (fread(bmp->header, sizeof(unsigned char), 54, file) != 54) {
        perror("Failed to read BMP header");
        free(bmp);
        fclose(file);
        return NULL;
    }
    
    // Extract metadata
    bmp->width = *(int*)&bmp->header[18];
    bmp->height = *(int*)&bmp->header[22];
    int pixelOffset = *(int*)&bmp->header[10];
    short bitsPerPixel = *(short*)&bmp->header[28];
    bmp->bitDepth = bitsPerPixel / 8;
    bmp->rowSize = ((bitsPerPixel * bmp->width + 31) / 32) * 4;

    printf("width: %d\nheight: %d\nbitDepth: %d\n", bmp->width, bmp->height, bmp->bitDepth);

    fseek(file, pixelOffset, SEEK_SET);
    bmp->pixels = malloc(bmp->height * bmp->rowSize);
    if (!bmp->pixels) {
        free(bmp);
        fclose(file);
        return NULL;
    }
    fread(bmp->pixels, 1, bmp->height * bmp->rowSize, file);
    
    fclose(file);
    return bmp;
}

unsigned char* bmp2Memory(const char* fileName, int* sizeOut){
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);

    unsigned char* data = malloc(fileSize);
    if (!data) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(data, 1, fileSize, file);
    if (bytesRead != fileSize) {
        fprintf(stderr, "Failed to read entire file\n");
        free(data);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *sizeOut = fileSize;
    return data;
}

