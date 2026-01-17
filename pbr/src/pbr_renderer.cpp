#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "mygl/camera.h"
#include "mygl/geometry.h"
#include "mygl/mesh.h"
#include "mygl/shader.h"
#include "mygl/texture.h"

#include <glm/gtc/matrix_transform.hpp>

enum SceneType { SCENE_GRID = 0, SCENE_PLANET = 1 };

struct MaterialSphere {
    glm::vec3 position;
    float metallic;
    float roughness;
    glm::vec3 albedo;
};

struct {
    SceneType currentScene = SCENE_GRID;

    // Cameras for each scene
    Camera cameraGrid;
    Camera cameraPlanet;

    // Grid scene
    Mesh sphereMesh;
    std::vector<MaterialSphere> materialSpheres;

    // Planet scene
    std::vector<Mesh> planetMeshes;
    glm::mat4 planetModel = glm::mat4(1.0f);

    // Shared
    int rendermode = 0;
    ShaderProgram shaderPBR;

    // Textures
    bool useTextures = false;
    GLuint texAlbedo = 0;
    GLuint texMetallic = 0;
    GLuint texRoughness = 0;
    GLuint texAO = 0;

    float exposure = 1.0f;

    // Lights per scene
    glm::vec3 lightPositionsGrid[5];
    glm::vec3 lightColorsGrid[5];

    glm::vec3 lightPositionsPlanet[5];
    glm::vec3 lightColorsPlanet[5];

} sScene;


struct {
    bool mouseButtonPressed = false;
    glm::vec2 mousePressStart;
} sInput;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* called on keyboard event */

    /* close window on escape */
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        screenshotToPNG("screenshot.png");
    }

    /* switch render mode (polygons, wireframe, vertex points) */
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        sScene.rendermode = (sScene.rendermode + 1) % 3;

        if (sScene.rendermode == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else if (sScene.rendermode == 1) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        }
    }

    // Switch between scenes
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        sScene.currentScene = SCENE_GRID;
        std::cout << "[Scene] Switched to Material Grid" << std::endl;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        sScene.currentScene = SCENE_PLANET;
        std::cout << "[Scene] Switched to Planet" << std::endl;
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        sScene.useTextures = !sScene.useTextures;
        std::cout << "[Toggle] useTextures = " << (sScene.useTextures ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_KP_ADD && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        sScene.exposure *= 1.1f;
        std::cout << "[Exposure] " << sScene.exposure << std::endl;
    }
    if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        sScene.exposure /= 1.1f;
        std::cout << "[Exposure] " << sScene.exposure << std::endl;
    }
}

void mouse_pos_callback(GLFWwindow *window, double x, double y) {
    /* called on cursor position change */
    if (sInput.mouseButtonPressed) {
        glm::vec2 diff = sInput.mousePressStart - glm::vec2(x, y);

        if (sScene.currentScene == SCENE_GRID) {
            cameraUpdateOrbit(sScene.cameraGrid, diff, 0.0f);
        } else {
            cameraUpdateOrbit(sScene.cameraPlanet, diff, 0.0f);
        }

        sInput.mousePressStart = glm::vec2(x, y);
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    /* called on mouse button event */
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        sInput.mouseButtonPressed = (action == GLFW_PRESS);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = glm::vec2(x, y);
    }
}

void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (sScene.currentScene == SCENE_GRID) {
        cameraUpdateOrbit(sScene.cameraGrid, {0, 0}, yoffset * 0.5);
    } else {
        cameraUpdateOrbit(sScene.cameraPlanet, {0, 0}, yoffset * 0.5);
    }
}

void window_resize_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    // Update both cameras
    sScene.cameraGrid.width = width;
    sScene.cameraGrid.height = height;

    sScene.cameraPlanet.width = width;
    sScene.cameraPlanet.height = height;
}

void updateCameraGrid(Camera &cam, GLFWwindow *window) {
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);
    lastTime = currentTime;

    float moveSpeed = 10.0f * deltaTime;

    glm::vec3 front = glm::normalize(cam.lookAt - cam.position);
    glm::vec3 right = glm::normalize(glm::cross(front, cam.initUp));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cam.position += front * moveSpeed;
        cam.lookAt += front * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cam.position -= front * moveSpeed;
        cam.lookAt -= front * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cam.position -= right * moveSpeed;
        cam.lookAt -= right * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cam.position += right * moveSpeed;
        cam.lookAt += right * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        cam.position += up * moveSpeed;
        cam.lookAt += up * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cam.position -= up * moveSpeed;
        cam.lookAt -= up * moveSpeed;
    }
}

void updateCameraPlanet(Camera &cam, GLFWwindow *window) {
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);
    lastTime = currentTime;

    float moveSpeed = 10.0f * deltaTime;

    glm::vec3 front = glm::normalize(cam.lookAt - cam.position);
    glm::vec3 right = glm::normalize(glm::cross(front, cam.initUp));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cam.position += front * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cam.position -= front * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cam.position -= right * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cam.position += right * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        cam.position += up * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cam.position -= up * moveSpeed;
    }

    // Planet-specific: Q/E for zooming closer/further
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        cam.position += front * moveSpeed * 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        cam.position -= front * moveSpeed * 0.5f;
    }
}

