#include "countriesMask.h"

COUNTRIES_MASK* edgeDetect_ReturnIDMAP(){
    char filename[256];
    printf("filename: ");
    scanf("%255s", filename);

    BMP* image = bmpLoad(filename);
    if (!image) {
        perror("Failed to load image");
        return NULL;
    }

    BGR countries[1000];
    int countriesCount = 0;

    const int bytesPerPixel = image->bitDepth;
    const int threshold = 5; // Adjust sensitivity here
    
    // Create output buffer (don't modify original)
    unsigned char* output = malloc(image->height * image->rowSize);
    if (!output) {
        perror("Memory allocation failed");
        return NULL;
    }
    memcpy(output, image->pixels, image->height * image->rowSize);

    int *countryMap = malloc(sizeof(int) * image->height * image->width);

    int yNew = image->height - 1;
    for (int y = 0; y < image->height; y++) {
        int xNew = 0;
        for (int x = 0; x < image->rowSize; x += bytesPerPixel) {
            // Skip incomplete pixels at row end
            if (x + bytesPerPixel > image->rowSize) continue;

            const int index = y * image->rowSize + x;
            BGR current = {
                image->pixels[index],
                image->pixels[index+1],
                image->pixels[index+2]
            };

            int isEdge = 0;
            
            // Check 4-connected neighbors (left, right, top, bottom)
            int offsets[4] = {
                -bytesPerPixel, bytesPerPixel, 
                -image->rowSize, image->rowSize
            };

            for (int i = 0; i < 4; i++) {
                int ni = index + offsets[i];
                
                // Check if neighbor is within bounds
                if (ni >= 0 && ni < image->height * image->rowSize) {
                    BGR neighbor = {
                        image->pixels[ni],
                        image->pixels[ni+1],
                        image->pixels[ni+2]
                    };

                    // Calculate color difference
                    int diff = abs(current.b - neighbor.b) + 
                               abs(current.g - neighbor.g) + 
                               abs(current.r - neighbor.r);
                    
                    if (diff > threshold) {
                        isEdge = 1;
                        break;
                    }
                } else {
                    //image borders are white
                    isEdge = 0;
                    break;
                }
            }

            if (!isEdge && !BGR_Compare(current, (BGR){255, 255, 255}) && !BGR_Compare(current, (BGR){0, 0, 0})){
                // not edge or black/white

                int similar_found = 0;
                for (int i = 0; i < countriesCount; i++){
                    if (BGR_Compare(current, countries[i])){
                        similar_found = 1;
                        countryMap[yNew * image->width + xNew] = i;
                        break;
                    }
                }

                if (!similar_found && countriesCount < 1000) {
                    countries[countriesCount++] = (BGR){current.b, current.g, current.r};
                    countryMap[yNew * image->width + xNew] = countriesCount - 1;
                    printf("%d %d %d\n", current.b, current.g, current.r);
                }
            }

            // border ID of -2
            if ((isEdge && !BGR_Compare(current, (BGR){255, 255, 255})) || BGR_Compare(current, (BGR){0, 0, 0})){
                countryMap[yNew * image->width + xNew] = -2;
            }

            //white(water) is -1
            if (BGR_Compare(current, (BGR){255, 255, 255})){
                countryMap[yNew * image->width + xNew] = -1;
            }
            xNew++;
        }
        yNew--;
    }

    printf("Found %d countries\nPress Enter to continue..", countriesCount);
    printf("DEBUG: countryMap[0,0] = %d", countryMap[0]);
    scanf("%*c"); 
    getchar();     

    // check for -2 pixels AFTER all of the countries have been found.
    int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };
    for (int y = 0; y < image->height; y++){
        for (int x = 0; x < image->width; x++){
            if (countryMap[y * image->width + x] == -2){
                //found border pixel
                int borderingCountries[100];
                int arrSize = 0;

                int pixelCount[1000] = {0};

                for (int d = 0; d < 8; d++){
                    for (int step = 1; step <= 5; step++){
                        int nx = x + step * directions[d][0];
                        int ny = y + step * directions[d][1];

                        if (nx < 0 || ny < 0 || nx >= image->width || ny >= image->height) continue;

                        if (countryMap[ny * image->width + nx] >= 0){
                            int newCountryID = 0;
                            for (int i = 0; i < arrSize; i++){
                                if (borderingCountries[i] == countryMap[ny * image->width + nx]){
                                    pixelCount[borderingCountries[i]]++;
                                    newCountryID = 1;
                                    break;
                                } 
                            }

                            if (!newCountryID){
                                borderingCountries[arrSize++] = countryMap[ny * image->width + nx];
                                pixelCount[borderingCountries[arrSize - 1]]++;
                            }
                        }
                    }
                }

                int mostCommon = -2;
                for (int i = 0; i < arrSize; i++){
                    if (mostCommon == -2) {
                        mostCommon = borderingCountries[i];
                        continue;
                    }

                    if (i + 1 < arrSize){
                        if (pixelCount[mostCommon] < pixelCount[borderingCountries[i+1]]){
                            mostCommon = borderingCountries[i + 1];
                        }
                    }
                }

                countryMap[y * image->width + x] = mostCommon;
            }
        }
    }

    // Cleanup
    free(output);
    free(image->pixels);
    free(image);

    COUNTRIES_MASK *mask = malloc(sizeof(COUNTRIES_MASK));
    mask->countryMap = countryMap;
    mask->countriesCount = countriesCount;

    return mask;
}
