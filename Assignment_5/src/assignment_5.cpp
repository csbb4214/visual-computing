#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <array>

#include "mygl/camera.h"
#include "mygl/mesh.h"
#include "mygl/shader.h"
#include "mygl/cube_map.h"
#include "mygl/geometry.h"

#include "light.h"
#include "ground.h"
#include "pickup.h"

/* enum for different render modes */
enum eRenderMode
{
    COLOR = 0,    // render diffuse colors
    NORMAL,       // render normals
    BLINN_PHONG,  // render blinn phong
    MODE_COUNT
};

/* directional light definition */
const Light_Directional lightDay = { { 0.3f, -1.0f, 0.0f }, { 0.2f, 0.2f, 0.2f }, { 0.8f, 0.8f, 0.8f } };
const Light_Directional lightNight = { { 0.3f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.1f }, { 0.1f, 0.1f, 0.2f } };

/* struct holding all necessary state variables for scene */
struct {
    /* camera */
    Camera camera;
    bool cameraFollowPickup;
    float zoomSpeedMultiplier;

    /* game objects */
    Ground ground;
    Pickup pickup;

    /* lights */
    bool isDay;
    Light_Directional lightSun;
    Light_Spot lightSpots[4];
    bool lightsOn;

    /* shader */
    eRenderMode renderMode;
    ShaderProgram shaderColor;
    ShaderProgram shaderNormal;
    ShaderProgram shaderBlinnPhong;
    ShaderProgram shaderEnvironmentMap;
} sScene;

/* struct holding all state variables for input */
struct {
    bool mouseLeftButtonPressed = false;
    Vector2D mousePressStart;
    bool buttonPressed[Pickup::eControl::CONTROL_COUNT] = {false, false, false, false};
} sInput;

/* function to set day or night */
void setDay(bool isDay) {
    sScene.lightSun = isDay ? lightDay : lightNight;
    sScene.isDay = isDay;
}

void setPickupLights(bool lightsOn) {
    sScene.lightsOn = lightsOn;
    for (int i = 0; i < 4; i++) {
        sScene.lightSpots[i].enabled = lightsOn;
    }
    setEmission(sScene.pickup, lightsOn);
}

