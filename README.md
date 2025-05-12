# Province Generator
Program that generates a province map using a height map and country mask(optional) for games like HOI4/eu4

<img src="https://github.com/DamirAlkhaov/provinceGen/raw/refs/heads/master/output_raylib.bmp" alt="Alt Text" width="300" height="300">
Image of provinces in unique color

<img src="https://github.com/DamirAlkhaov/provinceGen/raw/refs/heads/master/edges.bmp" alt="Alt Text" width="300" height="300">
Image of the provinces with edge detection

## Building
Currently only have windows libs on the repo, but if you get linux or mac repos it should work since I incorporated that to the makefile.

## Usage
-- for the base map you need a map of your world. red channel == spawn density | blue channel == water (not land, cannot spawn provinces here)
-- country mask (optional) white == water, countries must be unique colors, same image size as base map and preferably no blending of colours for easier detection.
