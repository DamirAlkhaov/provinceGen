#include "BGR.h"

//simple random color function that also checks for any similar exisiting colors.
BGR randColor(int ID, BGR *hash, int size, int waterTile) {
    int attempts = 0;
    while (attempts < 1000) { 
        int red = ((ID + attempts) * 37 * (attempts + 2)) % 256;
        int green = ((ID + attempts) * 67 * (attempts + 1)) % 256;
        int blue = ((ID + attempts) * 113 * (attempts + 5)) % 256;

        //only water tiles can have 255 blue;
        if (waterTile != 0){
            blue = 255;
        } else {
            if (blue == 255){
                blue -= 40;
            }
        }

        if ((blue < 5 && red < 5 && green < 5) || (blue > 250 && red > 250 && green > 250)){
            attempts++;
            continue;
        }
        
        int collision = 0;
        for (int i = 0; i < size; i++) {
            if (hash[i].b == blue && hash[i].g == green && hash[i].r == red) {
                collision = 1;
                break;
            }
        }
        
        if (!collision) {
            return (BGR){blue, green, red};
        }
        attempts++;
    }
    return (BGR){ID % COLOR_MAX, (ID * 2) % COLOR_MAX, (ID * 3) % COLOR_MAX};
}

int BGR_Compare(BGR c1, BGR c2){
    if (abs(c1.b - c2.b) <= TRESHOLD && abs(c1.g - c2.g) <= TRESHOLD && abs(c1.r - c2.r) <= TRESHOLD) return 1;
    return 0;
}
