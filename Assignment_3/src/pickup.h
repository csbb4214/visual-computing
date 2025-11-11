#pragma once

#include "mygl/mesh.h"
#include "mygl/geometry.h"

struct Pickup {
    // Meshes
    Mesh base;
    Mesh cockpit;
    Mesh wheelFL, wheelFR, wheelRL, wheelRR;
    Mesh spare;

    // Final world model matrices (werden pro Frame neu gebaut)
    Matrix4D modelBase;
    Matrix4D modelCockpit;
    Matrix4D modelWheelFL, modelWheelFR, modelWheelRL, modelWheelRR;
    Matrix4D modelSpare;

    // Geometrie / Maße
    float baseLength;       // ~ Fahrzeuglänge
    float baseHeight;       // Höhe der Basis
    float baseWidth;        // Breite
    float baseY;            // y-Position der Basis (Mitte)

    float wheelBaseHalf;    // halber Achsabstand (nur zur internen Berechnung)
    float wheelTrack;       // Abstand zwischen linker und rechter Radseite
    float frontWheelRadius;
    float rearWheelRadius;
    float wheelThickness;

    // Für calculateTurningAnglePerMeter
    float wheelBase;        // Abstand Vorderachse–Hinterachse
    float width;            // Fahrzeugbreite (für Turning-Berechnung)

    // Dynamischer Zustand (für Aufgabe 2)
    Vector3D position = {0.0f, 0.0f, 0.0f}; // Weltposition (Pickup-Origin)
    float rotationY = 0.0f;                 // Yaw des Fahrzeugs
    float steeringAngle = 0.0f;             // Lenkwinkel der Vorderräder (rad)
    float wheelSpinFront = 0.0f;            // Rotationswinkel der Vorderräder (rad)
    float wheelSpinRear  = 0.0f;            // Rotationswinkel der Hinterräder (rad)
};

/* Create a pickup truck with specified colors */
Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels);

/* Delete pickup and free resources */
void pickupDelete(Pickup &pickup);

/* Draw the entire pickup truck */
void pickupDraw(const Pickup &pickup, ShaderProgram &shader);

/* Update pickup movement and wheel animation based on input (Task 2) */
void pickupUpdate(
    Pickup &pickup,
    float moveSpeed,                 // Fahrgeschwindigkeit [Einheiten/s]
    float maxSteeringAngleRad,       // maximaler Lenkwinkel der Räder [rad]
    float turningAnglePerMeterDeg,   // wie stark das Auto pro Meter dreht [deg/m]
    float dt,                        // Delta time seit letztem Frame
    bool moveForward,                // W
    bool moveBackward,               // S
    bool turnLeft,                   // A
    bool turnRight                   // D
);
