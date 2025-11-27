#include "pickup.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

Pickup pickupLoad(const std::string& filePath) {
    std::vector<Model> models = modelLoad(filePath);

    if(models.size() != Pickup::ePart::PART_COUNT) {
        throw std::runtime_error("[Pickup] number of parts do not match!" + std::to_string(models.size()));
    }

    Pickup pickup;
    pickup.partModel.resize(models.size());
    pickup.partTransformations.resize(Pickup::ePart::PART_COUNT, Matrix4D::identity());
    pickup.wheelTranslations.resize(Pickup::ePart::PART_COUNT, {0.0f, 0.0f, 0.0f});

    pickup.wheelTranslations[Pickup::WHEEL_BR] = pickup.wheelBRPos;
    pickup.wheelTranslations[Pickup::WHEEL_BL] = pickup.wheelBLPos;
    pickup.wheelTranslations[Pickup::WHEEL_FR] = pickup.wheelFRPos;
    pickup.wheelTranslations[Pickup::WHEEL_FL] = pickup.wheelFLPos;

    for(const auto& obj : models) {
        if(obj.name == "Body") pickup.partModel[Pickup::BODY] = obj;
        else if(obj.name == "WheelBR") pickup.partModel[Pickup::WHEEL_BR] = obj;
        else if(obj.name == "WheelBL") pickup.partModel[Pickup::WHEEL_BL] = obj;
        else if(obj.name == "WheelFR") pickup.partModel[Pickup::WHEEL_FR] = obj;
        else if(obj.name == "WheelFL") pickup.partModel[Pickup::WHEEL_FL] = obj;
        else if(obj.name == "LightBL") {
            pickup.partModel[Pickup::LIGHT_BL] = obj;
            pickup.emissionColors[Pickup::LIGHT_BL] = obj.material[0].emission;
        } else if(obj.name == "LightBR") {
            pickup.partModel[Pickup::LIGHT_BR] = obj;
            pickup.emissionColors[Pickup::LIGHT_BR] = obj.material[0].emission;
        } else if(obj.name == "LightFL") {
            pickup.partModel[Pickup::LIGHT_FL] = obj;
            pickup.emissionColors[Pickup::LIGHT_FL] = obj.material[0].emission;
        } else if(obj.name == "LightFR") {
            pickup.partModel[Pickup::LIGHT_FR] = obj;
            pickup.emissionColors[Pickup::LIGHT_FR] = obj.material[0].emission;
        } else throw std::runtime_error("[Pickup] unkown part name: " + obj.name);
    }

    float wheelBase = abs(pickup.wheelFRPos.z - pickup.wheelBRPos.z);
    float width = abs(pickup.wheelFRPos.x - pickup.wheelFLPos.x);
    pickup.turningDegPerMeter = calculateTurningAnglePerMeter(wheelBase, pickup.turningAngle, width);

    return pickup;
}

void pickupDelete(Pickup &pickup) {
    for (auto &model : pickup.partModel) {
        modelDelete(model);
    }

    pickup.partModel.clear();
    pickup.partTransformations.clear();
    pickup.wheelTranslations.clear();
}

SquashHeights calculateFrontBackSquashHeights(const Pickup &pickup) {
    float frontWheelDiameter = 0.35f;
    float backWheelDiameter = 0.5f;
    float frontSquashHeight = frontWheelDiameter * pickup.squashRatio * pickup.frontSquashFactor;
    float backSquashHeight = backWheelDiameter * pickup.squashRatio * pickup.backSquashFactor;
    return {frontSquashHeight, backSquashHeight};
}

void updateSquashHeightsBySpeed(Pickup &pickup, float deltaMove, float dt) {
    /* update squash factors based on movement, forward movement squashes back wheel and vice versa */
    if (deltaMove > 0.0f) {
        pickup.frontSquashFactor -= pickup.squashSpeed;
        pickup.backSquashFactor += pickup.squashSpeed;
    } else if (deltaMove < 0.0f) {
        pickup.frontSquashFactor += pickup.squashSpeed;
        pickup.backSquashFactor -= pickup.squashSpeed;
    } else { /* no movement, release squash factors */
        pickup.frontSquashFactor += (0.0f - pickup.frontSquashFactor) * pickup.squashReleaseSpeed * dt;
        pickup.backSquashFactor += (0.0f - pickup.backSquashFactor) * pickup.squashReleaseSpeed * dt;
    }

    /* clamp squash factors */
    pickup.frontSquashFactor = std::clamp(pickup.frontSquashFactor, 0.0f, 1.0f);
    pickup.backSquashFactor = std::clamp(pickup.backSquashFactor, 0.0f, 1.0f);

    pickup.squashHeights = calculateFrontBackSquashHeights(pickup);
}

