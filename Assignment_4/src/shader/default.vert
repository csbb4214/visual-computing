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
    // Apply local transformation (includes wheel rotation)
    vec4 localPosition = uLocalModel * vec4(aPosition, 1.0);

    // Apply squash and stretch deformation
    if (uSquashFactor > 0.0) {
        float squashAmount = uSquashFactor * uSquashRatio;

        // Compress vertically (scale Y toward 0)
        localPosition.y *= (1.0 - squashAmount);

        // Expand horizontally (scale XZ away from center)
        float horizontalExpansion = 1.0 + squashAmount * 0.5;
        localPosition.x *= horizontalExpansion;
        localPosition.z *= horizontalExpansion;
    }

    // Continue with world transformation
    vec4 worldPosition = uModel * localPosition;
    gl_Position = uProj * uView * worldPosition;

    tFragPos = worldPosition.xyz;
    tNormal = normalize(mat3(transpose(inverse(uModel * uLocalModel))) * aNormal);
}
