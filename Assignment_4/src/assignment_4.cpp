#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <array>

#include "mygl/camera.h"
#include "mygl/mesh.h"
#include "mygl/shader.h"

#include "ground.h"
#include "pickup.h"

/* enum for different render modes */
enum eRenderMode
{
    COLOR = 0,    // render diffuse colors
    NORMAL,       // render normals
    MODE_COUNT
};

/* struct holding all necessary state variables for scene */
struct {
    /* camera */
    Camera camera;
    bool cameraFollowPickup;
    float zoomSpeedMultiplier;

    /* game objects */
    Ground ground;
    Pickup pickup;

    /* shader */
    eRenderMode renderMode;
    ShaderProgram shaderColor;
    ShaderProgram shaderNormal;

    Matrix4D lightRotation;

    /* lighting */
    bool isDayTime;
} sScene;

/* struct holding all state variables for input */
struct {
    bool mouseLeftButtonPressed = false;
    Vector2D mousePressStart;
    bool buttonPressed[Pickup::eControl::CONTROL_COUNT] = {false, false, false, false};
} sInput;

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
    }

    /* toggle day/night */
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        sScene.isDayTime = !sScene.isDayTime;
    }
}

/* GLFW callback function for mouse position events */
void callbackMousePos(GLFWwindow *window, double x, double y) {
    /* called on cursor position change */
    if (sInput.mouseLeftButtonPressed) {
        Vector2D diff = sInput.mousePressStart - Vector2D(static_cast<float>(x), static_cast<float>(y));
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = Vector2D(static_cast<float>(x), static_cast<float>(y));
    }
}

/* GLFW callback function for mouse button events */
void callbackMouseButton(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        sInput.mouseLeftButtonPressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = Vector2D(static_cast<float>(x), static_cast<float>(y));
    }
}

/* GLFW callback function for mouse scroll events */
void callbackMouseScroll(GLFWwindow *window, double xoffset, double yoffset) {
    cameraUpdateOrbit(sScene.camera, {0, 0}, static_cast<float>(-sScene.zoomSpeedMultiplier * yoffset));
}

/* GLFW callback function for window resize event */
void callbackWindowResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    sScene.camera.width = static_cast<float>(width);
    sScene.camera.height = static_cast<float>(height);
}

/* function to setup and initialize the whole scene */
void sceneInit(float width, float height) {
    sScene.pickup = pickupLoad("assets/pickup/pickup-simple.obj");
    sScene.ground = groundCreate("assets/ground/ground.obj");

    /* initialize camera */
    sScene.camera = cameraCreate(width, height, static_cast<float>(to_radians(45.0f)), 0.01f, 500.0f, {12.0f, 4.0f, -12.0f});
    sScene.cameraFollowPickup = false;
    sScene.zoomSpeedMultiplier = 0.05f;

    /* load shader from file */
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/color.frag");
    sScene.shaderNormal = shaderLoad("shader/default.vert", "shader/normal.frag");

    sScene.renderMode = eRenderMode::COLOR;
    sScene.isDayTime = true;
}

