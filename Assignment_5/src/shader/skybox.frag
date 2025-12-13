#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube uSkybox;
uniform vec3 uTintColor;
uniform bool isDay;

void main()
{    
    vec3 color = texture(uSkybox, TexCoords).rgb;
    if (isDay) {
        color *= uTintColor;
    } else {
        color *= uTintColor * 0.3;
    }
    FragColor = vec4(color, 1.0);
}