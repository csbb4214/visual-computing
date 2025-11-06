#pragma once

#include "mygl/base.h"
#include "mygl/mesh.h"

struct WaveParams {
    float amplitude;
    float omega;
    Vector2D direction;
};

struct Ground {
    Mesh mesh;
    std::vector<Vertex> vertices;

    std::vector<WaveParams> waveParamsVec = {
        {0.9f, 0.35f, normalize(Vector2D{0.0f, 1.0f})},
        {0.7f, 0.4f, normalize(Vector2D{1.0f, 0.0f})},
        {1.1f, 0.1f, normalize(Vector2D{-2.0f, 1.0f})},
        {0.3f, 0.8f, normalize(Vector2D{-1.0f, -2.5f})},
    };
};

/**
 * @brief Initializes a plane grid to visualize the ground surface. For that a vector containing all grid vertices is created and
 * a mesh (see function meshCreate(...)) is setup with these vertices.
 *
 * @param color Color of the ground.
 *
 * @return Object containing the vector of vertices and an initialized mesh structure that can be drawn with OpenGL.
 *
 * usage:
 *
 *   Ground myGround = groundCreate({0.15f, 0.45f, 0.15f});
 *   glBindVertexArray(myGround.mesh.vao);
 *   glDrawElements(GL_TRIANGLES, myGround.size_ibo, GL_UNSIGNED_INT, nullptr);
 *
 */
Ground groundCreate(const Vector3D &color);

/**
 * @brief Cleanup and delete all OpenGL buffers of the ground mesh.
 *
 * @param ground ground to delete.
 */
void groundDelete(Ground &ground);
