#version 330 core

in vec3 tNormal;
in vec3 tFragPos;
out vec4 FragColor;

uniform vec3 uViewPos;
uniform bool isGround;

void main(void) {
    vec3 normal = normalize(tNormal);

    /* for ground check if normal is facing the camera */
    vec3 viewDir = normalize(uViewPos - tFragPos);
    if (isGround && dot(normal, viewDir) < 0.0) {
        normal = -normal;
    }

    FragColor = vec4((normal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
}