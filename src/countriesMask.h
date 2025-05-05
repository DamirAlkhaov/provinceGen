#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bmpLoader.h"
#include "BGR.h"

typedef struct {
    int* countryMap;
    int countriesCount;
} COUNTRIES_MASK;

COUNTRIES_MASK* edgeDetect_ReturnIDMAP();