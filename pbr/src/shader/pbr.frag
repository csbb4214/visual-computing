#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

// Material parameters
uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;

// Lights
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];

uniform vec3 uCamPos;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// Easy to read PBR implementation based on:
// https://learnopengl.com/PBR/Theory
// ----------------------------------------------------------------------------

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);

    // Calculate reflectance at normal incidence
    // For dielectric surfaces use F0 of 0.04 and for metals use the albedo color as F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, uAlbedo, uMetallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i)
    {
        // Calculate per-light radiance
        vec3 L = normalize(uLightPositions[i] - vWorldPos);
        vec3 H = normalize(V + L);
        float distance = length(uLightPositions[i] - vWorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = uLightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, uRoughness);
        float G   = GeometrySmith(N, V, L, uRoughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // For energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // Multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - uMetallic;

        // Scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // Add to outgoing radiance Lo
        Lo += (kD * uAlbedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // Ambient lighting (simple approximation)
    vec3 ambient = vec3(0.03) * uAlbedo * uAO;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}