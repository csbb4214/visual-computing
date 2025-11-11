#include <cstdlib>
#include <iostream>

#include "mygl/camera.h"
#include "mygl/geometry.h"
#include "mygl/mesh.h"
#include "mygl/shader.h"

#include "ground.h"
#include "pickup.h"

/* struct holding all necessary state variables for scene */
struct {
    Camera camera;
    bool cameraFollowPickup;
    float zoomSpeedMultiplier;

    Ground ground;
    Pickup pickup;

    ShaderProgram shaderColor;
} sScene;

/* calculate how much the car approximately turns per meter travelled for a given turning angle */
float calculateTurningAnglePerMeter(float wheelBase, float turningAngle, float width) {
    /* according to https://calculator.academy/turning-radius-calculator/ and
     * https://gamedev.stackexchange.com/questions/50022/typical-maximum-steering-angle-of-a-real-car/50029 */
    float turningRadius = wheelBase / tan(turningAngle);
    float innerTurningRadius = turningRadius - width;
    return 360.0f / (2.0f * innerTurningRadius * static_cast<float>(M_PI));
}

/* struct holding all state variables for input */
struct {
    bool mouseLeftButtonPressed = false;
    Vector2D mousePressStart;
    bool buttonPressed[4] = {false, false, false, false};
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

    /* input for car control */
    if (key == GLFW_KEY_W) {
        sInput.buttonPressed[0] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_S) {
        sInput.buttonPressed[1] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if (key == GLFW_KEY_A) {
        sInput.buttonPressed[2] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_D) {
        sInput.buttonPressed[3] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    /* camera mode 1*/
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        sScene.cameraFollowPickup = false;
        sScene.camera.lookAt = {0.0f, 0.0f, 0.0f};
    }


    /* camera mode 2*/
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        sScene.cameraFollowPickup = true;
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
    sScene.camera.width = width;
    sScene.camera.height = height;
}

/* function to setup and initialize the whole scene */
void sceneInit(float width, float height) {
    
    /* initialize camera */
    sScene.camera = cameraCreate(width, height, to_radians(45.0f), 0.01f, 500.0f, {12.0f, 4.0f, -12.0f});
    /* set camera at the origin of the world coordinate system*/
    sScene.camera.lookAt = {0.0f, 0.0f, 0.0f};
    sScene.cameraFollowPickup = false;
    sScene.zoomSpeedMultiplier = 0.05f;

    /* Colors */
    Vector3D colorGround = {0.15f, 0.45f, 0.15f};
    Vector4D colorBase = {0.1f, 0.1f, 0.5f, 1.0f};
    Vector4D colorCockpit = {0.1f, 0.1f, 0.8f, 1.0f};
    Vector4D colorWheels = {0.15f, 0.15f, 0.15f, 1.0f};

    /* setup objects in scene and create opengl buffers for meshes */
    sScene.ground = groundCreate(colorGround);
    sScene.pickup = pickupCreate(colorBase, colorCockpit, colorWheels);

    /* load shader from file */
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/default.frag");
}

/* function to move and update objects in scene (e.g., move car according to user input) */
void sceneUpdate(float dt) {
    /* if 'w' or 's' pressed, cube should rotate around x axis */
    int rotationDirX = 0;
    if (sInput.buttonPressed[0]) {
        rotationDirX = -1;
    } else if (sInput.buttonPressed[1]) {
        rotationDirX = 1;
    }

    /* if 'a' or 'd' pressed, cube should rotate around y axis */
    int rotationDirY = 0;
    if (sInput.buttonPressed[2]) {
        rotationDirY = -1;
    } else if (sInput.buttonPressed[3]) {
        rotationDirY = 1;
    }

    /* udpate cube transformation matrix to include new rotation if one of the keys was pressed */
    if (rotationDirX != 0 || rotationDirY != 0) {
        // sScene.cubeTransformationMatrix = Matrix4D::rotationY(rotationDirY * sScene.cubeSpinRadPerSecond * dt) * Matrix4D::rotationX(rotationDirX * sScene.cubeSpinRadPerSecond * dt) * sScene.cubeTransformationMatrix;
    }

    /* if camera mode 2 is activated, set the camera to the pos of the pickup*/
    if (sScene.cameraFollowPickup) {
    // Vector3D pickupPos = getPickupWorldPosition();
    Vector3D pickupPos = {2.0f, 3.0f, 0.0f}; // pos of the cube for testing
    sScene.camera.lookAt = pickupPos;
    }
}

/* function to draw all objects in the scene */
void sceneDraw() {
    glClearColor(135.0f / 255, 206.0f / 255, 235.0f / 255, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(sScene.shaderColor.id);
    shaderUniform(sScene.shaderColor, "uProj", cameraProjection(sScene.camera));
    shaderUniform(sScene.shaderColor, "uView", cameraView(sScene.camera));

    // Draw ground
    shaderUniform(sScene.shaderColor, "uModel", Matrix4D::identity());
    glBindVertexArray(sScene.ground.mesh.vao);
    glDrawElements(GL_TRIANGLES, sScene.ground.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);

    // Draw pickup
    pickupDraw(sScene.pickup, sScene.shaderColor);

    glCheckError();
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
        sceneUpdate(timeStampNew - timeStamp);
        timeStamp = timeStampNew;

        /* draw all objects in the scene */
        sceneDraw();

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }

    /*-------- cleanup --------*/
    /* delete opengl shader and buffers */
    shaderDelete(sScene.shaderColor);
    groundDelete(sScene.ground);
    pickupDelete(sScene.pickup);

    /* cleanup glfw/glcontext */
    windowDelete(window);

    return EXIT_SUCCESS;
}
