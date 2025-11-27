#include <algorithm>
#include <stdexcept>

#include "ground.h"
#include <iostream>

float getDisplacementValue(WaveParams waveParams, const Vector2D &position) {
    return waveParams.amplitude * static_cast<float>(sin(dot(normalize(waveParams.direction), {position.x, position.y}) * waveParams.omega));
}

float groundDisplacement(const std::vector<WaveParams> &waveParamsVec, const Vector2D &position) {
    float displacement = 0.0f;
    for (WaveParams waveParams : waveParamsVec) {
        displacement += getDisplacementValue(waveParams, position);
    }
    return displacement;
}

Ground groundCreate(const std::string& filePath) {
    /* load ground model */
    std::vector<Model> models = modelLoad(filePath);
    if(models.size() != 1) {
        throw std::runtime_error("[Ground] not one object!" + std::to_string(models.size()));
    }
    
    Ground ground;
    ground.model = models[0];

    /* bind the vertex buffer to update positions and normals */
    glBindBuffer(GL_ARRAY_BUFFER, ground.model.mesh.vbo);

    /* get the size of the buffer */
    GLint sizeInBytes = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &sizeInBytes);

    /* get a pointer to the buffer to read and write */
    float* vertexData = (float*) glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeInBytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    /* keep the memory layout in mind! One Vertex struct contains 8 floats: 
     *    position (3),        normal (3),                   uv (2)
     *   [pos.x, pos.y, pos.z, normal.x, normal.y, normal.z, uv.x, uv.y]
     */

    if (vertexData) {
        int numFloats = sizeInBytes / sizeof(GL_FLOAT);

        /* Transform the grid's vertices */
        for (int i = 0; i < numFloats / 8; i++) {
            /* Get x and z position of vertex */
            float x = vertexData[i * 8];
            float z = vertexData[i * 8 + 2];

            /* Update y position based on wave displacement */
            float displacement = groundDisplacement(ground.waveParamsVec, {x, z});
            vertexData[i * 8 + 1] += displacement;
            
            /* Update normals */ 
            // Calculate partial derivatives for each wave
            float dh_dx = 0.0f;
            float dh_dz = 0.0f;

            for (WaveParams waveParams : ground.waveParamsVec) {
                Vector2D normalizedDir = normalize(waveParams.direction);
                float dotProduct = dot(normalizedDir, {x, z});
    
                // Partial derivative with respect to x
                dh_dx += waveParams.amplitude * waveParams.omega * normalizedDir.x * 
                        static_cast<float>(cos(waveParams.omega * dotProduct));
    
                // Partial derivative with respect to z (=y in the mathematical notation)
                dh_dz += waveParams.amplitude * waveParams.omega * normalizedDir.y * 
                        static_cast<float>(cos(waveParams.omega * dotProduct));
            }

            // Computed normal using cross product: n = -(1, dh/dx, 0) x (0, dh/dz, 1)
            Vector3D normal = {
                -dh_dx,
                1.0f,
                -dh_dz
            };

            // Write the normalized normal to the buffer
            normal = normalize(normal);

            vertexData[i * 8 + 3] = normal.x;
            vertexData[i * 8 + 4] = normal.y;
            vertexData[i * 8 + 5] = normal.z;
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    return ground;
}

void groundDelete(Ground &ground) { 
    modelDelete(ground.model);
}