/* GLFW callback function for keyboard events */
void callbackKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* called on keyboard event */

    /* close window on escape */
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        screenshotToPNG("screenshot.png");
    }

    /* input for camera control */
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        sScene.cameraFollowPickup = false;
        sScene.camera.lookAt = {0.0f, 0.0f, 0.0f};
        cameraUpdateOrbit(sScene.camera, {0.0f, 0.0f}, 0.0f);
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        sScene.cameraFollowPickup = true;
    }

    /* input for car control */
    if (key == GLFW_KEY_W) {
        sInput.buttonPressed[Pickup::eControl::FASTER] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_S) {
        sInput.buttonPressed[Pickup::eControl::SLOWER] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if (key == GLFW_KEY_A) {
        sInput.buttonPressed[Pickup::eControl::LEFT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_D) {
        sInput.buttonPressed[Pickup::eControl::RIGHT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    /* toggle render mode */
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        sScene.renderMode = static_cast<eRenderMode>((static_cast<int>(sScene.renderMode) + 1) % eRenderMode::MODE_COUNT);
        switch(sScene.renderMode) {
            case eRenderMode::COLOR:
                std::cout << "Render mode: COLOR" << std::endl;
                break;
            case eRenderMode::NORMAL:
                std::cout << "Render mode: NORMAL" << std::endl;
                break;
            case eRenderMode::BLINN_PHONG:
                std::cout << "Render mode: BLINN_PHONG" << std::endl;
                break;
            default:
                std::cout << "Render mode: UNKNOWN" << std::endl;
        }
    }

    /* night light setting */
    if(key == GLFW_KEY_N && action == GLFW_PRESS) {
        setDay(false);
    }

    /* day light setting */
    if(key == GLFW_KEY_M && action == GLFW_PRESS) {
        setDay(true);
    }

    /* toggle pickup lights */
    if(key == GLFW_KEY_L && action == GLFW_PRESS) {
        setPickupLights(!sScene.lightsOn);
    }
}

/* GLFW callback function for mouse position events */
void callbackMousePos(GLFWwindow *window, double x, double y) {
    /* called on cursor position change */
    if (sInput.mouseLeftButtonPressed) {
        Vector2D diff = sInput.mousePressStart - Vector2D(x, y);
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

/* GLFW callback function for mouse button events */
void callbackMouseButton(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        sInput.mouseLeftButtonPressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

/* GLFW callback function for mouse scroll events */
void callbackMouseScroll(GLFWwindow *window, double xoffset, double yoffset) {
    cameraUpdateOrbit(sScene.camera, {0, 0}, -sScene.zoomSpeedMultiplier * yoffset);
}

/* GLFW callback function for window resize event */
void callbackWindowResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    sScene.camera.width = static_cast<float>(width);
    sScene.camera.height = static_cast<float>(height);
}

/* function to setup and initialize the whole scene */
void sceneInit(float width, float height) {
    sScene.pickup = pickupLoad("assets/pickup/pickup.obj");
    sScene.ground = groundCreate("assets/ground/ground.obj");

    /* initialize camera */
    sScene.camera = cameraCreate(width, height, to_radians(45.0f), 0.01f, 500.0f, {12.0f, 4.0f, -12.0f});
    sScene.cameraFollowPickup = false;
    sScene.zoomSpeedMultiplier = 0.05f;

    /* Light positions and strength: see for examples - https://learnopengl.com/Lighting/Light-casters */
    Vector3D headLightDir = normalize(Vector3D{0.0f, -0.4f, 1.0f});
    Vector3D tailLightDir = normalize(Vector3D{0.0f, -0.3f, -1.0f});
    sScene.lightSpots[0] = { sScene.pickup.lightFLPos, headLightDir, { 1.0f, 1.0f, 1.0f }, 0.4f, 0.01f, 0.01f, to_radians(50.0f), sScene.lightsOn };    
    sScene.lightSpots[1] = { sScene.pickup.lightFRPos, headLightDir, { 1.0f, 1.0f, 1.0f }, 0.4f, 0.01f, 0.01f, to_radians(50.0f), sScene.lightsOn }; 
    sScene.lightSpots[2] = { sScene.pickup.lightBLPos, tailLightDir, { 0.6f, 0.1f, 0.1f }, 1.0f, 0.0045f, 0.075f, to_radians(80.0f), sScene.lightsOn };    
    sScene.lightSpots[3] = { sScene.pickup.lightBRPos, tailLightDir, { 0.6f, 0.1f, 0.1f }, 1.0f, 0.0045f, 0.075f, to_radians(80.0f), sScene.lightsOn }; 

    /* load shader from file */
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/color.frag");
    sScene.shaderNormal = shaderLoad("shader/default.vert", "shader/normal.frag");
    sScene.shaderBlinnPhong = shaderLoad("shader/default.vert", "shader/blinn_phong.frag");

    setPickupLights(true);
    setDay(true);
    sScene.renderMode = eRenderMode::BLINN_PHONG;
}

/* function to move and update objects in scene (e.g., move pickup according to user input) */
void sceneUpdate(float dt) {
    pickupMove(sScene.pickup, sScene.ground, sInput.buttonPressed, dt);

    if (sScene.cameraFollowPickup) {
        sScene.camera.lookAt = sScene.pickup.position + Vector3D{0.0f, 1.0f, 0.0f};
    }
}

/* function to set light uniforms */
void setLights(ShaderProgram& shader) {
    /* set light directional source */
    shaderUniform(shader, "uLightSun.direction", sScene.lightSun.direction);
    shaderUniform(shader, "uLightSun.ambient", sScene.lightSun.ambient);
    shaderUniform(shader, "uLightSun.color", sScene.lightSun.color);

    /* set the pickup's spot lights */
    for(int i = 0; i < 4; i++) {
        Vector4D pos = sScene.pickup.transformation * Vector4D(sScene.lightSpots[i].position);
        Vector4D dir = normalize(Matrix4D(Matrix3D(sScene.pickup.transformation)) * Vector4D(sScene.lightSpots[i].direction));

        std::string light = "uLightSpots[" + std::to_string(i) + "]";
        shaderUniform(shader, light + ".position", Vector3D{pos.x, pos.y, pos.z});
        shaderUniform(shader, light + ".direction", Vector3D{dir.x, dir.y, dir.z});
        shaderUniform(shader, light + ".color", sScene.lightSpots[i].color);
        shaderUniform(shader, light + ".constant", sScene.lightSpots[i].constant);
        shaderUniform(shader, light + ".linear", sScene.lightSpots[i].linear);
        shaderUniform(shader, light + ".quadratic", sScene.lightSpots[i].quadratic);
        shaderUniform(shader, light + ".cutoff", sScene.lightSpots[i].cutoff);
        shaderUniform(shader, light + ".enabled", sScene.lightSpots[i].enabled);
    }
}

/* function to set material uniforms and textures */
void setMaterial(ShaderProgram& shader, Material& material) {
    /* set material properties */
    shaderUniform(shader, "uMaterial.emission", material.emission);
    shaderUniform(shader, "uMaterial.ambient", material.ambient);
    shaderUniform(shader, "uMaterial.diffuse", material.diffuse);
    shaderUniform(shader, "uMaterial.specular", material.specular);
    shaderUniform(shader, "uMaterial.shininess", material.shininess);
}

/* 
 * function to render all objects in the scene using their diffuse colors or their normals
 */
void renderColor(ShaderProgram& shader, bool renderNormal) {
    /* setup camera and model matrices */
    Matrix4D proj = cameraProjection(sScene.camera);
    Matrix4D view = cameraView(sScene.camera);
    glUseProgram(shader.id);
    shaderUniform(shader, "uProj",  proj);
    shaderUniform(shader, "uView",  view);
    shaderUniform(shader, "uModel",  sScene.pickup.transformation);
    shaderUniform(shader, "uSquashRatio", sScene.pickup.squashRatio);
    shaderUniform(shader, "isWheel", false);
    if (renderNormal) {
        shaderUniform(shader, "uViewPos", cameraPosition(sScene.camera));
        shaderUniform(shader, "isGround", false);
    }

    /* render pickup */
    for(unsigned int i = 0; i < sScene.pickup.partModel.size(); i++) {
        auto& model = sScene.pickup.partModel[i];
        auto& transform = sScene.pickup.partTransformations[i];
        glBindVertexArray(model.mesh.vao);

        shaderUniform(shader, "uLocalModel", transform);
        shaderUniform(shader, "uModel", sScene.pickup.transformation);

        /* set animation properties */
        if (std::find(std::begin(sScene.pickup.ePartWheels), std::end(sScene.pickup.ePartWheels), i) != std::end(sScene.pickup.ePartWheels)) {            
            float squashFactor = (i == Pickup::ePart::WHEEL_BR || i == Pickup::ePart::WHEEL_BL) ? sScene.pickup.backSquashFactor : sScene.pickup.frontSquashFactor;
            shaderUniform(shader, "uSquashFactor", squashFactor);
            shaderUniform(shader, "isWheel", true);
        } else {
            shaderUniform(shader, "isWheel", false);
        }

        for(auto& material : model.material) {
            if (!renderNormal) {
                /* set material properties */
                shaderUniform(shader, "uMaterial.diffuse", material.diffuse);
            }
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset * sizeof(unsigned int)) );
        }
    }

    /* render ground */
    {
        auto& model = sScene.ground.model;
        shaderUniform(shader, "uModel", Matrix4D::identity());

        glBindVertexArray(model.mesh.vao);
        for(auto& material : model.material) {
            if (!renderNormal) {
                /* set material properties */
                shaderUniform(shader, "uMaterial.diffuse", material.diffuse);
            }
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
        }
    }

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

/* function to render all objects in the scene using the blinn-phong shading model */
void renderBlinnPhong(ShaderProgram& shader) {
    /* setup camera and model matrices */
    Matrix4D proj = cameraProjection(sScene.camera);
    Matrix4D view = cameraView(sScene.camera);
    glUseProgram(shader.id);
    shaderUniform(shader, "uProj",  proj);
    shaderUniform(shader, "uView",  view);
    shaderUniform(shader, "uModel",  sScene.pickup.transformation);
    shaderUniform(shader, "uViewPos", cameraPosition(sScene.camera));
    shaderUniform(shader, "isGround", false);
    shaderUniform(shader, "uSquashRatio", sScene.pickup.squashRatio);
    shaderUniform(shader, "isWheel", false);

    setLights(shader);

    /* render pickup */
    for(unsigned int i = 0; i < sScene.pickup.partModel.size(); i++) {
        auto& model = sScene.pickup.partModel[i];
        auto& transform = sScene.pickup.partTransformations[i];
        glBindVertexArray(model.mesh.vao);

        shaderUniform(shader, "uLocalModel", transform);
        shaderUniform(shader, "uModel", sScene.pickup.transformation);

        /* set animation properties */
        if (std::find(std::begin(sScene.pickup.ePartWheels), std::end(sScene.pickup.ePartWheels), i) != std::end(sScene.pickup.ePartWheels)) {            
            float squashFactor = (i == Pickup::ePart::WHEEL_BR || i == Pickup::ePart::WHEEL_BL) ? sScene.pickup.backSquashFactor : sScene.pickup.frontSquashFactor;
            shaderUniform(shader, "uSquashFactor", squashFactor);
            shaderUniform(shader, "isWheel", true);
        } else {
            shaderUniform(shader, "isWheel", false);
        }

        for(auto& material : model.material) {
            /* set material properties */
            setMaterial(shader, material);
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset * sizeof(unsigned int)) );
        }
    }

    /* render ground */
    {
        shaderUniform(shader, "uModel", Matrix4D::identity());
        shaderUniform(shader, "uLocalModel", Matrix4D::identity());
        shaderUniform(shader, "isGround", true);

        auto& model = sScene.ground.model;

        glBindVertexArray(model.mesh.vao);
        for(auto& material : model.material) {
            /* set material properties */
            setMaterial(shader, material);
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset * sizeof(unsigned int)) );
        }
    }

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

