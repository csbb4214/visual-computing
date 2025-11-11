#pragma once

#include "mygl/mesh.h"
#include "mygl/geometry.h"

struct Pickup {
    // Meshes
    Mesh base;
    Mesh cockpit;
    Mesh wheelFL, wheelFR, wheelRL, wheelRR;
    Mesh spare;

    // Lokale Modellmatrizen (relativ zum Pickup-Ursprung)
    Matrix4D modelBaseLocal;
    Matrix4D modelCockpitLocal;
    Matrix4D modelWheelFLBase, modelWheelFRBase, modelWheelRLBase, modelWheelRRBase;
    Matrix4D modelSpareLocal;

    // Globale Transformationsmatrix des Fahrzeugs (Weltmatrix)
    Matrix4D vehicleTransform;

    // Truck dimensions (aus der Basis abgeleitet)
    float baseLength;    // 4.0
    float baseHeight;    // 1.0
    float baseWidth;     // 1.5
    float baseY;         // 2.0 (center Y position)

    // Rad-Parameter (für calculateTurningAnglePerMeter)
    float wheelBaseHalf;     // halber Radstand
    float wheelTrack;        // Abstand links–rechts
    float frontWheelRadius;
    float rearWheelRadius;
    float wheelThickness;

    float wheelBase;         // voller Radstand
    float width;             // Fahrzeugbreite

    // Neue Variablen für Radanimation und Lenkung
    float wheelRotationAngle;    // Rotationswinkel aller Räder (um ihre eigene Achse)
    float wheelSteeringAngle;    // Lenkwinkel der Vorderräder (nur Vorderräder)
};

/* Create a pickup truck with specified colors */
Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels);

/* Delete pickup and free resources */
void pickupDelete(Pickup &pickup);

/* Draw the entire pickup truck */
void pickupDraw(const Pickup &pickup, ShaderProgram &shader);

/* Update pickup transform based on input (Task 2) */
void pickupUpdate(
    Pickup &pickup,
    float moveSpeed,
    float maxSteeringAngleRad,      // für Lenkbegrenzung der Vorderräder
    float turningAnglePerMeterDeg,  // wie stark dreht der Wagen pro Meter
    float dt,
    bool moveForward,
    bool moveBackward,
    bool turnLeft,
    bool turnRight
);

/* Optionale Hilfsfunktion: Fahrzeugposition aus vehicleTransform für Kamera-Follow */
Vector3D pickupGetWorldPosition(const Pickup &pickup);

void pickupAdjustToTerrain(Pickup &pickup, const Ground &ground);