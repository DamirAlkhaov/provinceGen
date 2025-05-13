#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
    // First try normal open with proper sharing flags
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Verify BMP signature
    unsigned char signature[2];
    if (fread(signature, 1, 2, file) != 2 || signature[0] != 'B' || signature[1] != 'M') {
        fclose(file);
        printf("Invalid BMP signature\n");
        return NULL;
    }

    // Read header
    BMP* bmp = malloc(sizeof(BMP));
    if (!bmp) {
        fclose(file);
        return NULL;
    }

    rewind(file);
    if (fread(bmp->header, 1, 54, file) != 54) {
        perror("Failed to read BMP header");
        free(bmp);
        fclose(file);
        return NULL;
    }

    // Extract metadata with proper bounds checking
    bmp->width = *(int*)&bmp->header[18];
    bmp->height = *(int*)&bmp->header[22];
    int pixelOffset = *(int*)&bmp->header[10];
    short bitsPerPixel = *(short*)&bmp->header[28];
    
    // Validate basic BMP parameters
    if (bitsPerPixel != 24 && bitsPerPixel != 32) {
        printf("Unsupported BMP format: %d bits per pixel\n", bitsPerPixel);
        free(bmp);
        fclose(file);
        return NULL;
    }

    bmp->bitDepth = bitsPerPixel / 8;
    bmp->rowSize = ((bitsPerPixel * bmp->width + 31) / 32) * 4;

    // Verify pixel data offset is reasonable
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (pixelOffset + (bmp->height * bmp->rowSize) > fileSize) {
        printf("Invalid BMP dimensions or corrupted file\n");
        free(bmp);
        fclose(file);
        return NULL;
    }

    // Read pixel data
    fseek(file, pixelOffset, SEEK_SET);
    bmp->pixels = malloc(bmp->height * bmp->rowSize);
    if (!bmp->pixels) {
        free(bmp);
        fclose(file);
        return NULL;
    }

    if (fread(bmp->pixels, 1, bmp->height * bmp->rowSize, file) != bmp->height * bmp->rowSize) {
        perror("Failed to read pixel data");
        free(bmp->pixels);
        free(bmp);
        fclose(file);
        return NULL;
    }

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

