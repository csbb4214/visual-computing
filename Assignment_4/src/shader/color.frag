#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    vec3 emission;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 direction;
    float cutoff;
};

in vec3 tNormal;
in vec3 tFragPos;
out vec4 FragColor;

uniform Material uMaterial;
uniform PointLight uPointLights[4];
uniform vec3 uViewPos;
uniform vec3 uLightDir;      // Directional light direction
uniform vec3 uLightAmbient;  // Ambient light color
uniform vec3 uLightDiffuse;  // Diffuse light color
uniform vec3 uLightSpecular; // Specular light color
uniform bool isGround;

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);

    // Check angle for Spotlight
    float theta = dot(-lightDir, normalize(light.direction));
    if (theta < light.cutoff) {
        return vec3(0.0);
    }

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterial.shininess);

    // Decaying intensity
    float distance = length(light.position - fragPos);
    float decay = 1.0 / (1.0 + 0.15 * distance + 0.05 * distance * distance);

    vec3 ambient = light.ambient * uMaterial.ambient;
    vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
    vec3 specular = light.specular * spec * uMaterial.specular;

    return decay * (ambient + diffuse + specular);
}

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

    result += calcPointLight(uPointLights[0], normal, tFragPos, viewDir);
    result += calcPointLight(uPointLights[1], normal, tFragPos, viewDir);
    result += calcPointLight(uPointLights[2], normal, tFragPos, viewDir);
    result += calcPointLight(uPointLights[3], normal, tFragPos, viewDir);

    result += uMaterial.emission;

    result = clamp(result, 0.0, 1.0);

    FragColor = vec4(result, 1.0);
}