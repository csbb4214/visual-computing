#version 330 core

struct Material {
    vec3 diffuse;
    float shininess;
};

in vec3 tNormal;
in vec3 tFragPos;
out vec4 FragColor;

uniform Material uMaterial;
uniform vec3 uViewPos;
uniform vec3 uLightDir;      // Directional light direction
uniform vec3 uLightAmbient;  // Ambient light color
uniform vec3 uLightDiffuse;  // Diffuse light color
uniform vec3 uLightSpecular; // Specular light color
uniform bool isGround;

void main(void) {
    // Normalize inputs
    vec3 normal = normalize(tNormal);
    vec3 lightDir = normalize(-uLightDir); // Negate because we want direction TO light
    vec3 viewDir = normalize(uViewPos - tFragPos);

    // For ground, flip normal if facing away from camera
    if (isGround && dot(normal, viewDir) < 0.0) {
        normal = -normal;
    }

    // Ambient component
    vec3 ambient = uLightAmbient * uMaterial.diffuse;

    // Diffuse component (Lambertian)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = uLightDiffuse * diff * uMaterial.diffuse;

    // Specular component (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);
    vec3 specular = uLightSpecular * spec;

    // Combine all components
    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}