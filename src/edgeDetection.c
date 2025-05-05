#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bmpLoader.h"
#include "BGR.h"

int main(){
    char filename[256];
    printf("filename: ");
    scanf("%255s", filename);

    BMP* image = bmpLoad(filename);
    if (!image) {
        perror("Failed to load image");
        return -1;
    }

    const int bytesPerPixel = image->bitDepth;
    const int threshold = 5; // Adjust sensitivity here
    
    // Create output buffer (don't modify original)
    unsigned char* output = malloc(image->height * image->rowSize);
    if (!output) {
        perror("Memory allocation failed");
        return -1;
    }
    memcpy(output, image->pixels, image->height * image->rowSize);

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

            // Set output pixel (black edges on white background)
            unsigned char val = isEdge ? 0 : 255;
            output[index] = val;
            output[index+1] = val;
            output[index+2] = val;
            xNew++;
        }
        yNew--;
    }

    // Save output
    FILE* outFile = fopen("edges.bmp", "wb");
    if (outFile) {
        fwrite(image->header, 1, 54, outFile);
        fwrite(output, 1, image->height * image->rowSize, outFile);
        fclose(outFile);
    } else {
        perror("Failed to save output");
    }

    // Cleanup
    free(output);
    free(image->pixels);
    free(image);

    return 0;
}