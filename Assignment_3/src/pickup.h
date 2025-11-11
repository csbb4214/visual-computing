#pragma once

#include "mygl/mesh.h"
#include "mygl/geometry.h"

struct Pickup {
    Mesh base;
    Mesh cockpit;
    Mesh wheelFL, wheelFR, wheelRL, wheelRR;
    Mesh spare;

    Matrix4D modelBase;
    Matrix4D modelCockpit;
    Matrix4D modelWheelFL, modelWheelFR, modelWheelRL, modelWheelRR;
    Matrix4D modelSpare;

    // Truck dimensions (derived from base)
    float baseLength;    // 4.0
    float baseHeight;    // 1.0
    float baseWidth;     // 1.5
    float baseY;         // 2.0 (center Y position)
};

/* Create a pickup truck with specified colors */
Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels);

/* Delete pickup and free resources */
void pickupDelete(Pickup &pickup);

/* Draw the entire pickup truck */
void pickupDraw(const Pickup &pickup, ShaderProgram &shader);