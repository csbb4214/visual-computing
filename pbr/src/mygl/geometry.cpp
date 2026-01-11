#include "geometry.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mesh createUVSphereMesh(unsigned int xSegments, unsigned int ySegments, float radius) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (unsigned int y = 0; y <= ySegments; ++y) {
        for (unsigned int x = 0; x <= xSegments; ++x) {
            float xSegment = (float)x / (float)xSegments;
            float ySegment = (float)y / (float)ySegments;
            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            Vertex v;
            v.pos = glm::vec3(xPos * radius, yPos * radius, zPos * radius);
            v.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            v.normal = glm::vec3(xPos, yPos, zPos); // For sphere, normal = normalized position
            v.uv = glm::vec2(xSegment, ySegment);

            vertices.push_back(v);
        }
    }

    // Generate indices
    for (unsigned int y = 0; y < ySegments; ++y) {
        for (unsigned int x = 0; x < xSegments; ++x) {
            indices.push_back(y * (xSegments + 1) + x);
            indices.push_back((y + 1) * (xSegments + 1) + x);
            indices.push_back((y + 1) * (xSegments + 1) + x + 1);

            indices.push_back(y * (xSegments + 1) + x);
            indices.push_back((y + 1) * (xSegments + 1) + x + 1);
            indices.push_back(y * (xSegments + 1) + x + 1);
        }
    }

    return meshCreate(vertices, indices);
}