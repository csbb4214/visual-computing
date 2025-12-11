#pragma once

#include "mygl/base.h"
#include "mygl/model.h"

struct WaveParams {
    float amplitude;
    // float phi;
    float omega;
    Vector2D direction;
};

struct Ground {
    Model model;
    std::vector<Vertex> vertices;

    float minPosZ;

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
 * @param filePath Path to a file containing the ground mesh.
 *
 * @return Object containing the vector of vertices and an initialized mesh structure that can be drawn with OpenGL.
 *
 * usage:
 *
 *   Ground myGround = groundCreate({0.15f, 0.3f, 0.17f}, {0.15f, 0.45f, 0.15f});
 *   glBindVertexArray(myGround.mesh.vao);
 *   glDrawElements(GL_TRIANGLES, myGround.size_ibo, GL_UNSIGNED_INT, nullptr);
 *
 */
Ground groundCreate(const std::string& filePath);

/**
 * @brief Cleanup and delete all OpenGL buffers of the ground mesh.
 *
 * @param ground ground to delete.
 */
void groundDelete(Ground &ground);

/**
 * @brief Computes the displacement of the ground at the given position according to the wave parameters.
 *
 * @param waveParamsVec Vector of wave parameters defining the waves affecting the ground.
 * @param position 2D position on the ground where the displacement should be computed.
 */
float groundDisplacement(const std::vector<WaveParams> &waveParamsVec, const Vector2D &position);

/**
 * @brief Computes the normal vector of the ground at the given position according to the wave parameters.
 *
 * @param waveParamsVec Vector of wave parameters defining the waves affecting the ground.
 * @param position 2D position on the ground where the normal should be computed.
 */
Vector3D groundNormal(const std::vector<WaveParams> &waveParamsVec, const Vector2D &pos);