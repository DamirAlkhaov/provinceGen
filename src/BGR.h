#ifndef BGR_HEADER
#define BGR_HEADER
#include <stdlib.h>
#include <math.h>
#define COLOR_MAX 180
#define TRESHOLD 5

//BGR struct
typedef struct {
    unsigned char b, g, r;
} BGR;

//simple random color function that also checks for any similar exisiting colors.
BGR randColor(int ID, BGR *hash, int size);

int BGR_Compare(BGR c1, BGR c2);
#endif