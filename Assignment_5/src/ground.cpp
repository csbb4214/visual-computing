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

Vector2D getWaveGradient(const WaveParams &waveParams, const Vector2D &pos) {
    float dx = waveParams.amplitude * waveParams.direction.x * waveParams.omega * cos(dot(normalize(waveParams.direction), pos) * waveParams.omega);
    float dy = waveParams.amplitude * waveParams.direction.y * waveParams.omega * cos(dot(normalize(waveParams.direction), pos) * waveParams.omega);

    return {dx, dy};
}

Vector3D groundNormal(const std::vector<WaveParams> &waveParamsVec, const Vector2D &pos) {
    Vector2D gradSum = { 0.0f, 0.0f };
    for (const WaveParams &wp : waveParamsVec) {
        gradSum += getWaveGradient(wp, pos);
    }

    return normalize(-cross({ 1, gradSum.x, 0 }, { 0, gradSum.y, 1 }));
}

Ground groundCreate(const std::string& filePath) {
    std::vector<Model> models = modelLoad(filePath);
    
    if(models.size() != 1) {
        throw std::runtime_error("[Ground] not one object!" + std::to_string(models.size()));
    }
    
    Ground ground;
    ground.model = models[0];

    glBindBuffer(GL_ARRAY_BUFFER, ground.model.mesh.vbo);

    GLint sizeInBytes = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &sizeInBytes);

    float* data = (float*) glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeInBytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

    if (data) {
        int numFloats = sizeInBytes / sizeof(GL_FLOAT);

        /* Transform the grid's vertices and add them to the flag with the correct color */
        for (int i = 0; i < numFloats / 9; i++) {
            float x = data[i * 9];
            float z = data[i * 9 + 2];
            float displacement = groundDisplacement(ground.waveParamsVec, {x, z});

            Vector3D normal = groundNormal(ground.waveParamsVec, {x, z});
            
            data[i * 9 + 1] += displacement;
            data[i * 9 + 3] = normal.x;
            data[i * 9 + 4] = normal.y;
            data[i * 9 + 5] = normal.z;

            data[i * 9 + 7] = data[i * 9 + 7] * 20.0f;
            data[i * 9 + 8] = data[i * 9 + 8] * 20.0f;
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    return ground;
}

void groundDelete(Ground &ground) { 
    modelDelete(ground.model);
}
