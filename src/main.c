#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "bmpLoader.h"
#include "countriesMask.h"

#define PLAINS 80
#define FOREST 50
#define DESERT 15

#define COLOR_THRESHOLD 0

#define PIXEL_SPACING 15


//used by the queue system to remember queued pixels
typedef struct {
    int x;
    int y;
} POINT;

// this struct answers for the province center/ids - can be used to add province growth rate modifiers and other stuff later on
typedef struct {
    int x, y;
    int ID;
    float growthSpeed;

    POINT *borderArray;
    int borderCount;
    int borderArrayCapacity;

    int countryID;
    int size;
} PROVINCE_CENTER;

float calculateGrowthSpeed(int index, int* densityMap){
    switch (densityMap[index])
    {
    case PLAINS:
        return (1/5.0f);
        break;
    case FOREST:
        return (1/2.5f);
        break;
    case DESERT:
        return (1/0.5f);
        break;
    default:
        break;
    }
}

void addBorderPixel(PROVINCE_CENTER* p, int x, int y, int width, int height) {
    // Check bounds
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    
    // Check capacity and expand if needed
    if (p->borderCount >= p->borderArrayCapacity) {
        int newCapacity = p->borderArrayCapacity * 2;
        
        POINT* newArray = realloc(p->borderArray, newCapacity * sizeof(POINT));
        if (!newArray) {
            printf("Failed to expand province %d borders\n", p->ID);
            return;
        }
        p->borderArray = newArray;
        p->borderArrayCapacity = newCapacity;
    }
    
    // Add new border pixel
    p->borderArray[p->borderCount++] = (POINT){x, y};
    p->size++;
}

//function that takes existing starter points and grows them with BFS
void growProvince(PROVINCE_CENTER* p, int* map, int* densityMap, int* countryMaskMap, int width, int height) {
    int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    int initialBorderCount = p->borderCount; // Capture before modification

    int chance = rand() % 100;
    if (chance < 10 / p->growthSpeed){
        return;
    }
    
    int growthThisTurn = 10;
    int processed = 0;
    for (int i = 0; i < initialBorderCount; i++) { // Use initial count
        POINT current = p->borderArray[i];
        
        for (int d = 0; d < 4; d++) {
            int nx = current.x + directions[d][0];
            int ny = current.y + directions[d][1];
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && 
                map[ny*width + nx] == 0 && densityMap[ny*width + nx] != 0 && 
                (countryMaskMap == NULL || p->countryID == countryMaskMap[ny*width + nx] || countryMaskMap[ny*width + nx] == -2)){
                
                map[ny*width + nx] = p->ID;
                addBorderPixel(p, nx, ny, width, height);
                
            }
        }
        processed++;
    }
    
    // Remove processed pixels (shift array down)
    memmove(p->borderArray, 
            p->borderArray + initialBorderCount,
            (p->borderCount - initialBorderCount) * sizeof(POINT));
    p->borderCount -= initialBorderCount;
}

