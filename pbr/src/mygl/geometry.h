#pragma once

#include "mesh.h"
#include <glm/glm.hpp>
#include <vector>

Mesh createUVSphereMesh(unsigned int xSegments, unsigned int ySegments, float radius);

Mesh createCubeMesh();
Mesh createQuadMesh();