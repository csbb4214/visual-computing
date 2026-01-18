#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

// --- Material (Uniform-Fallback) ---
uniform vec3  uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;

// --- Optional Textures ---
uniform bool uUseTextures;
uniform sampler2D uAlbedoMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAOMap;

// --- IBL ---
uniform bool uUseIBL;
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D uBRDFLUT;

// Lights
uniform vec3 uLightPositions[5];
uniform vec3 uLightColors[5];

uniform vec3 uCamPos;

// Tone mapping
uniform float uExposure;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

// ----------------------------------------------------------------------------
// PBR helper functions
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 1e-6);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 1e-6);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------
void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 R = reflect(-V, N);

    // --- Pick material parameters (textures or uniforms) ---
    vec3  albedo    = uAlbedo;
    float metallic  = uMetallic;
    float roughness = uRoughness;
    float ao        = uAO;

    if (uUseTextures) {
        albedo    = texture(uAlbedoMap, vUV).rgb;
        metallic  = texture(uMetallicMap, vUV).r;
        roughness = texture(uRoughnessMap, vUV).r;
        ao        = texture(uAOMap, vUV).r;
    }

    // Robustness
    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = clamp(metallic, 0.0, 1.0);
    ao        = clamp(ao, 0.0, 1.0);

    // F0: dielectric 0.04, metals use albedo
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // ----------------------------------------------------------------------------
    // Direct lighting (point lights)
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < 5; ++i)
    {
        vec3 L = normalize(uLightPositions[i] - vWorldPos);
        vec3 H = normalize(V + L);

        float distance    = length(uLightPositions[i] - vWorldPos);
        float attenuation = 1.0 / max(distance * distance, 1e-6);
        vec3 radiance     = uLightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denom       = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denom, 1e-6);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // ----------------------------------------------------------------------------
    // Ambient lighting (IBL)
    vec3 ambient = vec3(0.0);
    
    if (uUseIBL) {
        // Fresnel for IBL
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;
        
        // Diffuse IBL
        vec3 irradiance = texture(uIrradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedo;
        
        // Specular IBL
        vec3 prefilteredColor = textureLod(uPrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(uBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        
        ambient = (kD * diffuse + specular) * ao;
    } else {
        // Simple ambient fallback
        ambient = vec3(0.03) * albedo * ao;
    }

    // ----------------------------------------------------------------------------
    vec3 color = ambient + Lo;

    // Exposure tone mapping
    color = vec3(1.0) - exp(-color * uExposure);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}