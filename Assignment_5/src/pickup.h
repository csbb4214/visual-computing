#pragma once

#include "mygl/base.h"
#include "mygl/model.h"

#include "ground.h"

#include <vector>
#include <map>

struct SquashHeights {
    float front;
    float back;
};

struct Pickup {
    enum eControl { 
        LEFT, 
        RIGHT, 
        FASTER, 
        SLOWER, 
        CONTROL_COUNT 
    };

    enum ePart { 
        BODY = 0, 
        WHEEL_BR,
        WHEEL_BL,
        WHEEL_FR,
        WHEEL_FL,
        LIGHT_BR,
        LIGHT_BL,
        LIGHT_FR,
        LIGHT_FL,
        PART_COUNT 
    };

    ePart ePartWheels[4] = {WHEEL_BR, WHEEL_BL, WHEEL_FR, WHEEL_FL};

    Vector3D wheelBRPos = Vector3D{-0.66745f, 0.45448f, -1.1886f};
    Vector3D wheelBLPos = Vector3D{0.66745f, 0.45448f, -1.1886f};
    Vector3D wheelFRPos = Vector3D{-0.65191f, 0.3454f, 1.4846f};
    Vector3D wheelFLPos = Vector3D{0.65191f, 0.3454f, 1.4846f};
    Vector3D lightBRPos = Vector3D{-0.73327f, 0.49011f, -2.0237f};
    Vector3D lightBLPos = Vector3D{0.73327f, 0.49011f, -2.0237f};
    Vector3D lightFRPos = Vector3D{-0.45591f, 0.55543f, 1.9673f};
    Vector3D lightFLPos = Vector3D{0.45591f, 0.55543f, 1.9673f};

    std::vector<Matrix4D> partTransformations;
    std::vector<Vector3D> wheelTranslations;
    std::vector<Model> partModel;
    std::map<int, Vector3D> emissionColors;
    std::map<int, Texture> emissionTextures;
    Texture noEmissionTexture;

    Matrix4D transformation = Matrix4D::identity();
    Matrix4D rotation = Matrix4D::identity();
    Matrix4D lastSteeringWheelRotation = Matrix4D::identity();

    Vector3D position = {0.0, 0.0, 0.0};

    Matrix4D frontWheelRotation = Matrix4D::identity();
    Matrix4D backWheelRotation = Matrix4D::identity();

    float turningAngle = static_cast<float>(to_radians(20.0f));
    float wheelBase = 0.0f;
    float width = 0.0f;
    float turningDegPerMeter = 0.0f;

    float frontSquashFactor = 0.0f;
    float backSquashFactor = 0.0f;
    float squashRatio = 1 / 3.0f;
    float squashSpeed = 0.02f;
    float squashReleaseSpeed = 3.0f;
    SquashHeights squashHeights = {0.0f, 0.0f};

    float maxSpeed = 4.0f;
};

/**
 * @brief Initializes the pickup object with all its meshes.
 *
 * @return Initialized pickup.
 */
Pickup pickupLoad(const std::string& filePath);

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
