#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "mygl/camera.h"
#include "mygl/geometry.h"
#include "mygl/shader.h"
#include "mygl/mesh.h"
#include "mygl/texture.h"

#include <glm/gtc/matrix_transform.hpp>

struct MaterialSphere {
    glm::vec3 position;
    float metallic;
    float roughness;
    glm::vec3 albedo;
};

struct {
    Camera camera;
    Mesh sphereMesh;
    std::vector<MaterialSphere> materialSpheres;
    int rendermode = 0;

    ShaderProgram shaderPBR;

    // --- Planet OBJ ---
    std::vector<Mesh> planetMeshes;
    glm::mat4 planetModel = glm::mat4(1.0f);

    // --- Optional PBR textures (planet) ---
    bool useTextures = false;
    GLuint texAlbedo = 0;
    GLuint texMetallic = 0;
    GLuint texRoughness = 0;
    GLuint texAO = 0;

    float exposure = 1.0f;

    // Light properties
    glm::vec3 lightPositions[5];
    glm::vec3 lightColors[5];

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
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
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
    cameraUpdateOrbit(sScene.camera, {0, 0}, yoffset * 0.5);
}

void window_resize_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    sScene.camera.width = width;
    sScene.camera.height = height;
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
    GLFWwindow *window = windowCreate("PBR Renderer - Material Grid", 1280, 720);

    /* set window callbacks */
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(window, window_resize_callback);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* create a sphere mesh (using UV sphere generation) */
    sScene.sphereMesh = createUVSphereMesh(64, 64, 1.0f);

    /* create material grid */
    sScene.materialSpheres = createMaterialGrid();

    /* load PBR shader from file */
    sScene.shaderPBR = shaderLoad("shader/pbr.vert", "shader/pbr.frag");

    // --- load planet OBJ (tinyobj) ---
    sScene.planetMeshes = meshLoadFromObj("assets/planet/cute-little-planet.obj");

    // transform so it sits above/behind the grid a bit
    sScene.planetModel = glm::mat4(1.0f);
    sScene.planetModel = glm::translate(sScene.planetModel, glm::vec3(0.0f, 0.0f, -8.0f));
    sScene.planetModel = glm::scale(sScene.planetModel, glm::vec3(3.0f));

    // --- load example textures (optional) ---
    // IMPORTANT: These files may not exist yet -> loader uses magenta fallback.
    // Put your textures into: pbr/assets/textures/ (create folder)
    sScene.texAlbedo   = textureLoad2D("assets/textures/albedo.png", true);   // sRGB
    sScene.texMetallic = textureLoad2D("assets/textures/metallic.png", false);
    sScene.texRoughness= textureLoad2D("assets/textures/roughness.png", false);
    sScene.texAO       = textureLoad2D("assets/textures/ao.png", false);

    // start with textures OFF (toggle later)
    sScene.useTextures = false;
    sScene.exposure = 2.0f;


    /* create camera - positioned to see the grid */
    sScene.camera = cameraCreate(1280, 720, glm::radians(45.0f), 0.1f, 500.0f, {0.0f, 0.0f, 35.0f});

    /* setup lights in a square around the grid */
    // --- PBR test lights (for spheres) ---
    sScene.lightPositions[0] = glm::vec3(-10.0f,  10.0f, 10.0f);
    sScene.lightPositions[1] = glm::vec3( 10.0f,  10.0f, 10.0f);
    sScene.lightPositions[2] = glm::vec3(-10.0f, -10.0f, 10.0f);
    sScene.lightPositions[3] = glm::vec3( 10.0f, -10.0f, 10.0f);

    // --- NEW: strong key light for planet ---
    sScene.lightPositions[4] = glm::vec3(0.0f, 5.0f, -4.0f);  // slightly above & in front of planet

    sScene.lightColors[0] = glm::vec3(300.0f);
    sScene.lightColors[1] = glm::vec3(300.0f);
    sScene.lightColors[2] = glm::vec3(300.0f);  
    sScene.lightColors[3] = glm::vec3(300.0f);

    // brighter and slightly warm key light
    sScene.lightColors[4] = glm::vec3(800.0f, 700.0f, 600.0f);


    std::cout << "PBR Material Grid Controls:" << std::endl;
    std::cout << "  - Mouse drag to rotate camera" << std::endl;
    std::cout << "  - Mouse wheel to zoom" << std::endl;
    std::cout << "  - R to toggle wireframe" << std::endl;
    std::cout << "  - P for screenshot" << std::endl;
    std::cout << "  - ESC to exit" << std::endl;
    std::cout << "\nGrid layout:" << std::endl;
    std::cout << "  - Columns (left to right): Metallic 0.0 to 1.0" << std::endl;
    std::cout << "  - Rows (bottom to top): Roughness 0.05 to 1.0" << std::endl;

    /*-------------- main loop ----------------*/
    float t = 0.0f;
    /* loop until user closes window */
    while (!glfwWindowShouldClose(window)) {
        /* poll and process input and window events */
        glfwPollEvents();
        t += 1.0f / 60.0f;

        /*------------ render scene -------------*/
        {
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Dark background
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            /* setup camera matrices */
            glm::mat4 proj = cameraProjection(sScene.camera);
            glm::mat4 view = cameraView(sScene.camera);
            glm::vec3 camPos = sScene.camera.position;

            glUseProgram(sScene.shaderPBR.id);
            shaderUniform(sScene.shaderPBR, "uProj", proj);
            shaderUniform(sScene.shaderPBR, "uView", view);
            shaderUniform(sScene.shaderPBR, "uCamPos", camPos);

            shaderUniform(sScene.shaderPBR, "uExposure", sScene.exposure);
            shaderUniform(sScene.shaderPBR, "uUseTextures", (int)sScene.useTextures);

            // texture units (match pbr.frag)
            shaderUniform(sScene.shaderPBR, "uAlbedoMap", 0);
            shaderUniform(sScene.shaderPBR, "uMetallicMap", 1);
            shaderUniform(sScene.shaderPBR, "uRoughnessMap", 2);
            shaderUniform(sScene.shaderPBR, "uAOMap", 3);

            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sScene.texAlbedo);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, sScene.texMetallic);
            glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, sScene.texRoughness);
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, sScene.texAO);


            /* set light uniforms */
            /* set light uniforms */
            for (int i = 0; i < 5; ++i) {
                shaderUniform(sScene.shaderPBR,
                            ("uLightPositions[" + std::to_string(i) + "]").c_str(),
                            sScene.lightPositions[i]);

                shaderUniform(sScene.shaderPBR,
                            ("uLightColors[" + std::to_string(i) + "]").c_str(),
                            sScene.lightColors[i]);
            }


            /* render each sphere with its material properties */
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

            // --- render planet OBJ ---
            // use a single material (uniform fallback), textures optional
            shaderUniform(sScene.shaderPBR, "uAlbedo", glm::vec3(0.9f, 0.85f, 0.7f));
            shaderUniform(sScene.shaderPBR, "uMetallic", 0.0f);
            shaderUniform(sScene.shaderPBR, "uRoughness", 0.5f);
            shaderUniform(sScene.shaderPBR, "uAO", 1.0f);

            for (const auto& m : sScene.planetMeshes) {
                shaderUniform(sScene.shaderPBR, "uModel", sScene.planetModel);
                glBindVertexArray(m.vao);
                glDrawElements(GL_TRIANGLES, m.size_ibo, GL_UNSIGNED_INT, nullptr);
            }

            // restore sphere vao for safety (optional)
            glBindVertexArray(sScene.sphereMesh.vao);


            /* cleanup opengl state */
            glBindVertexArray(0);
            glUseProgram(0);
        }

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }

    /*-------- cleanup --------*/
    /* delete opengl shader and buffers */
    shaderDelete(sScene.shaderPBR);
    meshDelete(sScene.sphereMesh);

    /* destroy window/context */
    windowDelete(window);

    return EXIT_SUCCESS;
}