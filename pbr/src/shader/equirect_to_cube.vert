#version 330 core
layout (location = 0) in vec3 aPosition;

out vec3 vLocalPos;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    vLocalPos = aPosition;
    gl_Position = uProjection * uView * vec4(vLocalPos, 1.0);
}