int main(int argc, char* argv[]){
    srand(time(NULL));
    char fileName[256];
    
    puts("filename (.bmp):");
    scanf("%256s", &fileName);

    BMP *mainImg = bmpLoad(fileName);
    if (mainImg == NULL) {
        perror("Failed to load image");
        return -1;
    }

    puts("use country mask?\n 1 - yes, 0 - no");
    int UseCountryMask;
    scanf("%d", &UseCountryMask);

    COUNTRIES_MASK *cmask;
    if (UseCountryMask){
        cmask = edgeDetect_ReturnIDMAP();
        if (cmask == NULL){
            perror("Error with country mask");
            return -1;
        }
    }

   

    int *map = calloc(mainImg->height * mainImg->width, sizeof(int));
    if (map == NULL) return -1;

    int *densityMap = calloc(mainImg->height * mainImg->width, sizeof(int));
    if (densityMap == NULL) return -1;

    int landCount = 0;

    
    //land/water map
    int yNew = 0;
    for (int y = mainImg->height - 1; y >= 0; y--) {
        int xNew = 0;
        for (int x = 0; x < mainImg->rowSize; x += mainImg->bitDepth) {
            int index = yNew * mainImg->width + xNew;
            unsigned char blue = mainImg->pixels[y * mainImg->rowSize + x];
            unsigned char green = mainImg->pixels[y * mainImg->rowSize + x + 1];
            unsigned char red = mainImg->pixels[y * mainImg->rowSize + x + 2];

            unsigned char gray = blue;

            //land
            if(gray > 200){
                //if y = height -1 then 0 
                // if y = 0 then height - 1
                map[index] = -1;
            } else {
                landCount++;
                map[index] = 0;
            }

            //density % chance of spawning
            if (red > 200){
                //highest density 
                densityMap[index] = PLAINS;
            } else if (red > 128){
                //medium density (heavy forests/jungles)
                densityMap[index] = FOREST;
            } else if (red > 10){
                // lowest density (deserts/artic)
                densityMap[index] = DESERT;
            } else {
                //non viable terrain
                densityMap[index] = 0;
            }

            xNew++;
        }
        yNew++;
    }

    free(mainImg->pixels);
    
    //init provinces

    int provinceCount;
    printf("Land count: %d\n", landCount);
    puts("Enter province count: ");
    scanf("%d", &provinceCount);

    PROVINCE_CENTER Points[provinceCount];

    //----
    // ADD A SPACER FOR PROVINCE ROOTS SO THAT YOU GET FAIR AND EQUAL GROWTH BETWEEN NEIGHBOURS! don't want dick provinces popping up.
    //----
    int pixelSpace = (mainImg->width * PIXEL_SPACING) / provinceCount;
    for (int i = 0; i < provinceCount; i++){
        int x = rand() % mainImg->width;
        int y = rand() % mainImg->height;

        if (map[y * mainImg->width + x] == 0 && densityMap[y * mainImg->width + x] != 0){
            if ( 1 + rand() % 100 > densityMap[y * mainImg->width + x]){
                puts("skipped spawn");
                i--;
                continue;
            }

            int tooCloseNeighbour = 0;
            for (int j = 0; j < i-1; j++){
                int dx = Points[j].x - x;
                int dy = Points[j].y - y;
                if (dx * dx + dy * dy < PIXEL_SPACING * PIXEL_SPACING){
                    
                    if (UseCountryMask){
                        if (cmask->countryMap[Points[j].y * mainImg->width + Points[j].x] != cmask->countryMap[y * mainImg->width + x]){
                            continue;
                        }
                    }
                    i--;
                    tooCloseNeighbour = 1;
                }
            }
            if (tooCloseNeighbour) continue;

            // land add the province here
            map[y * mainImg->width + x] = i+1;
            int cID = -2; 
            if (UseCountryMask){
                cID = cmask->countryMap[y * mainImg->width + x];
            }

            POINT *borderArray = malloc(100 * sizeof(POINT));
            if (borderArray == NULL) {
                perror("Failed to allocate border array");
                return -1;
            }

            Points[i] = (PROVINCE_CENTER){
                .x = x, 
                .y = y,
                .ID = i+1,
                .growthSpeed = calculateGrowthSpeed(y * mainImg->width + x, densityMap),
                .borderArray = borderArray,
                .borderCount = 1,
                .borderArrayCapacity = 100,
                .countryID = cID,
                .size = 0
            };
            Points[i].borderArray[0] = (POINT){x,y};
        } else {
            i--; // restart
        }
    }

    //expand province points borders, uses BFS algorithm
    int activeProvinces = provinceCount;
    int unchangedIterations = 0;
    const int MAX_UNCHANGED = 10; 

    while (activeProvinces > 0 && unchangedIterations < MAX_UNCHANGED) {
        int changed = 0;
        
        for (int i = 0; i < provinceCount; i++) {
            int before = Points[i].borderCount;
            if (before > 0) {
                growProvince(&Points[i], map, densityMap, cmask->countryMap, mainImg->width, mainImg->height);
                if (Points[i].borderCount != before) changed = 1;
                if (Points[i].borderCount == 0) activeProvinces--;
            }
        }
        
        if (!changed) unchangedIterations++;
        else unchangedIterations = 0;
    }
    

    //check for any unoccupied land
    //fillRemainingLand(map, height, width, &provinceCount);
    puts("finished growing provinces");

    // Update color hashmap allocation
    BGR *colorHashMap = malloc(sizeof(BGR) * (provinceCount));
    for (int i = 0; i < provinceCount; i++) {
        colorHashMap[i] = randColor(i+1, colorHashMap, provinceCount);
    }
    puts("finished assigning colors.");

    //save province map

    unsigned char *newPixels = malloc(mainImg->height * mainImg->rowSize);
    if (newPixels == NULL) return -1;

    yNew = 0;
    for (int y = mainImg->height - 1; y >= 0; y--) {
        int xNew = 0;
        for (int x = 0; x < mainImg->rowSize; x += mainImg->bitDepth) {
            int index = yNew * mainImg->width + xNew;
            if (map[index] != -1 && map[index] != 0){
                newPixels[y * mainImg->rowSize + x] = colorHashMap[map[index]].b;
                newPixels[y * mainImg->rowSize + x + 1] = colorHashMap[map[index]].g;
                newPixels[y * mainImg->rowSize + x + 2] = colorHashMap[map[index]].r;
                if(mainImg->bitDepth > 3) newPixels[y * mainImg->rowSize + x + 3] = 255;
            } else if (map[index] == -2 || map[index] == 0){
                newPixels[y * mainImg->rowSize + x] = 255;
                newPixels[y * mainImg->rowSize + x + 1] = 255;
                newPixels[y * mainImg->rowSize + x + 2] = 255;
                if(mainImg->bitDepth > 3) newPixels[y * mainImg->rowSize + x + 3] = 255;
            } else {
                newPixels[y * mainImg->rowSize + x] = 0;
                newPixels[y * mainImg->rowSize + x + 1] = 0;
                newPixels[y * mainImg->rowSize + x + 2] = 0;
                if(mainImg->bitDepth > 3) newPixels[y * mainImg->rowSize + x + 3] = 255;
            }
        
            xNew++;
        }
        yNew++;
    }

    FILE *Output = fopen("output.bmp", "wb");
    if(mainImg->bitDepth < 3){
        mainImg->header[28] = 8*3;
    }
    
    fwrite(mainImg->header, 1, 54, Output);

    fwrite(newPixels, sizeof(unsigned char), mainImg->height * mainImg->rowSize, Output);
    
    fclose(Output);


    FILE *OutputTEST = fopen("output.txt", "w");

    fclose(OutputTEST);

    free(map);
    free(newPixels);
    free(colorHashMap);

    if (UseCountryMask) {
        free(cmask->countryMap);
        free(cmask);
    }
    
}