std::vector<MaterialSphere> createMaterialGrid() {
    std::vector<MaterialSphere> spheres;

    int rows = 7; // roughness variation
    int cols = 7; // metallic variation
    float spacing = 2.5f;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            MaterialSphere sphere;
            sphere.position = glm::vec3((col - cols / 2.0f) * spacing, (row - rows / 2.0f) * spacing, 0.0f);
            sphere.roughness = glm::clamp((float)row / (float)(rows - 1), 0.05f, 1.0f);
            sphere.metallic = (float)col / (float)(cols - 1);

            // Create some color variety
            if (col < cols / 3) {
                sphere.albedo = glm::vec3(1.0f, 0.0f, 0.0f); // Red
            } else if (col < 2 * cols / 3) {
                sphere.albedo = glm::vec3(0.0f, 1.0f, 0.0f); // Green
            } else {
                sphere.albedo = glm::vec3(0.0f, 0.5f, 1.0f); // Blue
            }

            spheres.push_back(sphere);
        }
    }

    return spheres;
}

int main(int argc, char **argv) {
    /* create window/context */
    GLFWwindow *window = windowCreate("PBR Renderer - Material Grid & Planet", 1280, 720);

    /* set window callbacks */
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(window, window_resize_callback);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* create sphere mesh and grid */
    sScene.sphereMesh = createUVSphereMesh(64, 64, 1.0f);
    sScene.materialSpheres = createMaterialGrid();

    /* load PBR shaders */
    sScene.shaderPBR = shaderLoad("shader/pbr.vert", "shader/pbr.frag");

    /* load planet */
    sScene.planetMeshes = meshLoadFromObj("assets/planet/cute-little-planet.obj");
    sScene.planetModel = glm::mat4(1.0f);
    sScene.planetModel = glm::translate(sScene.planetModel, glm::vec3(0.0f, 0.0f, 0.0f)); // Centered
    sScene.planetModel = glm::scale(sScene.planetModel, glm::vec3(2.0f));                 // Smaller scale

    /* load textures */
    sScene.texAlbedo = textureLoad2D("assets/textures/albedo.png", true);
    sScene.texMetallic = textureLoad2D("assets/textures/metallic.png", false);
    sScene.texRoughness = textureLoad2D("assets/textures/roughness.png", false);
    sScene.texAO = textureLoad2D("assets/textures/ao.png", false);

    sScene.useTextures = false;
    sScene.exposure = 2.0f;

    /* setup cameras */
    sScene.cameraGrid = cameraCreate(1280, 720, glm::radians(45.0f), 0.1f, 500.0f, {0.0f, 0.0f, 35.0f}, {0, 0, 0}, {0, 1, 0});
    sScene.cameraPlanet = cameraCreate(1280, 720, glm::radians(45.0f), 0.1f, 500.0f, {0.0f, 5.0f, 15.0f}, {0, 0, 0}, {0, 1, 0});

    /* setup lights for grid scene */
    sScene.lightPositionsGrid[0] = glm::vec3(-10.0f, 10.0f, 10.0f);
    sScene.lightPositionsGrid[1] = glm::vec3(10.0f, 10.0f, 10.0f);
    sScene.lightPositionsGrid[2] = glm::vec3(-10.0f, -10.0f, 10.0f);
    sScene.lightPositionsGrid[3] = glm::vec3(10.0f, -10.0f, 10.0f);
    sScene.lightPositionsGrid[4] = glm::vec3(0.0f, 5.0f, -4.0f);

    for (int i = 0; i < 5; i++)
        sScene.lightColorsGrid[i] = glm::vec3(300.0f);
    sScene.lightColorsGrid[4] = glm::vec3(800.0f, 700.0f, 600.0f);

    /* setup lights for planet scene */
    sScene.lightPositionsPlanet[0] = glm::vec3(-8.0f, 8.0f, 8.0f);
    sScene.lightPositionsPlanet[1] = glm::vec3(8.0f, 8.0f, 8.0f);
    sScene.lightPositionsPlanet[2] = glm::vec3(-8.0f, -8.0f, 8.0f);
    sScene.lightPositionsPlanet[3] = glm::vec3(8.0f, -8.0f, 8.0f);
    sScene.lightPositionsPlanet[4] = glm::vec3(0.0f, 10.0f, -5.0f);

    sScene.lightColorsPlanet[0] = glm::vec3(400.0f, 350.0f, 300.0f);
    sScene.lightColorsPlanet[1] = glm::vec3(300.0f, 350.0f, 400.0f);
    sScene.lightColorsPlanet[2] = glm::vec3(300.0f);
    sScene.lightColorsPlanet[3] = glm::vec3(300.0f);
    sScene.lightColorsPlanet[4] = glm::vec3(1000.0f, 900.0f, 800.0f);

    /* print controls */
    std::cout << "PBR Renderer Controls:" << std::endl;
    std::cout << "  - 1: Material Grid Scene" << std::endl;
    std::cout << "  - 2: Planet Scene" << std::endl;
    std::cout << "  - Mouse drag to rotate camera" << std::endl;
    std::cout << "  - Mouse wheel to zoom" << std::endl;
    std::cout << "  - W/A/S/D/Space/Shift: Move camera" << std::endl;
    std::cout << "  - Q/E: Zoom in/out (planet scene)" << std::endl;
    std::cout << "  - R: Toggle wireframe" << std::endl;
    std::cout << "  - T: Toggle textures (planet scene)" << std::endl;
    std::cout << "  - P: Screenshot" << std::endl;
    std::cout << "  - ESC: Exit" << std::endl;

    /*-------------- main loop ----------------*/
    float t = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        t += 1.0f / 60.0f;

        /* update camera based on current scene */
        if (sScene.currentScene == SCENE_GRID) {
            updateCameraGrid(sScene.cameraGrid, window);
        } else {
            updateCameraPlanet(sScene.cameraPlanet, window);
        }

        /*------------ render scene -------------*/
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(sScene.shaderPBR.id);

        /* common shader uniforms */
        shaderUniform(sScene.shaderPBR, "uExposure", sScene.exposure);
        shaderUniform(sScene.shaderPBR, "uUseTextures", (int)sScene.useTextures);

        /* texture units */
        shaderUniform(sScene.shaderPBR, "uAlbedoMap", 0);
        shaderUniform(sScene.shaderPBR, "uMetallicMap", 1);
        shaderUniform(sScene.shaderPBR, "uRoughnessMap", 2);
        shaderUniform(sScene.shaderPBR, "uAOMap", 3);

        /* bind textures */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sScene.texAlbedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sScene.texMetallic);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sScene.texRoughness);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, sScene.texAO);

        if (sScene.currentScene == SCENE_GRID) {
            /* render grid scene */
            Camera &cam = sScene.cameraGrid;
            glm::mat4 proj = cameraProjection(cam);
            glm::mat4 view = cameraView(cam);
            glm::vec3 camPos = cam.position;

            shaderUniform(sScene.shaderPBR, "uProj", proj);
            shaderUniform(sScene.shaderPBR, "uView", view);
            shaderUniform(sScene.shaderPBR, "uCamPos", camPos);

            /* set grid lights */
            for (int i = 0; i < 5; ++i) {
                shaderUniform(sScene.shaderPBR, ("uLightPositions[" + std::to_string(i) + "]").c_str(), sScene.lightPositionsGrid[i]);
                shaderUniform(sScene.shaderPBR, ("uLightColors[" + std::to_string(i) + "]").c_str(), sScene.lightColorsGrid[i]);
            }

            /* render grid spheres */
            glBindVertexArray(sScene.sphereMesh.vao);
            for (const auto &sphere : sScene.materialSpheres) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, sphere.position);

                shaderUniform(sScene.shaderPBR, "uModel", model);
                shaderUniform(sScene.shaderPBR, "uAlbedo", sphere.albedo);
                shaderUniform(sScene.shaderPBR, "uMetallic", sphere.metallic);
                shaderUniform(sScene.shaderPBR, "uRoughness", sphere.roughness);
                shaderUniform(sScene.shaderPBR, "uAO", 1.0f);

                glDrawElements(GL_TRIANGLES, sScene.sphereMesh.size_ibo, GL_UNSIGNED_INT, nullptr);
            }
        } else {
            /* render planet scene */
            Camera &cam = sScene.cameraPlanet;
            glm::mat4 proj = cameraProjection(cam);
            glm::mat4 view = cameraView(cam);
            glm::vec3 camPos = cam.position;

            shaderUniform(sScene.shaderPBR, "uProj", proj);
            shaderUniform(sScene.shaderPBR, "uView", view);
            shaderUniform(sScene.shaderPBR, "uCamPos", camPos);

            /* set planet lights */
            for (int i = 0; i < 5; ++i) {
                shaderUniform(sScene.shaderPBR, ("uLightPositions[" + std::to_string(i) + "]").c_str(), sScene.lightPositionsPlanet[i]);
                shaderUniform(sScene.shaderPBR, ("uLightColors[" + std::to_string(i) + "]").c_str(), sScene.lightColorsPlanet[i]);
            }

            /* render planet */
            shaderUniform(sScene.shaderPBR, "uAlbedo", glm::vec3(0.9f, 0.85f, 0.7f));
            shaderUniform(sScene.shaderPBR, "uMetallic", 0.0f);
            shaderUniform(sScene.shaderPBR, "uRoughness", 0.5f);
            shaderUniform(sScene.shaderPBR, "uAO", 1.0f);

            for (const auto &m : sScene.planetMeshes) {
                shaderUniform(sScene.shaderPBR, "uModel", sScene.planetModel);
                glBindVertexArray(m.vao);
                glDrawElements(GL_TRIANGLES, m.size_ibo, GL_UNSIGNED_INT, nullptr);
            }
        }

        /* cleanup opengl state */
        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    /*-------- cleanup --------*/
    shaderDelete(sScene.shaderPBR);
    meshDelete(sScene.sphereMesh);
    windowDelete(window);

    return EXIT_SUCCESS;
}