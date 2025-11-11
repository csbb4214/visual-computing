#include <cstdlib>
#include <iostream>
#include <cmath> // für sin, cos, fabs

#include "mygl/mesh.h"
#include "mygl/shader.h"

#include "pickup.h"

// Hilfsfunktion: baut alle Modelmatrizen aus dem aktuellen Zustand
static void pickupBuildModelMatrices(Pickup &pickup) {
    // Welttransformation aus Position + Yaw
    Matrix4D world =
        Matrix4D::translation(pickup.position) *
        Matrix4D::rotationY(pickup.rotationY);

    // Basis
    Matrix4D baseLocal =
        Matrix4D::translation({0.0f, pickup.baseY, 0.0f}) *
        Matrix4D::scale(pickup.baseLength, pickup.baseHeight, pickup.baseWidth);

    pickup.modelBase = world * baseLocal;

    // Cockpit (wie in deiner ursprünglichen Version)
    float cockpitX = pickup.baseLength / 4.0f;
    float cockpitY = pickup.baseY + pickup.baseHeight + 1.0f;

    Matrix4D cockpitLocal =
        Matrix4D::translation({cockpitX, cockpitY, 0.0f}) *
        Matrix4D::scale(1.0f, 1.0f, pickup.baseWidth);

    pickup.modelCockpit = world * cockpitLocal;

    // Rad-Geometrie & Matrizen
    float wheelBaseHalf = pickup.wheelBaseHalf;
    float wheelTrack    = pickup.wheelTrack;
    float frontR        = pickup.frontWheelRadius;
    float rearR         = pickup.rearWheelRadius;
    float thickness     = pickup.wheelThickness;

    float frontWheelX   = wheelBaseHalf * 1.4f;
    float rearWheelX    = -wheelBaseHalf * 1.0f;

    // Zylinder-Achse ist entlang z → wir drehen um Y 90°, damit sie entlang x zeigt
    Matrix4D wheelRot      = Matrix4D::rotationY(to_radians(90.0f));
    Matrix4D wheelFScale = Matrix4D::scale(frontR, thickness, frontR);
    Matrix4D wheelRScale = Matrix4D::scale(rearR, thickness, rearR);
    // Lenkung (Vorderräder)
    Matrix4D steerRot = Matrix4D::rotationY(pickup.steeringAngle);

    // Drehen um Z-Achse (lokale Achse des Zylinders → nach wheelRot wird das zur x-Achse)
    Matrix4D spinFront = Matrix4D::rotationX(pickup.wheelSpinFront);
    Matrix4D spinRear  = Matrix4D::rotationX(pickup.wheelSpinRear);
    pickup.modelWheelFL =
        world *
        Matrix4D::translation({frontWheelX, frontR, -wheelTrack / 2.0f}) *
        Matrix4D::rotationZ(to_radians(90.0f)) *           // Zylinder von Y auf X kippen
        Matrix4D::rotationX(pickup.steeringAngle) *        // Lenkung (um die lokale X-Achse)
        Matrix4D::rotationX(pickup.wheelSpinFront) *       // Rollbewegung
        wheelFScale;

    // Vorderachse rechts
    pickup.modelWheelFR =
        world *
        Matrix4D::translation({frontWheelX, frontR, wheelTrack / 2.0f}) *
        Matrix4D::rotationZ(to_radians(90.0f)) *
        Matrix4D::rotationX(pickup.steeringAngle) *
        Matrix4D::rotationX(pickup.wheelSpinFront) *
        wheelFScale;

    // Hinterachse links
    pickup.modelWheelRL =
        world *
        Matrix4D::translation({rearWheelX, rearR, -wheelTrack / 2.0f}) *
        Matrix4D::rotationZ(to_radians(90.0f)) *
        Matrix4D::rotationX(pickup.wheelSpinRear) *
        wheelRScale;

    // Hinterachse rechts
    pickup.modelWheelRR =
        world *
        Matrix4D::translation({rearWheelX, rearR, wheelTrack / 2.0f}) *
        Matrix4D::rotationZ(to_radians(90.0f)) *
        Matrix4D::rotationX(pickup.wheelSpinRear) *
        wheelRScale;

        // Ersatzrad bleibt statisch am Heck
        float spareX = -pickup.baseLength - thickness;
        float spareY = pickup.baseY + pickup.baseHeight * 0.6f;
        Matrix4D spareRot = Matrix4D::rotationX(to_radians(90.0f));

    pickup.modelSpare =
        world *
        Matrix4D::translation({spareX, spareY, 0.0f}) *
        spareRot *
        wheelFScale;
}

Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels) {
    Pickup pickup;

    // Basis-Maße
    pickup.baseLength = 4.0f;
    pickup.baseHeight = 1.0f;
    pickup.baseWidth  = 1.5f;
    pickup.baseY      = 2.0f;

    // Rad-Parameter
    pickup.wheelBaseHalf     = pickup.baseLength / 2.0f;
    pickup.wheelTrack        = pickup.baseWidth + 1.9f;
    pickup.frontWheelRadius  = 0.7f;
    pickup.rearWheelRadius   = 1.0f;
    pickup.wheelThickness    = 0.2f;

    pickup.wheelBase = 2.0f * pickup.wheelBaseHalf;
    pickup.width     = pickup.wheelTrack;

    // Dynamischer Zustand initial
    pickup.position        = {0.0f, 0.0f, 0.0f};
    pickup.rotationY       = 0.0f;
    pickup.steeringAngle   = 0.0f;
    pickup.wheelSpinFront  = 0.0f;
    pickup.wheelSpinRear   = 0.0f;

    // Meshes
    pickup.base     = meshCreate(cube::vertexPos, cube::indices, colorBase, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.cockpit  = meshCreate(cube::vertexPos, cube::indices, colorCockpit, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFL  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFR  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRL  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRR  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.spare    = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);

    // Initiale Modelmatrizen basierend auf obigen Werten
    pickupBuildModelMatrices(pickup);

    return pickup;
}

void pickupDraw(const Pickup &pickup, ShaderProgram &shader) {
    // Base
    shaderUniform(shader, "uModel", pickup.modelBase);
    glBindVertexArray(pickup.base.vao);
    glDrawElements(GL_TRIANGLES, pickup.base.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Cockpit
    shaderUniform(shader, "uModel", pickup.modelCockpit);
    glBindVertexArray(pickup.cockpit.vao);
    glDrawElements(GL_TRIANGLES, pickup.cockpit.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Räder
    shaderUniform(shader, "uModel", pickup.modelWheelFL);
    glBindVertexArray(pickup.wheelFL.vao);
    glDrawElements(GL_TRIANGLES, pickup.wheelFL.size_ibo, GL_UNSIGNED_INT, nullptr);

    shaderUniform(shader, "uModel", pickup.modelWheelFR);
    glBindVertexArray(pickup.wheelFR.vao);
    glDrawElements(GL_TRIANGLES, pickup.wheelFR.size_ibo, GL_UNSIGNED_INT, nullptr);

    shaderUniform(shader, "uModel", pickup.modelWheelRL);
    glBindVertexArray(pickup.wheelRL.vao);
    glDrawElements(GL_TRIANGLES, pickup.wheelRL.size_ibo, GL_UNSIGNED_INT, nullptr);

    shaderUniform(shader, "uModel", pickup.modelWheelRR);
    glBindVertexArray(pickup.wheelRR.vao);
    glDrawElements(GL_TRIANGLES, pickup.wheelRR.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Ersatzrad
    shaderUniform(shader, "uModel", pickup.modelSpare);
    glBindVertexArray(pickup.spare.vao);
    glDrawElements(GL_TRIANGLES, pickup.spare.size_ibo, GL_UNSIGNED_INT, nullptr);
}

void pickupDelete(Pickup &pickup) {
    meshDelete(pickup.base);
    meshDelete(pickup.cockpit);
    meshDelete(pickup.wheelFL);
    meshDelete(pickup.wheelFR);
    meshDelete(pickup.wheelRL);
    meshDelete(pickup.wheelRR);
    meshDelete(pickup.spare);
}

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
    // 1) Fahr-Richtung (vor/zurück)
    float dir = 0.0f;
    if (moveForward)  dir += 1.0f;
    if (moveBackward) dir -= 1.0f;

    float distance = dir * moveSpeed * dt; // zurückgelegte Strecke dieses Frames

    // 2) Lenkrichtung
    int steeringSign = 0;
    if (turnLeft)  steeringSign += 1;
    if (turnRight) steeringSign -= 1;

    // Front-Räder sofort auf ±maxSteeringAngle stellen (oder 0)
    if (steeringSign != 0) {
        pickup.steeringAngle = steeringSign * maxSteeringAngleRad;
    } else {
        pickup.steeringAngle = 0.0f;
    }

    // 3) Fahrzeug dreht sich, wenn gelenkt wird UND wir uns bewegen
    if (steeringSign != 0 && std::fabs(distance) > 1e-6f) {
        // turningAnglePerMeterDeg: [deg pro Meter]
        float headingChangeDeg = steeringSign * turningAnglePerMeterDeg * distance;
        pickup.rotationY += to_radians(headingChangeDeg);
    }

    // 4) Vorwärts-Vektor (Achtung: Pickup zeigt in +X-Richtung!)
    // yaw = 0 => Vorwärts = (1, 0, 0)
    Vector3D forward = {
        std::cos(pickup.rotationY),
        0.0f,
        std::sin(pickup.rotationY)
    };

    // 5) Position aktualisieren
    pickup.position += forward * distance;

    // 6) Radrotation (Drehung um Umfang)
    // α = Δx / r
    if (std::fabs(distance) > 1e-6f) {
        float spinDeltaFront = distance / pickup.frontWheelRadius;
        float spinDeltaRear  = distance / pickup.rearWheelRadius;

        pickup.wheelSpinFront += spinDeltaFront;
        pickup.wheelSpinRear  += spinDeltaRear;
    }

    // 7) Alle Modelmatrizen wieder neu aus Zustand bauen
    pickupBuildModelMatrices(pickup);
}
