#pragma once 
#include "countriesMask.h"

static float minNavalSize = 1000.f;
static float minLandSize = 100.f;

unsigned char* program(char* fileName, char* cfileName, int* fileSize, float PIXEL_SPACING, float NAVAL_PIXEL_SPACING);