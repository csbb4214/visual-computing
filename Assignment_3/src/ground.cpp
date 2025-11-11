#include "ground.h"
#include "mygl/geometry.h"

/* calculate displacement of ground at location p */
float computeHeight(const Vector2D &p, const std::vector<WaveParams> &waves) {
    float height = 0.0f;
    for (const auto &w : waves) {
        height += w.amplitude * sinf(w.omega * dot(p, w.direction));
    }
    return height;
}

Vector3D computeColor(const Vector3D &lowColor, const Vector3D &highColor, float t) {
    return lowColor * (1.0f - t) + highColor * t;
}

Ground groundCreate(const Vector3D &color) {
    Ground ground;
    ground.vertices.resize(grid::vertexPos.size());

    std::vector<float> heights(grid::vertexPos.size());
    float minHeight = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::min();

    /* compute heights */
    for (unsigned i = 0; i < ground.vertices.size(); i++) {
        Vector3D pos = grid::vertexPos[i];
        Vector2D pos2D = {pos.x, pos.z};

        float y = computeHeight(pos2D, ground.waveParamsVec);
        heights[i] = y;

        if (y < minHeight) {
            minHeight = y;
        }
        if (y > maxHeight) {
            maxHeight = y;
        }
    }

    Vector3D lowColor = color * 0.5f;
    Vector3D highColor = color * 1.5f;

    /* compute colors */
    for (unsigned i = 0; i < ground.vertices.size(); i++) {
        Vector3D pos = grid::vertexPos[i];
        pos.y = heights[i];

        float t = (heights[i] - minHeight) / (maxHeight - minHeight);
        ground.vertices[i] = {pos, computeColor(lowColor, highColor, t)};
    }
    ground.mesh = meshCreate(ground.vertices, grid::indices, GL_DYNAMIC_DRAW, GL_STATIC_DRAW);

    return ground;
}

void groundDelete(Ground &ground) {
    meshDelete(ground.mesh);
}
