#version 330 core
out vec4 FragColor;

in vec3 vTexCoords;

uniform samplerCube uSkybox;
uniform float uExposure;

void main()
{
    vec3 color = texture(uSkybox, vTexCoords).rgb;
    
    // Tone mapping
    color = vec3(1.0) - exp(-color * uExposure);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}