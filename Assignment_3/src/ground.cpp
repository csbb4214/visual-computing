#include "ground.h"
#include "mygl/geometry.h"

Ground groundCreate(const Vector3D &color) {
    Ground ground;
    ground.vertices.resize(grid::vertexPos.size());

    /* Transform the grid's vertices and add them to the flag with the correct color */
    for (unsigned i = 0; i < ground.vertices.size(); i++) {
        ground.vertices[i] = {grid::vertexPos[i], color};
    }
    ground.mesh = meshCreate(ground.vertices, grid::indices, GL_DYNAMIC_DRAW, GL_STATIC_DRAW);

    return ground;
}

void groundDelete(Ground &ground) { 
    meshDelete(ground.mesh);
}
