#pragma once

typedef struct {
    unsigned char header[54];
    unsigned char *pixels;
    int height;
    int width;
    int rowSize;
    short bitDepth;
} BMP;


BMP* bmpLoad(char fileName[256]);

unsigned char* bmp2Memory(const char* fileName, int* sizeOut);