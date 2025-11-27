#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uLocalModel;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

// squash parameters (defined in pickup.h)
uniform float uSquashFactor;
uniform float uSquashRatio;

out vec3 tNormal;
out vec3 tFragPos;

void main(void) {
    // Apply wheel rotation and local transformation first
    vec4 localPosition = uLocalModel * vec4(aPosition, 1.0);

    // Simple scaling-based squashing (alternative approach)
    if (uSquashFactor > 0.0) {
        // Scale y-axis based on squash factor
        float scaleY = 1.0 - (uSquashRatio * uSquashFactor);
        localPosition.y *= scaleY;

        // Compensate x and z axes to maintain volume (optional)
        // float scaleXZ = 1.0 / sqrt(scaleY); // for volume preservation
        // localPosition.xz *= scaleXZ;
    }

    // Continue with normal transformation pipeline
    gl_Position = uProj * uView * uModel * localPosition;
    tFragPos = vec3(uModel * localPosition);
    tNormal = normalize(mat3(transpose(inverse(uModel))) * mat3(transpose(inverse(uLocalModel))) * aNormal);
}
