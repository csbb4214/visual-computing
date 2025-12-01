#pragma once

#include "mygl/base.h"
#include "mygl/model.h"

#include "ground.h"

#include <map>
#include <vector>

struct SquashHeights {
    float front;
    float back;
};

struct Pickup {
    enum eControl { LEFT, RIGHT, FASTER, SLOWER, CONTROL_COUNT };

    enum ePart { BODY = 0, WHEEL_BR, WHEEL_BL, WHEEL_FR, WHEEL_FL, LIGHT_BR, LIGHT_BL, LIGHT_FR, LIGHT_FL, PART_COUNT };

    ePart ePartWheels[4] = {WHEEL_BR, WHEEL_BL, WHEEL_FR, WHEEL_FL};

    /* predefined positions of wheels and lights in model space */
    Vector3D wheelBRPos = Vector3D{-0.90995f, 0.54366f, -1.186f};
    Vector3D wheelBLPos = Vector3D{0.90995f, 0.54366f, -1.186f};
    Vector3D wheelFRPos = Vector3D{-0.77453f, 0.34805f, 1.4826f};
    Vector3D wheelFLPos = Vector3D{0.77453f, 0.34805f, 1.4826f};

    Vector3D lightBRPos = Vector3D{-0.78254f, 0.62657f, -2.0363f};
    Vector3D lightBLPos = Vector3D{0.78254f, 0.62657f, -2.0363f};
    Vector3D lightFRPos = Vector3D{-0.45246f, 0.55644f, 1.9088f};
    Vector3D lightFLPos = Vector3D{0.45246f, 0.55644f, 1.9088f};

    /* Model definitions */
    std::vector<Matrix4D> partTransformations;
    std::vector<Vector3D> wheelTranslations;
    std::vector<Model> partModel;
    std::map<int, Vector3D> emissionColors;

    /* current state */
    Matrix4D transformation = Matrix4D::identity();
    Matrix4D rotation = Matrix4D::identity();
    Matrix4D frontWheelRotation = Matrix4D::identity();
    Matrix4D backWheelRotation = Matrix4D::identity();
    Matrix4D lastSteeringWheelRotation = Matrix4D::identity();

    Vector3D position = {0.0, 0.0, 0.0};

    float maxSpeed = 4.0f;
    float turningAngle = static_cast<float>(to_radians(20.0f));
    float turningDegPerMeter = 0.0f;

    /* squashing parameters */
    float frontSquashFactor = 0.0f;             /* 0.0 = no squash, 1.0 = full squash */
    float backSquashFactor = 0.0f;              /* 0.0 = no squash, 1.0 = full squash */
    float squashRatio = 1 / 3.0f;               /* how much in percen of the wheel are squashed at full squash factor */
    float squashSpeed = 0.02f;                  /* how fast the squash height changes */
    float squashReleaseSpeed = 3.0f;            /* controls how fast the squash factor is released */
    SquashHeights squashHeights = {0.0f, 0.0f}; /* current squash heights for front and back wheels */

    bool lightsOn = true;
};

/**
 * @brief Initializes the pickup object with all its meshes.
 *
 * @return Initialized pickup.
 */
Pickup pickupLoad(const std::string &filePath);

/**
 * @brief Deletes the given pickup object.
 */
void pickupDelete(Pickup &pickup);

/**
 * @brief Animates the given pickup according to the given input. Also animates the tires.
 */
void pickupMove(Pickup &pickup, const Ground &ground, bool control[], float dt);

/**
 * @brief Calculates how much the pickup approximately turns per meter travelled for a given turning angle.
 */
float calculateTurningAnglePerMeter(float wheelBase, float turningAngle, float width);


/**
 * @brief Sets the emission of the lights of the given pickup model to be on or off.
 */
void setEmission(Pickup &pickup, bool emission);
