#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "bmpLoader.h"
#include "program.h"

#define MAX_BORDER_ARRAY_SIZE 100000

#define PLAINS 80
#define FOREST 40
#define DESERT 10

#define COLOR_THRESHOLD 0

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

    int water;
} PROVINCE_CENTER;

static int gridWidth;    
static int gridHeight;  
static int gridCellSize; 
static POINT** grid; 

int getCellIndex(int x, int y) {
    return (y / gridCellSize) * gridWidth + (x / gridCellSize);
}

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
        return 1.0f;
        break;
    }
}

float calculateDensityDistance(int index, int* densityMap, float PIXEL_SPACING){
    switch (densityMap[index])
    {
    case PLAINS:
        return PIXEL_SPACING * 0.8;
        break;
    case FOREST:
        return PIXEL_SPACING * 1.3;
        break;
    case DESERT:
        return PIXEL_SPACING * 2;
        break;
    default:
        return PIXEL_SPACING;
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

void floodFill(int waterTile, PROVINCE_CENTER* p, int* map, int* densityMap, int* floodMap, int width, int height) {
    int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    puts("DEBUG : floodFill function call");

    int changed;
    int tries = 0;
    while (tries < 100){
        changed = 0;
        int initialBorderCount = p->borderCount;

        for (int i = 0; i < initialBorderCount; i++) {
            POINT current = p->borderArray[i];

            for (int d = 0; d < 4; d++) {
                int nx = current.x + directions[d][0];
                int ny = current.y + directions[d][1];

                if (nx >= 0 && nx < width && ny >= 0 && ny < height && 
                    map[ny*width + nx] == waterTile && (densityMap[ny*width + nx] != 0 || waterTile != 0) && floodMap[ny*width+nx] == 0) {
                    
                    floodMap[ny*width + nx] = p->ID;
                    addBorderPixel(p, nx, ny, width, height);
                    changed = 1;
                    
                }
            }
        }

        if (!changed){
            tries++;
        }

        // Remove processed pixels (shift array down)
        memmove(p->borderArray, 
                p->borderArray + initialBorderCount,
                (p->borderCount - initialBorderCount    ) * sizeof(POINT));
        p->borderCount -= initialBorderCount;
    }
    
    
}

//function that takes existing starter points and grows them with BFS
void growProvince(int waterTile, PROVINCE_CENTER* p, int* map, int* densityMap, COUNTRIES_MASK* cmask, int width, int height) {
    int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    int initialBorderCount = p->borderCount; // Capture before modification

    if (!waterTile){
        int chance = rand() % 100;
        if (chance < 10 / p->growthSpeed){
            return;
        }
    }
    
    for (int i = 0; i < initialBorderCount; i++) {
        POINT current = p->borderArray[i];
        int px = p->x - current.x;
        int py = p->y - current.y;

        // Skip with chance
        if (px * px + py * py > 40){
            if (1 + rand() % 100 < 30) {
                addBorderPixel(p, current.x, current.y, width, height);
                continue;
            }
        }

        for (int d = 0; d < 4; d++) {
            int nx = current.x + directions[d][0];
            int ny = current.y + directions[d][1];

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && 
                map[ny*width + nx] == waterTile && (densityMap[ny*width + nx] != 0 || waterTile != 0)) {

                if (cmask!= NULL){
                    if ((p->countryID == cmask->countryMap[ny*width + nx] || cmask->countryMap[ny*width + nx] == -2)){
                        map[ny*width + nx] = p->ID;
                        addBorderPixel(p, nx, ny, width, height);
                    }
                } else if (cmask == NULL) {
                    map[ny*width + nx] = p->ID;
                    addBorderPixel(p, nx, ny, width, height);
                }
            }
        }
    }
    // Remove processed pixels (shift array down)
    memmove(p->borderArray, 
            p->borderArray + initialBorderCount,
            (p->borderCount - initialBorderCount    ) * sizeof(POINT));
    p->borderCount -= initialBorderCount;
}

int provinceFunction(int initial, int waterTile, PROVINCE_CENTER* Points, POINT* poissonPoints, COUNTRIES_MASK* cmask, int* map, int* densityMap, int* floodMap, int provinceCount, float PIXEL_SPACING, int width, int height){
    // --- Poisson Disk Sampling (Bridson's Algorithm) ---
    // Add flood fill for the seeds so that ALL continents get one seed. 

    #define MAX_ATTEMPTS 1000

    int maxSamples = provinceCount;
    int maxPoints = maxSamples * 2;
    gridCellSize = PIXEL_SPACING / sqrtf(2.0f);

    int minIslandSize = waterTile ? 1000 : 100;

    gridWidth = (width / gridCellSize) + 1;
    gridHeight = (height / gridCellSize) + 1;
    grid = calloc(gridWidth * gridHeight, sizeof(POINT*));

    poissonPoints = malloc(maxPoints * sizeof(POINT));
    POINT* processList = malloc(maxPoints * sizeof(POINT));
    int sampleCount = 0, processCount = 0;

    // seed points via flood fill for continents/islands.
    if (!floodMap){
        floodMap = malloc(height * width * sizeof(int));
        memset(floodMap, 0, sizeof(int) * width * height);
    }
    if (!floodMap){
        perror("Failed to allocate memory for the Flood map.");
        return 0;
    }

    puts("DEBUG : FLOODING MAP WITH SAMPLES");

    int changed;
    int done = 0;
    while (!done) {
        changed = 0;

        for (int y = 0; y < height; y++){
            for (int x = 0; x < width; x++){
                int index = y * width + x;

                // Check if shit is good for landing
                if (map[index] == waterTile && (densityMap[index] > 0 || waterTile != 0) && floodMap[index] == 0){
                    // start flood fill then add seed to the index point;
                    //printf("DEBUG : Flooding at %d, %d\n IsWater: %d\n", x, y, map[index]);

                    PROVINCE_CENTER poissonSeedPoint;
                    poissonSeedPoint.x = x;
                    poissonSeedPoint.y = y;
                    poissonSeedPoint.ID = sampleCount+1;
                    poissonSeedPoint.borderCount = 1;
                    poissonSeedPoint.borderArrayCapacity = 100;
                    poissonSeedPoint.size = 0;
                    poissonSeedPoint.water = waterTile;

                    poissonSeedPoint.borderArray = malloc(100 * sizeof(POINT));
                    if (!poissonSeedPoint.borderArray) {
                        perror("Failed to allocate border array");
                        free(poissonSeedPoint.borderArray);
                        return 0;
                    }
                    poissonSeedPoint.borderArray[0] = (POINT){x, y};

                    floodFill(waterTile, &poissonSeedPoint, map, densityMap, floodMap, width, height);

                    if (poissonSeedPoint.size < minIslandSize) {
                        free(poissonSeedPoint.borderArray);
                        puts("DEBUG : ignored lake/too small  body of water");
                        continue;
                    }

                    poissonPoints[sampleCount] = (POINT){x, y};
                    processList[processCount] = poissonPoints[sampleCount];
                    grid[getCellIndex(x, y)] = &poissonPoints[sampleCount];
                    sampleCount++;
                    processCount++;
                    changed = 1;

                    free(poissonSeedPoint.borderArray);
                }
            }
        }

        if (!changed){
            done = 1;
        }
    }

    puts("DEBUG : finished flooding");

    // --- Generate more points
    while (processCount > 0 && sampleCount < maxSamples) {
        int idx = rand() % processCount;
        POINT center = processList[idx];
        
        if (waterTile == -1) printf("DEBUG : Processing seed %d, process queue: %d, sample count: %d\n", idx, processCount, sampleCount);

        int attempts = 0;
        while (attempts < MAX_ATTEMPTS && sampleCount < maxSamples) {
            float angle = (float)rand() / RAND_MAX * 2 * M_PI;
            float radius = PIXEL_SPACING * (1 + ((float)rand() / RAND_MAX));
            int nx = center.x + cosf(angle) * radius;
            int ny = center.y + sinf(angle) * radius;

            if (nx < 0 || ny < 0 || nx >= width || ny >= height)
                { attempts++; continue; }

            int nidx = ny * width + nx;
            if (map[nidx] != waterTile || (densityMap[nidx] == 0 && waterTile == 0))
                { attempts++; continue; }

            // Check nearby grid cells
            int gx = nx / gridCellSize;
            int gy = ny / gridCellSize;
            int tooClose = 0;
            for (int j = -2; j <= 2 && !tooClose; j++) {
                for (int i = -2; i <= 2; i++) {
                    int cx = gx + i;
                    int cy = gy + j;
                    if (cx < 0 || cy < 0 || cx >= gridWidth || cy >= gridHeight) continue;
                    POINT* neighbor = grid[cy * gridWidth + cx];
                    if (neighbor) {
                        int dx = neighbor->x - nx;
                        int dy = neighbor->y - ny;
                        float spacing = calculateDensityDistance(center.y * width + center.x, densityMap, PIXEL_SPACING);

                        if (dx*dx + dy*dy < spacing*spacing) {
                            tooClose = 1;
                            break;
                        }
                    }
                }
            }

            if (!tooClose) {
                poissonPoints[sampleCount] = (POINT){nx, ny};
                processList[processCount++] = poissonPoints[sampleCount];
                grid[getCellIndex(nx, ny)] = &poissonPoints[sampleCount];
                sampleCount++;
            }

            attempts++;
        }

        // Remove processed center
        processList[idx] = processList[--processCount];
    }

    provinceCount = sampleCount;
    puts("DEBUG : Finished placing the rest poisson points");
    // Initialize provinces
    for (int i = initial; i < provinceCount+initial; i++) {
        int x = poissonPoints[i - initial].x;
        int y = poissonPoints[i - initial].y;
        int index = y * width + x;

        map[index] = i+1;
        int cID = cmask ? cmask->countryMap[index] : -2;

        Points[i].x = x;
        Points[i].y = y;
        Points[i].ID = i+1;
        Points[i].growthSpeed = 1.0f;
        Points[i].borderCount = 1;
        Points[i].borderArrayCapacity = 100;
        Points[i].countryID = cID;
        Points[i].size = 0;
        Points[i].water = waterTile;

        Points[i].borderArray = malloc(100 * sizeof(POINT));
        if (!Points[i].borderArray) {
            perror("Failed to allocate border array");
            // Free previously allocated border arrays
            for (int j = 0; j < i; j++) {
                free(Points[j].borderArray);
            }
            return 0;
        }
        Points[i].borderArray[0] = (POINT){x, y};
        if (waterTile == -1) printf("DEBUG : Initialized Point %d\n", i);
    }

    puts("DEBUG : FINISHED SPAWNING PROVINCE ROOTS");

    // Grow provinces using BFS
    int activeProvinces = provinceCount;
    int unchangedIterations = 0;
    const int MAX_UNCHANGED = 1000;

    while (activeProvinces > 0 && unchangedIterations < MAX_UNCHANGED) {
        int changed = 0;
        
        for (int i = initial; i < provinceCount+initial; i++) {
            int before = Points[i].borderCount;
            if (before > 0) {
                growProvince(waterTile, &Points[i], map, densityMap, cmask, width, height);
                if (Points[i].borderCount != before) changed = 1;
                if (Points[i].borderCount == 0) {
                    activeProvinces--;
                }
            }
        }
        
        if (!changed) unchangedIterations++;
        else unchangedIterations = 0;
    }

    puts("DEBUG : finished growing provinces");
    memset(floodMap, 0, sizeof(int) * width * height);
    return 1;
}

unsigned char* program(char* fileName, char* cfileName, int* fileSize, float PIXEL_SPACING, float NAVAL_PIXEL_SPACING) {
    puts("DEBUG : Initiating map generation program");

    // Initialize all pointers to NULL
    BMP *mainImg = NULL;
    int *map = NULL;
    int *floodMap = NULL;
    int *densityMap = NULL;
    PROVINCE_CENTER *Points = NULL;
    BGR *colorHashMap = NULL;
    unsigned char *newPixels = NULL;
    unsigned char *data = NULL;
    POINT* poissonPoints = NULL;
    COUNTRIES_MASK *cmask = NULL;
    int UseCountryMask = 0;

    if (strlen(cfileName) > 0){
        cmask = edgeDetect_ReturnIDMAP(cfileName);
        if (cmask == NULL){
            perror("Error with country mask");
            return NULL;
        }

        UseCountryMask = 1;
    }

    // Load the main image
    mainImg = bmpLoad(fileName);
    if (mainImg == NULL) {
        perror("Failed to load image");
        return NULL;
    }

    // Allocate memory for maps
    map = calloc(mainImg->height * mainImg->width, sizeof(int));
    if (map == NULL) {
        perror("Failed to allocate map");
        goto cleanup;
    }

    densityMap = calloc(mainImg->height * mainImg->width, sizeof(int));
    if (densityMap == NULL) {
        perror("Failed to allocate density map");
        goto cleanup;
    }

    int landCount = 0;
    int waterCount = 0;

    // Create land/water and density maps
    int yNew = 0;
    for (int y = mainImg->height - 1; y >= 0; y--) {
        int xNew = 0;
        for (int x = 0; x < mainImg->rowSize; x += mainImg->bitDepth) {
            int index = yNew * mainImg->width + xNew;
            unsigned char blue = mainImg->pixels[y * mainImg->rowSize + x];
            unsigned char green = mainImg->pixels[y * mainImg->rowSize + x + 1];
            unsigned char red = mainImg->pixels[y * mainImg->rowSize + x + 2];

            // Land/water classification
            if(blue > 10) {
                map[index] = -1;
                waterCount++;
            } else {
                landCount++;
                map[index] = 0;
            }

            // Density classification
            if (red > 200) {
                densityMap[index] = PLAINS;
            } else if (red > 128) {
                densityMap[index] = FOREST;
            } else if (red > 10) {
                densityMap[index] = DESERT;
            } else {
                densityMap[index] = 0;
            }

            xNew++;
        }
        yNew++;
    }

    puts("DEBUG : Created land and density map.");

    // Estimate province area based on spacing
    float provinceArea = 3.14f * PIXEL_SPACING * PIXEL_SPACING / 4.0f; // approximating circular influence
    float navalProvinceArea = 3.14f * NAVAL_PIXEL_SPACING * NAVAL_PIXEL_SPACING / 4.0f;

    // Compute weighted land score from densityMap
    float weightedLand = 0.0f;
    for (int i = 0; i < mainImg->height * mainImg->width; i++) {
        switch (densityMap[i]) {
            case PLAINS:
                weightedLand += 1.0f;
                break;
            case FOREST:
                weightedLand += 0.6f;
                break;
            case DESERT:
                weightedLand += 0.2f;
                break;
            default:
                break;
        }
    }

    // Derive province count from weighted land and estimated province area
    int provinceCount = (int)(weightedLand / provinceArea);
    int naval_provinceCount = (int)(waterCount / navalProvinceArea);
    if (provinceCount < 1) provinceCount = 1;  // safety guard

    printf("Weighted Land Score: %.2f\nEstimated Province Count: %d\n", weightedLand, provinceCount);
    printf("Water pixels: %d\nNaval Province Count: %d\n", waterCount, naval_provinceCount);

    // Allocate province centers
    Points = malloc((provinceCount + naval_provinceCount) * sizeof(PROVINCE_CENTER));
    if (!Points) {
        perror("Failed to allocate province centers");
        return 0;
    }

    if (!provinceFunction(0, 0, Points, poissonPoints, cmask, map, densityMap, floodMap, provinceCount, PIXEL_SPACING, mainImg->width, mainImg->height)){
        goto cleanup;
    }

    if (naval_provinceCount){
        if (!provinceFunction(provinceCount, -1, Points, poissonPoints, cmask, map, densityMap, floodMap, naval_provinceCount, NAVAL_PIXEL_SPACING, mainImg->width, mainImg->height)){
            goto cleanup;
        }
    }

    // Assign colors to provinces
    colorHashMap = malloc(sizeof(BGR) * ((provinceCount + 1) + (naval_provinceCount + 1)));
    if (!colorHashMap) {
        perror("Failed to allocate color hash map");
        goto cleanup;
    }

    for (int i = 0; i < provinceCount + naval_provinceCount; i++) {
        colorHashMap[i] = randColor(i+1, colorHashMap, provinceCount + naval_provinceCount, Points[i].water);
    }
    puts("DEBUG : finished assigning colors.");

    // Create output image
    newPixels = malloc(mainImg->height * mainImg->rowSize);
    if (!newPixels) {
        perror("Failed to allocate new pixels");
        goto cleanup;
    }

    yNew = 0;
    for (int y = mainImg->height - 1; y >= 0; y--) {
        int xNew = 0;
        for (int x = 0; x < mainImg->rowSize; x += mainImg->bitDepth) {
            int index = yNew * mainImg->width + xNew;
            if (map[index] != -1 && map[index] != 0) {
                int provinceIndex = map[index] - 1;
                newPixels[y * mainImg->rowSize + x] = colorHashMap[provinceIndex].b;
                newPixels[y * mainImg->rowSize + x + 1] = colorHashMap[provinceIndex].g;
                newPixels[y * mainImg->rowSize + x + 2] = colorHashMap[provinceIndex].r;
                if(mainImg->bitDepth > 3) newPixels[y * mainImg->rowSize + x + 3] = 255;
            } else if (map[index] == -2 || map[index] == 0) {
                memset(newPixels + y * mainImg->rowSize + x, 255, mainImg->bitDepth);
            } else {
                memset(newPixels + y * mainImg->rowSize + x, 0, mainImg->bitDepth);
                if(mainImg->bitDepth > 3) newPixels[y * mainImg->rowSize + x + 3] = 255;
            }
            xNew++;
        }
        yNew++;
    }

    puts("DEBUG : Finished converting to bmp data");

    // Create final BMP data
    *fileSize = 54 + mainImg->height * mainImg->rowSize;
    data = malloc(*fileSize);
    if (!data) {
        perror("Failed to allocate output data");
        goto cleanup;
    }

    memcpy(data, mainImg->header, 54);
    memcpy(data + 54, newPixels, mainImg->height * mainImg->rowSize);

cleanup:
    // Free all allocated resources
    if (map) free(map);
    if (densityMap) free(densityMap);
    if (colorHashMap) free(colorHashMap);
    if (newPixels) free(newPixels);
    if (poissonPoints) free(poissonPoints);
    if (grid) free(grid);
    if (floodMap) free(floodMap);
    
    if (Points) {
        for (int i = 0; i < provinceCount; i++) {
            if (Points[i].borderArray) free(Points[i].borderArray);
        }
        free(Points);
    }
    
   if (mainImg) {
        if (mainImg->pixels) {
            free(mainImg->pixels);
            mainImg->pixels = NULL;  // Explicit nulling
        }
        free(mainImg);
        mainImg = NULL;
    }

    if (cmask) {
        if (cmask->countryMap) free(cmask->countryMap);
        free(cmask);
    }

    return data;
}