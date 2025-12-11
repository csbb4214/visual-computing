#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uLocalModel;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

uniform bool isWheel;
uniform float uSquashRatio;
uniform float uSquashFactor;

out vec3 tNormal;
out vec3 tFragPos;
out vec2 vTexCoord;
out mat3 vNormalMatrix; // pass normal matrix to fragment shader

void main(void) {
    vec4 pos = uLocalModel * vec4(aPosition, 1.0f);
    if (isWheel) {
        float dispHeight = uSquashRatio * uSquashFactor;
        float disp = dispHeight * (1 - ((pos.y + 1) / 2.0f));
        pos.y += disp;
    }

    gl_Position = uProj * uView * uModel * pos;
    tFragPos = vec3(uModel * pos);

    // Calculate normal matrix for transforming normals
    mat3 normalMatrix = mat3(transpose(inverse(uModel * uLocalModel)));
    vNormalMatrix = normalMatrix;

    // Transform vertex normal to world space
    tNormal = normalize(normalMatrix * aNormal);

    // also pass texture coordinates to fragment shader
    vTexCoord = aUV;
}
