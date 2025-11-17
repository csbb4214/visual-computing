#include <cstdlib>
#include <iostream>
#include <cmath>

#include "mygl/mesh.h"
#include "mygl/shader.h"
#include "ground.h"
#include "pickup.h"

Pickup pickupCreate(const Vector4D &colorBase, const Vector4D &colorCockpit, const Vector4D &colorWheels) {
    Pickup pickup;

    // Meshes
    pickup.base     = meshCreate(cube::vertexPos,     cube::indices,     colorBase,    GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.cockpit  = meshCreate(cube::vertexPos,     cube::indices,     colorCockpit, GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFL  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelFR  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRL  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.wheelRR  = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);
    pickup.spare    = meshCreate(cylinder::vertexPos, cylinder::indices, colorWheels,  GL_STATIC_DRAW, GL_STATIC_DRAW);

    // Base dimensions
    pickup.baseLength = 4.0f;
    pickup.baseHeight = 1.0f;
    pickup.baseWidth  = 1.5f;
    pickup.baseY      = 2.0f;

    // Wheel dimensions
    pickup.wheelBaseHalf     = pickup.baseLength / 2.0f;
    pickup.wheelTrack        = pickup.baseWidth + 2.0f;   // wheels sit outside the chassis
    pickup.frontWheelRadius  = 0.7f;
    pickup.rearWheelRadius   = 1.0f;
    pickup.wheelThickness    = 0.2f;

    pickup.wheelBase = 2.0f * pickup.wheelBaseHalf;
    pickup.width     = pickup.wheelTrack;

    // Initial world transform (identity at origin)
    pickup.vehicleTransform = Matrix4D::identity();

    // Initial wheel animation states
    pickup.wheelRotationAngle = 0.0f;
    pickup.wheelSteeringAngle = 0.0f;

    // -------------------------------
    // Local model transforms
    // -------------------------------

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

    // Wheel positions, rotations and scales
    float frontWheelX = pickup.wheelBaseHalf * 1.3f;
    float rearWheelX  = -pickup.wheelBaseHalf * 0.8f;
    Matrix4D wheelRot = Matrix4D::rotationY(to_radians(90.0f));
    Matrix4D wheelFScale = Matrix4D::scale(pickup.wheelThickness, pickup.frontWheelRadius, pickup.frontWheelRadius);
    Matrix4D wheelRScale = Matrix4D::scale(pickup.wheelThickness, pickup.rearWheelRadius, pickup.rearWheelRadius);

    // Front wheels
    pickup.modelWheelFLBase =
        Matrix4D::translation({frontWheelX, pickup.frontWheelRadius, -pickup.wheelTrack / 2.0f}) *
        wheelRot * wheelFScale;

    pickup.modelWheelFRBase =
        Matrix4D::translation({frontWheelX, pickup.frontWheelRadius, pickup.wheelTrack / 2.0f}) *
        wheelRot * wheelFScale;

    // Rear wheels
    pickup.modelWheelRLBase =
        Matrix4D::translation({rearWheelX, pickup.frontWheelRadius, -pickup.wheelTrack / 2.0f}) *
        wheelRot * wheelRScale;

    pickup.modelWheelRRBase =
        Matrix4D::translation({rearWheelX, pickup.frontWheelRadius, pickup.wheelTrack / 2.0f}) *
        wheelRot * wheelRScale;

    // Spare wheel — placed horizontally on the truck bed
    float spareX = -pickup.baseLength - pickup.wheelThickness;
    float spareY = pickup.frontWheelRadius + 2.0f;
    float spareZ = 0.0f;
    Matrix4D spareRot = Matrix4D::rotationX(to_radians(90.0f));

    pickup.modelSpareLocal =
        Matrix4D::translation({spareX, spareY, spareZ}) *
        spareRot *
        Matrix4D::scale(pickup.wheelThickness, pickup.frontWheelRadius, pickup.frontWheelRadius);

    return pickup;
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

void pickupDraw(const Pickup &pickup, ShaderProgram &shader) {
    // draw pickup parts with respect to global vehicle transform
    const Matrix4D &W = pickup.vehicleTransform;

    // Base
    shaderUniform(shader, "uModel", W * pickup.modelBaseLocal);
    glBindVertexArray(pickup.base.vao);
    glDrawElements(GL_TRIANGLES, pickup.base.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Cockpit
    shaderUniform(shader, "uModel", W * pickup.modelCockpitLocal);
    glBindVertexArray(pickup.cockpit.vao);
    glDrawElements(GL_TRIANGLES, pickup.cockpit.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Wheel animation transforms
    Matrix4D roll = Matrix4D::rotationX(pickup.wheelRotationAngle);
    Matrix4D steering = Matrix4D::rotationY(pickup.wheelSteeringAngle);
    Matrix4D wheelTilt = Matrix4D::rotationY(to_radians(90.0f));

    /* Front wheels */
    float frontWheelX = pickup.wheelBaseHalf * 1.3f;
    Matrix4D scaleF = Matrix4D::scale(pickup.wheelThickness, pickup.frontWheelRadius, pickup.frontWheelRadius);

    // Front-left wheel
    {
        Matrix4D local =
            Matrix4D::translation({frontWheelX, pickup.frontWheelRadius, -pickup.wheelTrack / 2.0f}) *
            steering * wheelTilt * roll * scaleF;

        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelFL.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelFL.size_ibo, GL_UNSIGNED_INT, nullptr);
    }
    // Front-right wheel
    {
        Matrix4D local =
            Matrix4D::translation({frontWheelX, pickup.frontWheelRadius, pickup.wheelTrack / 2.0f}) *
            steering * wheelTilt * roll * scaleF;

        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelFR.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelFR.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    /* Rear wheels */
    float rearWheelX = -pickup.wheelBaseHalf * 0.8f;
    float rearR = pickup.rearWheelRadius;
    Matrix4D scaleR = Matrix4D::scale(pickup.wheelThickness, rearR, rearR);

    // Rear-left wheel
    {
        Matrix4D local =
            Matrix4D::translation({rearWheelX, rearR, -pickup.wheelTrack / 2.0f}) *
            wheelTilt * roll * scaleR;

        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelRL.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelRL.size_ibo, GL_UNSIGNED_INT, nullptr);
    }
    // Rear-right wheel
    {
        Matrix4D local =
            Matrix4D::translation({rearWheelX, rearR, pickup.wheelTrack / 2.0f}) *
            wheelTilt * roll * scaleR;

        shaderUniform(shader, "uModel", W * local);
        glBindVertexArray(pickup.wheelRR.vao);
        glDrawElements(GL_TRIANGLES, pickup.wheelRR.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    /* Spare wheel */
    shaderUniform(shader, "uModel", W * pickup.modelSpareLocal);
    glBindVertexArray(pickup.spare.vao);
    glDrawElements(GL_TRIANGLES, pickup.spare.size_ibo, GL_UNSIGNED_INT, nullptr);
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
    float distance = (moveForward - moveBackward) * moveSpeed * dt; // distance traveled this frame
    int turnSign = turnLeft - turnRight; // -1 = left, +1 = right, 0 = no turn

    // Wheel rolling animation (based on distance traveled so would work for different speeds as well)
    if (std::fabs(distance) > 1e-6f) {
        float circumferenceFront = 2.0f * M_PI * pickup.frontWheelRadius;
        float rotationAngle = (distance / circumferenceFront) * 2.0f * M_PI;

        pickup.wheelRotationAngle += rotationAngle;
    }

    // Steering angle update
    if (turnSign != 0) {
        float steeringChange = turnSign * maxSteeringAngleRad * 4.0f * dt;
        pickup.wheelSteeringAngle += steeringChange;

        // Clamp steering angle
        if (pickup.wheelSteeringAngle > maxSteeringAngleRad) {
            pickup.wheelSteeringAngle = maxSteeringAngleRad;
        }
        else if (pickup.wheelSteeringAngle < -maxSteeringAngleRad) {
            pickup.wheelSteeringAngle = -maxSteeringAngleRad;
        }
    }
    else {
        // Auto-centering steering
        if (std::fabs(pickup.wheelSteeringAngle) > 0.01f) {
            pickup.wheelSteeringAngle *= 0.8f;
        } else {
            pickup.wheelSteeringAngle = 0.0f;  
        }
    }

    // Move forward in local +X direction
    if (std::fabs(distance) > 1e-6f) {
        Matrix4D T = Matrix4D::translation({distance, 0.0f, 0.0f});
        pickup.vehicleTransform = pickup.vehicleTransform * T;
    }

    // Apply heading rotation based on traveled distance
    if (turnSign != 0 && std::fabs(distance) > 1e-6f) {
        float headingChangeDeg = turningAnglePerMeterDeg * distance * float(turnSign);
        float headingChangeRad = to_radians(headingChangeDeg);

        pickup.vehicleTransform = pickup.vehicleTransform * Matrix4D::rotationY(headingChangeRad);
    }
}

void pickupAdjustToTerrain(Pickup &pickup, const Ground &ground) {
    // helper lambda to normalize a Vector3D
    auto normalize = [](Vector3D v) {
        float len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        return len > 0.0f ? v / len : v;
    };

    // Define wheel positions in local space
    float frontWheelX = pickup.wheelBaseHalf * 1.3f;
    float rearWheelX  = -pickup.wheelBaseHalf * 0.8f;
    float wheelY = pickup.frontWheelRadius;
    float halfTrack = pickup.wheelTrack * 0.5f;

    Vector3D localWheels[4] = {
        {frontWheelX, wheelY, -halfTrack},  // FL
        {frontWheelX, wheelY,  halfTrack},  // FR
        {rearWheelX,  wheelY, -halfTrack},  // RL
        {rearWheelX,  wheelY,  halfTrack}   // RR
    };

    Matrix4D &M = pickup.vehicleTransform;

    // Transform wheels to world space and get terrain heights
    Vector3D worldWheels[4];
    float heights[4];
    float avgHeight = 0.0f;

    for (int i = 0; i < 4; ++i) {
        Vector4D p4(localWheels[i].x, localWheels[i].y, localWheels[i].z, 1.0f);
        Vector4D r = M * p4;
        worldWheels[i] = {r.x, r.y, r.z};

        heights[i] = 0.0f;
        for (const auto &w : ground.waveParamsVec)
            heights[i] += w.amplitude * sinf(w.omega * dot({worldWheels[i].x, worldWheels[i].z}, w.direction));
        
        avgHeight += heights[i];
    }
    avgHeight *= 0.25f;

    // Build orientation from wheel positions
    Vector3D frontMid = {
        (worldWheels[0].x + worldWheels[1].x) * 0.5f,
        (heights[0] + heights[1]) * 0.5f,
        (worldWheels[0].z + worldWheels[1].z) * 0.5f
    };

    Vector3D rearMid = {
        (worldWheels[2].x + worldWheels[3].x) * 0.5f,
        (heights[2] + heights[3]) * 0.5f,
        (worldWheels[2].z + worldWheels[3].z) * 0.5f
    };

    Vector3D rearLeft = {worldWheels[2].x, heights[2], worldWheels[2].z};
    Vector3D rearRight = {worldWheels[3].x, heights[3], worldWheels[3].z};

    // Compute orthonormal basis
    Vector3D normal = normalize(cross(frontMid - rearLeft, rearRight - rearLeft));
    if (normal.y < 0) normal = -normal;

    Vector3D forward = normalize(frontMid - rearMid);
    Vector3D right = normalize(cross(normal, forward));
    forward = normalize(cross(right, normal));

    // Build transform
    Matrix4D rotation = Matrix4D::identity();
    rotation(0,0) = forward.x; rotation(0,1) = normal.x; rotation(0,2) = right.x;
    rotation(1,0) = forward.y; rotation(1,1) = normal.y; rotation(1,2) = right.y;
    rotation(2,0) = forward.z; rotation(2,1) = normal.z; rotation(2,2) = right.z;

    Vector3D pos(M(0,3), avgHeight, M(2,3));
    pickup.vehicleTransform = Matrix4D::translation(pos) * rotation;
}


Vector3D pickupGetWorldPosition(const Pickup &pickup) {
    const Matrix4D &M = pickup.vehicleTransform;

    Vector4D origin(0.0f, 0.0f, 0.0f, 1.0f);
    Vector4D world = M * origin;

    return Vector3D(world.x, world.y, world.z);
}
