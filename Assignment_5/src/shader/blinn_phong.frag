#version 330 core

struct Light_Directional {
    vec3 direction;

    vec3 ambient;
    vec3 color;
};

struct Light_Spot {
    vec3 position;
    vec3 direction;

    vec3 color;

    float constant;
    float linear;
    float quadratic;

    float cutoff;

    bool enabled;
};

// have these instead of the Material struct
uniform sampler2D map_diffuse;
uniform sampler2D map_ambient;
uniform sampler2D map_specular;
uniform sampler2D map_shininess;
uniform sampler2D map_emission;
uniform sampler2D map_normal;

in vec3 tNormal;
in vec3 tFragPos;
in vec2 vTexCoord;     // receive texture coordinates
in mat3 vNormalMatrix; // receive normal matrix

out vec4 fragColor;

uniform vec3 uViewPos;
uniform Light_Directional uLightSun;
uniform Light_Spot uLightSpots[4];
uniform bool isGround;
uniform bool isWheel;             // for wheel special handling
uniform float uGroundNormalScale; // s parameter for ground blending

vec3 brdf_blinn_phong(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 diffuse, vec3 specular, float shininess) {
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);

    return (diff * diffuse) + (spec * specular);
}

void main(void) {
    vec3 viewDir = normalize(uViewPos - tFragPos);

    // Sample all texture maps
    vec4 diffuseTexel = texture(map_diffuse, vTexCoord);
    vec4 ambientTexel = texture(map_ambient, vTexCoord);
    vec4 specularTexel = texture(map_specular, vTexCoord);
    vec4 shininessTexel = texture(map_shininess, vTexCoord);
    vec4 emissionTexel = texture(map_emission, vTexCoord);
    vec4 normalTexel = texture(map_normal, vTexCoord);

    // Extract material properties from textures
    vec3 materialDiffuse = diffuseTexel.rgb;
    vec3 materialSpecular = specularTexel.rgb;
    vec3 materialEmission = emissionTexel.rgb;

    // Calculate ambient material using ambient occlusion map
    float ambientOcclusion = ambientTexel.r; // grayscale, use red channel
    vec3 materialAmbient = materialDiffuse * ambientOcclusion;

    // Extract shininess from texture
    float materialShininess = shininessTexel.r * 1000.0; // grayscale, use red channel

    // --- NORMAL CALCULATION ---
    // Get vertex normal (already in world space)
    vec3 vertexNormal = normalize(tNormal);

    // Calculate final normal based on object type
    vec3 finalNormal;

    if (isGround) {
        // For ground: transform normal map from object to world space and blend
        // Convert normal from texture space [0,1] to [-1,1]
        vec3 objectSpaceNormal = normalTexel.rgb * 2.0 - 1.0;

        // Transform object space normal to world space using normal matrix
        vec3 worldSpaceNormal = normalize(vNormalMatrix * objectSpaceNormal);

        // Blend as per instructions: n_s = norm(s * n_w + (1-s) * n_v)
        vec3 blendedNormal = uGroundNormalScale * worldSpaceNormal + (1.0 - uGroundNormalScale) * vertexNormal;
        finalNormal = normalize(blendedNormal);

        // For ground check if normal is facing the camera
        if (dot(finalNormal, viewDir) < 0.0) {
            finalNormal = -finalNormal;
        }
    } else if (isWheel) {
        // For wheels, use vertex normals only
        finalNormal = vertexNormal;
    } else {
        // For other objects (pickup body): transform normal map
        // Convert normal from texture space [0,1] to [-1,1]
        vec3 objectSpaceNormal = normalTexel.rgb * 2.0 - 1.0;

        // Transform object space normal to world space using normal matrix
        finalNormal = normalize(vNormalMatrix * objectSpaceNormal);
    }

    // --- LIGHTING CALCULATION ---
    vec3 illuminance = uLightSun.ambient * materialAmbient + materialEmission;
    illuminance += uLightSun.color * brdf_blinn_phong(-uLightSun.direction, viewDir, finalNormal, materialDiffuse, materialSpecular, materialShininess);

    for (int i = 0; i < 4; i++) {
        if (uLightSpots[i].enabled == false)
            continue;

        vec3 lightDir = normalize(uLightSpots[i].position - tFragPos);
        float distance = length(uLightSpots[i].position - tFragPos);
        float attenuation = 1.0 / (uLightSpots[i].constant + uLightSpots[i].linear * distance + uLightSpots[i].quadratic * (distance * distance));

        float angle = acos(dot(-lightDir, uLightSpots[i].direction));
        float intensity = (angle < uLightSpots[i].cutoff) ? 1.0 : 0.0;

        illuminance += intensity * attenuation * uLightSpots[i].color
                       * brdf_blinn_phong(lightDir, viewDir, finalNormal, materialDiffuse, materialSpecular, materialShininess);
    }

    // Use alpha from diffuse texture instead of 1.0
    fragColor = vec4(illuminance, diffuseTexel.a);
}