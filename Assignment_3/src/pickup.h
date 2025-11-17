#pragma once

#include "mygl/mesh.h"
#include "mygl/geometry.h"

struct Pickup {
    // Meshes
    Mesh base;
    Mesh cockpit;
    Mesh wheelFL, wheelFR, wheelRL, wheelRR;
    Mesh spare;

    // Model matrices (relative to Pickup-origin)
    Matrix4D modelBaseLocal;
    Matrix4D modelCockpitLocal;
    Matrix4D modelWheelFLBase, modelWheelFRBase, modelWheelRLBase, modelWheelRRBase;
    Matrix4D modelSpareLocal;

    // Global pickup transformation in world space
    Matrix4D vehicleTransform;

    // Pickup base measures
    float baseLength;
    float baseHeight;
    float baseWidth;
    float baseY;

    // Wheel measures
    float wheelBaseHalf;
    float wheelTrack;
    float frontWheelRadius;
    float rearWheelRadius;
    float wheelThickness;

    float wheelBase;
    float width;

    // Wheel animations
    float wheelRotationAngle;    // Driving animation
    float wheelSteeringAngle;    // steering animation
};

/* Create a pickup truck with specified colors */
Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels);

/* Delete pickup and free resources */
void pickupDelete(Pickup &pickup);

/* Draw the entire pickup truck */
void pickupDraw(const Pickup &pickup, ShaderProgram &shader);

/* Update pickup transform based on input */
void pickupUpdate(
    Pickup &pickup,
    float moveSpeed,
    float maxSteeringAngleRad,
    float turningAnglePerMeterDeg,
    float dt,
    bool moveForward,
    bool moveBackward,
    bool turnLeft,
    bool turnRight
);

/* Adjust pickup to ground */
void pickupAdjustToTerrain(Pickup &pickup, const Ground &ground);

/* Helperfunction for camera follow */
Vector3D pickupGetWorldPosition(const Pickup &pickup);