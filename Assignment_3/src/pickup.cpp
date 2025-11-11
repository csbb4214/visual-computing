#include <cstdlib>
#include <iostream>

#include "mygl/mesh.h"
#include "mygl/shader.h"

#include "pickup.h"

Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels) {
    Pickup pickup;

    // Store dimensions for reference
    pickup.baseLength = 4.0f;
    pickup.baseHeight = 1.0f;
    pickup.baseWidth = 1.5f;
    pickup.baseY = 2.0f;

    // Create meshes for all parts
    pickup.base = meshCreate(cube::vertexPos, cube::indices, colorBase, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.cockpit = meshCreate(cube::vertexPos, cube::indices, colorCockpit, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFL = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFR = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRL = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRR = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.spare = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels, GL_STATIC_DRAW, GL_STATIC_DRAW);

    // Setup transformation matrices based on base dimensions
    pickup.modelBase = Matrix4D::translation({0, pickup.baseY, 0}) * 
                       Matrix4D::scale(pickup.baseLength, pickup.baseHeight, pickup.baseWidth);

    // Cockpit (front part, stacked on top of base)
    float cockpitX = pickup.baseLength / 4.0f;
    float cockpitY = pickup.baseY + pickup.baseHeight + 0.5f;
    pickup.modelCockpit = Matrix4D::translation({cockpitX, cockpitY, 0}) * 
                          Matrix4D::scale(1, 1, pickup.baseWidth);

    // Calculate wheel positions
    float wheelBaseHalf = pickup.baseLength / 2.0f;
    float wheelTrack = pickup.baseWidth + 0.4f;

    float frontWheelX = wheelBaseHalf * 0.625f;
    float rearWheelX = -wheelBaseHalf * 0.5f;

    float frontWheelRadius = 0.7f;
    float rearWheelRadius = 1.0f;
    float wheelThickness = 0.2f;

    // Wheels
    Matrix4D wheelRot = Matrix4D::rotationY(to_radians(90.0f));
    Matrix4D wheelFScale = Matrix4D::scale(wheelThickness, frontWheelRadius, frontWheelRadius);
    Matrix4D wheelRScale = Matrix4D::scale(wheelThickness, rearWheelRadius, rearWheelRadius);

    pickup.modelWheelFL = Matrix4D::translation({frontWheelX, frontWheelRadius, -wheelTrack / 2.0f}) * 
                          wheelRot * wheelFScale;
    pickup.modelWheelFR = Matrix4D::translation({frontWheelX, frontWheelRadius, wheelTrack / 2.0f}) * 
                          wheelRot * wheelFScale;
    pickup.modelWheelRL = Matrix4D::translation({rearWheelX, rearWheelRadius, -wheelTrack / 2.0f}) * 
                          wheelRot * wheelRScale;
    pickup.modelWheelRR = Matrix4D::translation({rearWheelX, rearWheelRadius, wheelTrack / 2.0f}) * 
                          wheelRot * wheelRScale;
    
    // Spare wheel
    float spareX = -wheelBaseHalf - 0.2f;
    float spareY = pickup.baseY + pickup.baseHeight * 0.6f;
    Matrix4D spareRot = Matrix4D::rotationX(to_radians(90.0f));
    pickup.modelSpare = Matrix4D::translation({spareX, spareY, 0.0f}) * 
                        spareRot * wheelFScale;

    return pickup;
}

void pickupDraw(const Pickup &pickup, ShaderProgram &shader) {
    // Draw Base
    shaderUniform(shader, "uModel", pickup.modelBase);
    glBindVertexArray(pickup.base.vao);
    glDrawElements(GL_TRIANGLES, pickup.base.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Draw Cockpit
    shaderUniform(shader, "uModel", pickup.modelCockpit);
    glBindVertexArray(pickup.cockpit.vao);
    glDrawElements(GL_TRIANGLES, pickup.cockpit.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Draw Wheels
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

    // Draw Spare
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