#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vWorldPos = vec3(uModel * vec4(aPosition, 1.0));
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vUV = aUV;
    
    gl_Position = uProj * uView * vec4(vWorldPos, 1.0);
}