void pickupUpdateRotation(Pickup &pickup, const Ground &ground) {
    /* get local contact points of the tires */
    Vector3D localTireBR = Vector3D{-0.85f, 0.0f, -1.1f};
    Vector3D localTireBL = Vector3D{0.85f, 0.0f, -1.1f};
    Vector3D localTireFR = Vector3D{-0.85f, 0.0f, 1.3f};
    Vector3D localTireFL = Vector3D{0.85f, 0.0f, 1.3f};

    /* transform contact points into world space */
    Vector3D worldTireBR = pickup.position + (Matrix3D(pickup.rotation) * localTireBR);
    Vector3D worldTireBL = pickup.position + (Matrix3D(pickup.rotation) * localTireBL);
    Vector3D worldTireFR = pickup.position + (Matrix3D(pickup.rotation) * localTireFR);
    Vector3D worldTireFL = pickup.position + (Matrix3D(pickup.rotation) * localTireFL);

    /* get ground displacement at contact points */
    float dispBR = groundDisplacement(ground.waveParamsVec, {worldTireBR.x, worldTireBR.z}) - pickup.squashHeights.back;
    float dispBL = groundDisplacement(ground.waveParamsVec, {worldTireBL.x, worldTireBL.z}) - pickup.squashHeights.back;
    float dispFR = groundDisplacement(ground.waveParamsVec, {worldTireFR.x, worldTireFR.z}) - pickup.squashHeights.front;
    float dispFL = groundDisplacement(ground.waveParamsVec, {worldTireFL.x, worldTireFL.z}) - pickup.squashHeights.front;

    /* set pickup position and rotation according to tire displacements */
    float centerDisp = (dispBR + dispBL + (dispFR + dispFL) / 2.0f) / 3.0f;
    pickup.position.y = centerDisp;

    /* define triangle for rotation calculation */
    Vector3D v0 = (Vector3D{worldTireFR.x, dispFR, worldTireFR.z} + Vector3D{worldTireFL.x, dispFL, worldTireFL.z}) / 2;
    Vector3D v1 = Vector3D{worldTireBR.x, dispBR, worldTireBR.z};
    Vector3D v2 = Vector3D{worldTireBL.x, dispBL, worldTireBL.z};
    Vector3D n0 = normalize(v1 - v0);
    Vector3D n1 = normalize(v2 - v0);
    Vector3D n2 = normalize(v1 - v2);

    /* calculate up, right, and forward vectors for rotation */
    Vector3D up = normalize(cross(n1, n0));
    Vector3D right = n2;
    Vector3D forward = normalize(cross(right, up));

    /* update pickup rotation */
    pickup.rotation = Matrix4D(Matrix3D(-right, up, forward));
}

void animateWheels(Pickup &pickup, int turningDirection, float deltaMove) {
    Matrix4D steeringWheelRotation = Matrix4D::rotationY(turningDirection * pickup.turningAngle);

    for (auto wheel : pickup.ePartWheels) {
        float wheelRadius = wheel == Pickup::ePart::WHEEL_BR || wheel == Pickup::ePart::WHEEL_BL ? 0.25f : 0.175f;

        /* calculate wheel rotation due to movement */
        Matrix4D wheelTurningRotation = Matrix4D::rotationX(deltaMove / wheelRadius);

        /* apply steering rotation to matrices saving global tire x-rotations */
        if (wheel == Pickup::ePart::WHEEL_FR) {
            pickup.frontWheelRotation = wheelTurningRotation * pickup.frontWheelRotation;
        } else if (wheel == Pickup::ePart::WHEEL_BR) {
            pickup.backWheelRotation = wheelTurningRotation * pickup.backWheelRotation;
        }

        /* apply steering rotation to front wheels */
        if (wheel == Pickup::ePart::WHEEL_FR || wheel == Pickup::ePart::WHEEL_FL) {
            wheelTurningRotation = steeringWheelRotation * wheelTurningRotation * transpose(pickup.lastSteeringWheelRotation);
        }

        /* update wheel transformation */
        Vector3D wheelPos = pickup.wheelTranslations[wheel];
        pickup.partTransformations[wheel] =
            Matrix4D::translation(wheelPos) * wheelTurningRotation * Matrix4D::translation(-wheelPos) * pickup.partTransformations[wheel];
    }
    pickup.lastSteeringWheelRotation = steeringWheelRotation;
}

void pickupMove(Pickup &pickup, const Ground &ground, bool control[], float dt) {
    /* retrieve input for controls */
    int throttle = +control[Pickup::eControl::FASTER] - control[Pickup::eControl::SLOWER];
    int turningDirection = +control[Pickup::eControl::LEFT] - control[Pickup::eControl::RIGHT];

    /* update pickup position and rotation */
    float deltaMove = throttle * pickup.maxSpeed * dt;
    float deltaTurn = throttle * turningDirection * deltaMove * static_cast<float>(to_radians(pickup.turningDegPerMeter));
    pickup.position += pickup.rotation * (deltaMove * Vector4D(0.0, 0.0, 1.0, 0.0));
    pickup.rotation = Matrix4D::rotationY(deltaTurn * throttle) * pickup.rotation;

    /* call additional update and animate functions */
    updateSquashHeightsBySpeed(pickup, deltaMove, dt);
    
    pickupUpdateRotation(pickup, ground);
    animateWheels(pickup, turningDirection, deltaMove);

    /* calculate the pickup's final transformation */
    pickup.transformation = Matrix4D::translation(pickup.position) * pickup.rotation;
}

/* calculate how much the car approximately turns per meter travelled for a given turning angle */
float calculateTurningAnglePerMeter(float wheelBase, float turningAngle, float width) {
    /* according to https://calculator.academy/turning-radius-calculator/ and
     * https://gamedev.stackexchange.com/questions/50022/typical-maximum-steering-angle-of-a-real-car/50029 */
    float turningRadius = wheelBase / tan(turningAngle);
    float innerTurningRadius = turningRadius - width;
    return 360.0f / (2.0f * innerTurningRadius * static_cast<float>(M_PI));
}

void setEmission(Pickup &pickup, bool emission) {
    for (auto const &[part, color] : pickup.emissionColors) {
        /* assumes that each part in emission color has only one material */
        pickup.partModel[part].material[0].emission = emission ? color : Vector3D(0.0f, 0.0f, 0.0f);
    }
}