/* function to draw all objects in the scene */
void sceneDraw() {
    if (sScene.isDay) {
        glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
    } else {
        glClearColor(0.05, 0.1, 0.2, 1.0);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*------------ render scene -------------*/
    {
        if (sScene.renderMode == eRenderMode::COLOR) {
            renderColor(sScene.shaderColor, false);
        } else if (sScene.renderMode == eRenderMode::NORMAL) {
            renderColor(sScene.shaderNormal, true);
        } else if (sScene.renderMode == eRenderMode::BLINN_PHONG) {
            renderBlinnPhong(sScene.shaderBlinnPhong);
        }
    }
    glCheckError();

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

int main(int argc, char **argv) {
    /* create window/context */
    int width = 1280;
    int height = 720;
    GLFWwindow *window = windowCreate("Assignment 3 - Transformations, User Input and Camera", width, height);
    if (!window) {
        return EXIT_FAILURE;
    }

    /* set window callbacks */
    glfwSetKeyCallback(window, callbackKey);
    glfwSetCursorPosCallback(window, callbackMousePos);
    glfwSetMouseButtonCallback(window, callbackMouseButton);
    glfwSetScrollCallback(window, callbackMouseScroll);
    glfwSetFramebufferSizeCallback(window, callbackWindowResize);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* setup scene */
    sceneInit(width, height);

    /*-------------- main loop ----------------*/
    double timeStamp = glfwGetTime();
    double timeStampNew = 0.0;

    /* loop until user closes window */
    while (!glfwWindowShouldClose(window)) {
        /* poll and process input and window events */
        glfwPollEvents();

        /* update camera and model matrices */
        timeStampNew = glfwGetTime();
        sceneUpdate(static_cast<float>(timeStampNew - timeStamp));
        timeStamp = timeStampNew;

        /* draw all objects in the scene */
        sceneDraw();

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }

    /*-------- cleanup --------*/
    /* delete opengl shader and buffers */
    shaderDelete(sScene.shaderColor);
    shaderDelete(sScene.shaderNormal);
    shaderDelete(sScene.shaderBlinnPhong);
    pickupDelete(sScene.pickup);
    groundDelete(sScene.ground);

    /* cleanup glfw/glcontext */
    windowDelete(window);

    return EXIT_SUCCESS;
}
