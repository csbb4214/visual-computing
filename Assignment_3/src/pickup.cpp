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

    float frontWheelX = pickup.wheelBaseHalf * 1.3f;
    float rearWheelX  = -pickup.wheelBaseHalf * 0.8f;
    float wheelY = pickup.frontWheelRadius;

    float wheelTrack = pickup.wheelTrack;

    // Local wheel contact points
    Vector3D localWheelFL(frontWheelX, wheelY, -wheelTrack/2.0f);
    Vector3D localWheelFR(frontWheelX, wheelY, wheelTrack/2.0f);
    Vector3D localWheelRL(rearWheelX,  wheelY, -wheelTrack/2.0f);
    Vector3D localWheelRR(rearWheelX,  wheelY, wheelTrack/2.0f);

    Matrix4D &M = pickup.vehicleTransform;

    auto transformPoint = [&M](const Vector3D &p) {
        Vector4D p4(p.x, p.y, p.z, 1.0f);
        Vector4D r = M * p4;
        return Vector3D(r.x, r.y, r.z);
    };

    Vector3D worldWheelFL = transformPoint(localWheelFL);
    Vector3D worldWheelFR = transformPoint(localWheelFR);
    Vector3D worldWheelRL = transformPoint(localWheelRL);
    Vector3D worldWheelRR = transformPoint(localWheelRR);

    // Terrain height evaluation
    auto computeHeight = [&ground](const Vector2D &p) {
        float height = 0.0f;
        for (const auto &w : ground.waveParamsVec)
            height += w.amplitude * sinf(w.omega * dot(p, w.direction));
        return height;
    };

    float hFL = computeHeight({worldWheelFL.x, worldWheelFL.z});
    float hFR = computeHeight({worldWheelFR.x, worldWheelFR.z});
    float hRL = computeHeight({worldWheelRL.x, worldWheelRL.z});
    float hRR = computeHeight({worldWheelRR.x, worldWheelRR.z});

    float avgHeight = (hFL + hFR + hRL + hRR) / 4.0f;

    // Build a triangle plane from rear-left, rear-right, and midpoint front
    Vector3D frontMid(
        (worldWheelFL.x + worldWheelFR.x) * 0.5f,
        (hFL + hFR) * 0.5f,
        (worldWheelFL.z + worldWheelFR.z) * 0.5f
    );

    Vector3D rearLeft(worldWheelRL.x, hRL, worldWheelRL.z);
    Vector3D rearRight(worldWheelRR.x, hRR, worldWheelRR.z);

    Vector3D AB = rearRight - rearLeft;
    Vector3D AC = frontMid - rearLeft;

    Vector3D normal = cross(AC, AB);

    float len = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    if (len > 0.0f) normal /= len;

    if (normal.y < 0) normal = -normal;

    // Forward direction (rear midpoint → front midpoint)
    Vector3D rearMid(
        (worldWheelRL.x + worldWheelRR.x) * 0.5f,
        (hRL + hRR) * 0.5f,
        (worldWheelRL.z + worldWheelRR.z) * 0.5f
    );

    Vector3D forward = frontMid - rearMid;
    len = sqrt(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
    if (len > 0.0f) forward /= len;

    // Right = normal × forward
    Vector3D right = cross(normal, forward);
    len = sqrt(right.x*right.x + right.y*right.y + right.z*right.z);
    if (len > 0.0f) right /= len;

    // Recompute forward for orthogonality
    forward = cross(right, normal);
    len = sqrt(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
    if (len > 0.0f) forward /= len;

    // Build rotation matrix from the three orthonormal vectors
    Matrix4D rotation = Matrix4D::identity();
    rotation(0,0) = forward.x; rotation(0,1) = normal.x; rotation(0,2) = right.x;
    rotation(1,0) = forward.y; rotation(1,1) = normal.y; rotation(1,2) = right.y;
    rotation(2,0) = forward.z; rotation(2,1) = normal.z; rotation(2,2) = right.z;

    // Pickup world position (reuse X,Z, update Y to avg height)
    Vector3D pos(M(0,3), M(1,3), M(2,3));
    pos.y = avgHeight;

    pickup.vehicleTransform = Matrix4D::translation(pos) * rotation;
}


Vector3D pickupGetWorldPosition(const Pickup &pickup) {
    const Matrix4D &M = pickup.vehicleTransform;

    Vector4D origin(0.0f, 0.0f, 0.0f, 1.0f);
    Vector4D world = M * origin;

    return Vector3D(world.x, world.y, world.z);
}
