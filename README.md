# Province Generator
Program that generates a province map using a height map and country mask(optional) for games like HOI4/eu4

## Usage
-- for the base map you need a map of your world. red channel == spawn density | blue channel == water (not land, cannot spawn provinces here)
-- country mask (optional) white == water, countries must be unique colors, same image size as base map and preferably no blending of colours for easier detection.
