#include <cstdlib>
#include <iostream>
#include <cmath> // für fabs

#include "mygl/mesh.h"
#include "mygl/shader.h"

#include "pickup.h"

/* -------------------------------------------------------
 * Pickup erstellen: Geometrie + lokale Modelmatrizen
 * ----------------------------------------------------- */

Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels) {
    Pickup pickup;

    // Basis-Maße
    pickup.baseLength = 4.0f;
    pickup.baseHeight = 1.0f;
    pickup.baseWidth  = 1.5f;
    pickup.baseY      = 2.0f;

    // Rad-Parameter
    pickup.wheelBaseHalf     = pickup.baseLength / 2.0f;
    pickup.wheelTrack        = pickup.baseWidth + 2.0f; // Weiter außen
    pickup.frontWheelRadius  = 0.7f;
    pickup.rearWheelRadius   = 1.0f;
    pickup.wheelThickness    = 0.2f;

    pickup.wheelBase = 2.0f * pickup.wheelBaseHalf;
    pickup.width     = pickup.wheelTrack;

    // Globale Transformationsmatrix: Start bei Identität an Weltursprung
    pickup.vehicleTransform = Matrix4D::identity();

    // Initialisiere Radrotationen
    pickup.wheelRotationAngle = 0.0f;
    pickup.wheelSteeringAngle = 0.0f;

    // Meshes
    pickup.base     = meshCreate(cube::vertexPos,     cube::indices,     colorBase,    GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.cockpit  = meshCreate(cube::vertexPos,     cube::indices,     colorCockpit, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFL  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFR  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRL  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRR  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.spare    = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);

    // ---------- lokale Modelmatrizen (im Pickup-eigenen Koordinatensystem) ----------

    // Base
    pickup.modelBaseLocal =
        Matrix4D::translation({0.0f, pickup.baseY, 0.0f}) *
        Matrix4D::scale(pickup.baseLength, pickup.baseHeight, pickup.baseWidth);

    // Cockpit
    float cockpitX = pickup.baseLength / 4.0f;
    float cockpitY = pickup.baseY + pickup.baseHeight + 1.0f;
    pickup.modelCockpitLocal =
        Matrix4D::translation({cockpitX, cockpitY, 0.0f}) *
        Matrix4D::scale(1.0f, 1.0f, pickup.baseWidth);

    // Räder - Basispositionen ohne Rotation
    float wheelBaseHalf = pickup.wheelBaseHalf;
    float wheelTrack    = pickup.wheelTrack;

    // Positionen korrigiert - näher am Auto und auf richtiger Höhe
    float frontWheelX = wheelBaseHalf * 1.3f;
    float rearWheelX  = -wheelBaseHalf * 0.8f;

    float frontR    = pickup.frontWheelRadius;
    float rearR     = pickup.rearWheelRadius;
    float thickness = pickup.wheelThickness;

    // Einfache und klare Rad-Transformation:
    // Zylinder wird um Y-Achse gedreht, um ihn flach zu legen
    // Dann wird er entlang X gestreckt für die Dicke
    Matrix4D wheelRot = Matrix4D::rotationY(to_radians(90.0f));
    Matrix4D wheelFScale = Matrix4D::scale(thickness, frontR, frontR);
    Matrix4D wheelRScale = Matrix4D::scale(thickness, rearR, rearR);

    // Radhöhe korrigiert - Räder sollen auf dem Boden stehen
    float wheelY = pickup.frontWheelRadius; // Räder stehen auf y=0 + Radius

    // Vorderräder Basispositionen - weiter außen
    pickup.modelWheelFLBase =
        Matrix4D::translation({frontWheelX, wheelY, -wheelTrack / 2.0f}) *
        wheelRot * wheelFScale;

    pickup.modelWheelFRBase =
        Matrix4D::translation({frontWheelX, wheelY, wheelTrack / 2.0f}) *
        wheelRot * wheelFScale;

    // Hinterräder Basispositionen - weiter außen
    pickup.modelWheelRLBase =
        Matrix4D::translation({rearWheelX, wheelY, -wheelTrack / 2.0f}) *
        wheelRot * wheelRScale;

    pickup.modelWheelRRBase =
        Matrix4D::translation({rearWheelX, wheelY, wheelTrack / 2.0f}) *
        wheelRot * wheelRScale;

        // Ersatzrad – horizontal auf der Ladefläche am Heck
float spareX = -pickup.baseLength * 1.0f - pickup.wheelThickness;
float spareY = pickup.frontWheelRadius + 2.0f;
float spareZ = 0.0f;

// Nur um X rotieren für querstehenden Reifen
Matrix4D spareRot = Matrix4D::rotationX(to_radians(90.0f));

pickup.modelSpareLocal =
    Matrix4D::translation({spareX, spareY, spareZ}) *
    spareRot *
    Matrix4D::scale(pickup.wheelThickness, pickup.frontWheelRadius, pickup.frontWheelRadius);

    return pickup;
}

/* -------------------------------------------------------
 * Zeichnen: vehicleTransform * lokaleModelMatrix
 * ----------------------------------------------------- */

void pickupDraw(const Pickup &pickup, ShaderProgram &shader) {
    const Matrix4D &W = pickup.vehicleTransform;

    // Base
    shaderUniform(shader, "uModel", W * pickup.modelBaseLocal);
    glBindVertexArray(pickup.base.vao);
    glDrawElements(GL_TRIANGLES, pickup.base.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Cockpit
    shaderUniform(shader, "uModel", W * pickup.modelCockpitLocal);
    glBindVertexArray(pickup.cockpit.vao);
    glDrawElements(GL_TRIANGLES, pickup.cockpit.size_ibo, GL_UNSIGNED_INT, nullptr);

    // --- Radrotationen ---
    Matrix4D roll = Matrix4D::rotationX(pickup.wheelRotationAngle);
    Matrix4D steering = Matrix4D::rotationY(pickup.wheelSteeringAngle);
    Matrix4D wheelTilt = Matrix4D::rotationY(to_radians(90.0f)); // Zylinder-Achse anpassen

    // --- Vorderräder ---
    float wheelTrack = pickup.wheelTrack;
    float wheelBaseHalf = pickup.wheelBaseHalf;
    float wheelY = pickup.frontWheelRadius;
    float frontWheelX = wheelBaseHalf * 1.3f;
    float thickness = pickup.wheelThickness;
    float frontR = pickup.frontWheelRadius;

    // Gemeinsame Skalierung
    Matrix4D scaleF = Matrix4D::scale(thickness, frontR, frontR);

    // Linkes Vorderrad
    {
        Matrix4D local =
            Matrix4D::translation({frontWheelX, wheelY, -wheelTrack / 2.0f}) *
            steering * wheelTilt * roll * scaleF;
        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelFL.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelFL.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    // Rechtes Vorderrad
    {
        Matrix4D local =
            Matrix4D::translation({frontWheelX, wheelY,  wheelTrack / 2.0f}) *
            steering * wheelTilt * roll * scaleF;
        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelFR.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelFR.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    // --- Hinterräder ---
    float rearWheelX = -wheelBaseHalf * 0.8f;
    float rearR = pickup.rearWheelRadius;
    Matrix4D scaleR = Matrix4D::scale(thickness, rearR, rearR);

    // Links hinten
    {
        Matrix4D local =
            Matrix4D::translation({rearWheelX, rearR, -wheelTrack / 2.0f}) *
            wheelTilt * roll * scaleR;
        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelRL.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelRL.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    // Rechts hinten
    {
        Matrix4D local =
            Matrix4D::translation({rearWheelX, rearR,  wheelTrack / 2.0f}) *
            wheelTilt * roll * scaleR;
        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelRR.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelRR.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    // --- Ersatzrad ---
    shaderUniform(shader, "uModel", W * pickup.modelSpareLocal);
    glBindVertexArray(pickup.spare.vao);
    glDrawElements(GL_TRIANGLES, pickup.spare.size_ibo, GL_UNSIGNED_INT, nullptr);
}


/* -------------------------------------------------------
 * Ressourcen freigeben
 * ----------------------------------------------------- */

void pickupDelete(Pickup &pickup) {
    meshDelete(pickup.base);
    meshDelete(pickup.cockpit);
    meshDelete(pickup.wheelFL);
    meshDelete(pickup.wheelFR);
    meshDelete(pickup.wheelRL);
    meshDelete(pickup.wheelRR);
    meshDelete(pickup.spare);
}

/* -------------------------------------------------------
 * Bewegung + Rotation mit Radbewegungen
 * ----------------------------------------------------- */

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
) {
    float dir = 0.0f;
    if (moveForward)  dir += 1.0f;
    if (moveBackward) dir -= 1.0f;

    float distance = dir * moveSpeed * dt;

    int turnSign = 0;
    if (turnLeft)  turnSign += 1;
    if (turnRight) turnSign -= 1;

    // Radrotation basierend auf zurückgelegter Strecke
    if (std::fabs(distance) > 1e-6f) {
        // Umfang = 2 * π * Radius, Winkel = (Strecke / Umfang) * 2π
        float circumferenceFront = 2.0f * M_PI * pickup.frontWheelRadius;
        float rotationAngle = (distance / circumferenceFront) * 2.0f * M_PI;
        
        pickup.wheelRotationAngle += rotationAngle;
        
        // Winkel auf sinnvollen Bereich beschränken
        if (pickup.wheelRotationAngle > 2.0f * M_PI) {
            pickup.wheelRotationAngle -= 2.0f * M_PI;
        } else if (pickup.wheelRotationAngle < -2.0f * M_PI) {
            pickup.wheelRotationAngle += 2.0f * M_PI;
        }
    }

    // Lenkung aktualisieren - stärkere und schnellere Lenkung
    if (turnSign != 0) {
        float steeringChange = turnSign * maxSteeringAngleRad * 4.0f * dt; // Viel stärkere Lenkung
        pickup.wheelSteeringAngle += steeringChange;
        
        // Lenkwinkel begrenzen
        if (pickup.wheelSteeringAngle > maxSteeringAngleRad) {
            pickup.wheelSteeringAngle = maxSteeringAngleRad;
        } else if (pickup.wheelSteeringAngle < -maxSteeringAngleRad) {
            pickup.wheelSteeringAngle = -maxSteeringAngleRad;
        }
    } else {
        // Lenkung zurückstellen (Centerung)
        if (std::fabs(pickup.wheelSteeringAngle) > 0.01f) {
            pickup.wheelSteeringAngle *= 0.8f; // Schnelleres Zurückstellen
        } else {
            pickup.wheelSteeringAngle = 0.0f;
        }
    }

    // 1) Zuerst Translation in lokaler +X-Richtung (Fahrtrichtung)
    if (std::fabs(distance) > 1e-6f) {
        Matrix4D T = Matrix4D::translation({distance, 0.0f, 0.0f});
        pickup.vehicleTransform = pickup.vehicleTransform * T;
    }

    // 2) Rotation um die lokale Y-Achse (Lenkung) – hängt von zurückgelegter Distanz ab
    if (turnSign != 0 && std::fabs(distance) > 1e-6f) {
        float headingChangeDeg = turningAnglePerMeterDeg * distance * static_cast<float>(turnSign);
        float headingChangeRad = to_radians(headingChangeDeg);
        
        // Rotation um die lokale Y-Achse durch Rechtsmultiplikation
        Matrix4D R = Matrix4D::rotationY(headingChangeRad);
        pickup.vehicleTransform = pickup.vehicleTransform * R;
    }
}

/* -------------------------------------------------------
 * Hilfsfunktion: Fahrzeugposition aus Transformationsmatrix
 * (für Kamera-Follow)
 * ----------------------------------------------------- */

Vector3D pickupGetWorldPosition(const Pickup &pickup) {
    const Matrix4D &M = pickup.vehicleTransform;

    // Ursprungsvektor in homogenen Koordinaten (x, y, z, w = 1)
    Vector4D origin4(0.0f, 0.0f, 0.0f, 1.0f);

    // Transformiere den Ursprung mit der Fahrzeugmatrix
    Vector4D worldPos4 = M * origin4;

    // Ergebnis zurück als 3D-Vektor (xyz)
    return Vector3D(worldPos4.x, worldPos4.y, worldPos4.z);
}