/* function to move and update objects in scene (e.g., move car according to user input) */
void sceneUpdate(float dt) {
    pickupMove(sScene.pickup, sScene.ground, sInput.buttonPressed, dt);

    if (sScene.cameraFollowPickup) {
        sScene.camera.lookAt = sScene.pickup.position + Vector3D{0.0f, 1.0f, 0.0f};
    }
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
    if (renderNormal) {
        shaderUniform(shader, "uViewPos", cameraPosition(sScene.camera));
        shaderUniform(shader, "isGround", false);
    } else {
        // Set lighting uniforms for Blinn-Phong
        Vector3D lightDir = {-0.6f, -2.0f, -1.0f}; // Directional light pointing down and forward

        if (sScene.isDayTime) {
            // Day lighting - brighter, warmer sunlight
            shaderUniform(shader, "uLightAmbient", Vector3D{0.4f, 0.4f, 0.45f});   // Soft blue-ish ambient
            shaderUniform(shader, "uLightDiffuse", Vector3D{0.9f, 0.85f, 0.7f});   // Warm sunlight
            shaderUniform(shader, "uLightSpecular", Vector3D{1.0f, 0.95f, 0.8f});  // Warm highlights
        } else {
            // Night lighting - cooler, dimmer moonlight
            shaderUniform(shader, "uLightAmbient", Vector3D{0.08f, 0.08f, 0.12f}); // Very dim blue ambient
            shaderUniform(shader, "uLightDiffuse", Vector3D{0.25f, 0.25f, 0.4f});  // Cool blue moonlight
            shaderUniform(shader, "uLightSpecular", Vector3D{0.4f, 0.4f, 0.6f});   // Cool blue highlights
        }

        shaderUniform(shader, "uLightDir", lightDir);
    }

    // sqash ratio as shader uniform
    shaderUniform(shader, "uSquashRatio", sScene.pickup.squashRatio);

    /* render pickup */
    for(unsigned int i = 0; i < sScene.pickup.partModel.size(); i++) {
        auto& model = sScene.pickup.partModel[i];
        auto& transform = sScene.pickup.partTransformations[i];
        glBindVertexArray(model.mesh.vao);

        shaderUniform(shader, "uLocalModel", transform);
        shaderUniform(shader, "uModel", sScene.pickup.transformation);

        // Set appropriate squash factor for each wheel
        float squashFactor = 0.0f;
        if (i == Pickup::ePart::WHEEL_FL || i == Pickup::ePart::WHEEL_FR) {
            // Front wheels use frontSquashFactor
            squashFactor = sScene.pickup.frontSquashFactor;
        } else if (i == Pickup::ePart::WHEEL_BL || i == Pickup::ePart::WHEEL_BR) {
            // Back wheels use backSquashFactor
            squashFactor = sScene.pickup.backSquashFactor;
        }

        shaderUniform(shader, "uSquashFactor", squashFactor);

        for(auto& material : model.material) {
            if (!renderNormal) {
                /* set material properties */
                shaderUniform(shader, "uMaterial.ambient", material.ambient);
                shaderUniform(shader, "uMaterial.diffuse", material.diffuse);
                shaderUniform(shader, "uMaterial.specular", material.specular);
                shaderUniform(shader, "uMaterial.shininess", material.shininess);
            }
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset * sizeof(unsigned int)));
        }
    }

    /* render ground */
    {
        auto& model = sScene.ground.model;
        shaderUniform(shader, "uModel", Matrix4D::identity());

        // Ground doesn't get squashed
        shaderUniform(shader, "uSquashFactor", 0.0f);

        glBindVertexArray(model.mesh.vao);
        for(auto& material : model.material) {
            if (!renderNormal) {
                /* set material properties */
                shaderUniform(shader, "uMaterial.ambient", material.ambient);
                shaderUniform(shader, "uMaterial.diffuse", material.diffuse);
                shaderUniform(shader, "uMaterial.specular", material.specular);
                shaderUniform(shader, "uMaterial.shininess", material.shininess);
            } else {
                shaderUniform(shader, "isGround", true);
            }
            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)));
        }
    }

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

/* function to draw all objects in the scene */
void sceneDraw() {
    /* clear framebuffer color */
    glClearColor(135.0f / 255, 206.0f / 255, 235.0f / 255, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*------------ render scene -------------*/
    {
        if (sScene.renderMode == eRenderMode::COLOR) {
            renderColor(sScene.shaderColor, false);
        } else if (sScene.renderMode == eRenderMode::NORMAL) {
            renderColor(sScene.shaderNormal, true);
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
    sceneInit(static_cast<float>(width), static_cast<float>(height));

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
    pickupDelete(sScene.pickup);
    groundDelete(sScene.ground);

    /* cleanup glfw/glcontext */
    windowDelete(window);

    return EXIT_SUCCESS;
}
