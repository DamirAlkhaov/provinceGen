#version 330

uniform sampler2D baseMap;
uniform sampler2D idMap;
uniform vec3 selectedColor; 
in vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    vec4 baseCol = texture(baseMap, fragTexCoord);
    vec3 colorAtPixel = texture(idMap, fragTexCoord).rgb;

    float epsilon = 0.001;

    bool rMatch = abs(colorAtPixel.r - selectedColor.r) < epsilon;
    bool gMatch = abs(colorAtPixel.g - selectedColor.g) < epsilon;
    bool bMatch = abs(colorAtPixel.b - selectedColor.b) < epsilon;

    if (rMatch && gMatch && bMatch) {
        fragColor = mix(baseCol, vec4(1.0), 0.4); 
    } else {
        fragColor = baseCol;
    }
}
