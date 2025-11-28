#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
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
    vec3 normal = normalize(tNormal);
    vec3 lightDir = normalize(-uLightDir);
    vec3 viewDir = normalize(uViewPos - tFragPos);

    // Ambient
    vec3 ambient = uLightAmbient * uMaterial.ambient;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = uLightDiffuse * diff * uMaterial.diffuse;

    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);
    vec3 specular = uLightSpecular * spec * uMaterial.specular;

    // Combine
    vec3 result = ambient + diffuse + specular;
    result = clamp(result, 0.0, 1.0);

    FragColor = vec4(result, 1.0);
}