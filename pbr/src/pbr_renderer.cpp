#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "mygl/camera.h"
#include "mygl/geometry.h"
#include "mygl/mesh.h"
#include "mygl/shader.h"
#include "mygl/texture.h"
#include "stb_image/stb_image.h"

#include <glm/gtc/matrix_transform.hpp>

enum SceneType { SCENE_GRID = 0, SCENE_PLANET = 1 };

struct MaterialSphere {
    glm::vec3 position;
    float metallic;
    float roughness;
    glm::vec3 albedo;
    int textureSetIndex;
};

struct TextureSet {
    std::string name;
    GLuint arm;     // ao + rough + metal
    GLuint diffuse; // albedo
    GLuint normal;
};

struct {
    SceneType currentScene = SCENE_GRID;

    // Cameras for each scene
    Camera cameraGrid;
    Camera cameraPlanet;

    // Grid scene
    Mesh sphereMesh;
    std::vector<MaterialSphere> materialSpheres;
    std::vector<TextureSet> textureSets;

    // Planet scene
    std::vector<Mesh> planetMeshes;
    glm::mat4 planetModel = glm::mat4(1.0f);

    // Shared
    int rendermode = 0;
    ShaderProgram shaderPBR;

    // Textures
    bool useTextures = false;

    float exposure = 1.0f;

    // IBL textures
    GLuint envCubemap = 0;    // Environment map (for skybox)
    GLuint irradianceMap = 0; // Diffuse IBL
    GLuint prefilterMap = 0;  // Specular IBL
    GLuint brdfLUT = 0;       // BRDF integration map

    bool useIBL = true;

    // Skybox render
    ShaderProgram shaderSkybox;
    Mesh skyboxMesh;

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

GLuint loadHDR(const char *path) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(path, &width, &height, &nrComponents, 0);

    GLuint hdrTexture = 0;
    if (data) {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "[IBL] Loaded HDR: " << path << std::endl;
    } else {
        std::cout << "[IBL] Failed to load HDR image: " << path << std::endl;
    }

    return hdrTexture;
}

GLuint createCubemap(int width, int height, GLenum internalFormat, GLenum format, GLenum type, bool mipmap) {
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (mipmap) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    } else {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return cubemap;
}

GLuint equirectangularToCubemap(GLuint hdrTexture, ShaderProgram &shader, int resolution = 512) {
    GLuint captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    GLuint envCubemap = createCubemap(resolution, resolution, GL_RGB16F, GL_RGB, GL_FLOAT, false);

    // Projection for capturing
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    // Create a cube mesh for rendering
    Mesh cubeMesh = createCubeMesh();

    glUseProgram(shader.id);
    shaderUniform(shader, "uEquirectangularMap", 0);
    shaderUniform(shader, "uProjection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; ++i) {
        shaderUniform(shader, "uView", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeMesh.vao);
        glDrawElements(GL_TRIANGLES, cubeMesh.size_ibo, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    meshDelete(cubeMesh);

    std::cout << "[IBL] Environment cubemap created" << std::endl;
    return envCubemap;
}

GLuint generateIrradianceMap(GLuint envCubemap, ShaderProgram &shader, int resolution = 32) {
    GLuint captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    GLuint irradianceMap = createCubemap(resolution, resolution, GL_RGB16F, GL_RGB, GL_FLOAT, true);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    Mesh cubeMesh = createCubeMesh();

    glUseProgram(shader.id);
    shaderUniform(shader, "uEnvironmentMap", 0);
    shaderUniform(shader, "uProjection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; ++i) {
        shaderUniform(shader, "uView", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeMesh.vao);
        glDrawElements(GL_TRIANGLES, cubeMesh.size_ibo, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    meshDelete(cubeMesh);

    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    std::cout << "[IBL] Irradiance map created" << std::endl;
    return irradianceMap;
}

GLuint generatePrefilterMap(GLuint envCubemap, ShaderProgram &shader, int resolution = 128) {
    GLuint prefilterMap = createCubemap(resolution, resolution, GL_RGB16F, GL_RGB, GL_FLOAT, true);

    GLuint captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    Mesh cubeMesh = createCubeMesh();

    glUseProgram(shader.id);
    shaderUniform(shader, "uEnvironmentMap", 0);
    shaderUniform(shader, "uProjection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = resolution * std::pow(0.5, mip);
        unsigned int mipHeight = resolution * std::pow(0.5, mip);

        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        shaderUniform(shader, "uRoughness", roughness);

        for (unsigned int i = 0; i < 6; ++i) {
            shaderUniform(shader, "uView", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(cubeMesh.vao);
            glDrawElements(GL_TRIANGLES, cubeMesh.size_ibo, GL_UNSIGNED_INT, 0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    meshDelete(cubeMesh);

    std::cout << "[IBL] Prefilter map created" << std::endl;
    return prefilterMap;
}

GLuint generateBRDFLUT(ShaderProgram &shader, int resolution = 512) {
    GLuint brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, resolution, resolution, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, resolution, resolution);
    glUseProgram(shader.id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render full-screen quad
    Mesh quadMesh = createQuadMesh();
    glBindVertexArray(quadMesh.vao);
    glDrawElements(GL_TRIANGLES, quadMesh.size_ibo, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    meshDelete(quadMesh);

    std::cout << "[IBL] BRDF LUT created" << std::endl;
    return brdfLUTTexture;
}

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

    if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        sScene.useIBL = !sScene.useIBL;
        std::cout << "[Toggle] useIBL = " << (sScene.useIBL ? "ON" : "OFF") << std::endl;
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
        cameraUpdateOrbit(sScene.cameraGrid, {0, 0}, -yoffset * 0.5);
    } else {
        cameraUpdateOrbit(sScene.cameraPlanet, {0, 0}, -yoffset * 0.5);
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

            // assign texture based on row (and loop if there are more rows than textures)
            int index = row % 5;
            sphere.textureSetIndex = index;

            // alternative values that are used when textures are disabled
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

TextureSet loadTextureSet(const std::string &path, const std::string &name) {
    TextureSet set;
    set.name = name;

    set.arm = textureLoad2D((path + name + "/arm.png").c_str(), false);
    set.diffuse = textureLoad2D((path + name + "/diff.png").c_str(), true);
    set.normal = textureLoad2D((path + name + "/nor.png").c_str(), false);

    std::cout << "[Textures] Loaded material: " << name << std::endl;
    return set;
}

void setupIBL() {
    // Load additional shaders for IBL preprocessing
    ShaderProgram equirectToCube = shaderLoad("shader/equirect_to_cube.vert", "shader/equirect_to_cube.frag");
    ShaderProgram irradianceShader = shaderLoad("shader/irradiance_conv.vert", "shader/irradiance_conv.frag");
    ShaderProgram prefilterShader = shaderLoad("shader/prefilter.vert", "shader/prefilter.frag");
    ShaderProgram brdfShader = shaderLoad("shader/brdf.vert", "shader/brdf.frag");

    // Load HDR environment
    GLuint hdrTexture = loadHDR("assets/hdri/sky.hdr");

    // Generate IBL maps
    sScene.envCubemap = equirectangularToCubemap(hdrTexture, equirectToCube, 512);
    sScene.irradianceMap = generateIrradianceMap(sScene.envCubemap, irradianceShader, 128);
    sScene.prefilterMap = generatePrefilterMap(sScene.envCubemap, prefilterShader, 128);
    sScene.brdfLUT = generateBRDFLUT(brdfShader, 512);

    // Cleanup
    glDeleteTextures(1, &hdrTexture);
    shaderDelete(equirectToCube);
    shaderDelete(irradianceShader);
    shaderDelete(prefilterShader);
    shaderDelete(brdfShader);

    std::cout << "[IBL] Setup complete!" << std::endl;
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

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* create sphere mesh and grid */
    sScene.sphereMesh = createUVSphereMesh(64, 64, 1.0f);
    sScene.materialSpheres = createMaterialGrid();

    /* load PBR shaders */
    sScene.shaderPBR = shaderLoad("shader/pbr.vert", "shader/pbr.frag");
    sScene.shaderSkybox = shaderLoad("shader/skybox.vert", "shader/skybox.frag");

    setupIBL();

    std::cout << "[Debug] envCubemap ID: " << sScene.envCubemap << std::endl;
    std::cout << "[Debug] irradianceMap ID: " << sScene.irradianceMap << std::endl;
    std::cout << "[Debug] prefilterMap ID: " << sScene.prefilterMap << std::endl;
    std::cout << "[Debug] brdfLUT ID: " << sScene.brdfLUT << std::endl;

    glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.envCubemap);
    GLint width1;
    glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &width1);
    std::cout << "[Debug] Cubemap face width: " << width1 << std::endl;

    // Restore proper viewport after IBL setup
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    sScene.skyboxMesh = createCubeMesh();

    /* load planet */
    sScene.planetMeshes = meshLoadFromObj("assets/planet/cute-little-planet.obj");
    sScene.planetModel = glm::mat4(1.0f);
    sScene.planetModel = glm::translate(sScene.planetModel, glm::vec3(0.0f, 0.0f, 0.0f)); // Centered
    sScene.planetModel = glm::scale(sScene.planetModel, glm::vec3(2.0f));                 // Smaller scale

    /* load textures */
    sScene.textureSets.push_back(loadTextureSet("assets/textures/", "blue_metal_plate"));
    sScene.textureSets.push_back(loadTextureSet("assets/textures/", "diagonal_parquet"));
    sScene.textureSets.push_back(loadTextureSet("assets/textures/", "old_stone_wall"));
    sScene.textureSets.push_back(loadTextureSet("assets/textures/", "painted_concrete"));
    sScene.textureSets.push_back(loadTextureSet("assets/textures/", "stained_pine"));

    std::cout << "[Textures] Loaded " << sScene.textureSets.size() << " material sets" << std::endl;

    sScene.useTextures = false;
    sScene.exposure = 2.0f;

    /* setup cameras */
    sScene.cameraGrid = cameraCreate(1280, 720, glm::radians(45.0f), 0.1f, 500.0f, {0.0f, 0.0f, 35.0f}, {0, 0, 0}, {0, 1, 0});
    sScene.cameraPlanet = cameraCreate(1280, 720, glm::radians(45.0f), 0.1f, 500.0f, {0.0f, 160.0f, 25.0f}, {0, 0, 0}, {0, 1, 0});

    /* setup lights for grid scene */
    sScene.lightPositionsGrid[0] = glm::vec3(-10.0f, 10.0f, 10.0f);
    sScene.lightPositionsGrid[1] = glm::vec3(10.0f, 10.0f, 10.0f);
    sScene.lightPositionsGrid[2] = glm::vec3(-10.0f, -10.0f, 10.0f);
    sScene.lightPositionsGrid[3] = glm::vec3(10.0f, -10.0f, 10.0f);
    sScene.lightPositionsGrid[4] = glm::vec3(0.0f, 5.0f, -4.0f);

    for (int i = 0; i < 5; i++)
        sScene.lightColorsGrid[i] = glm::vec3(50.0f);
    sScene.lightColorsGrid[4] = glm::vec3(120.0f, 110.0f, 100.0f);

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
    std::cout << "  - I: Toggle IBL on/off" << std::endl;
    std::cout << "  - Mouse drag to rotate camera" << std::endl;
    std::cout << "  - Mouse wheel to zoom" << std::endl;
    std::cout << "  - W/A/S/D/Space/Shift: Move camera" << std::endl;
    std::cout << "  - Q/E: Zoom in/out (planet scene)" << std::endl;
    std::cout << "  - R: Toggle wireframe" << std::endl;
    std::cout << "  - T: Toggle textures" << std::endl;
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

        // Get current camera
        Camera &cam = (sScene.currentScene == SCENE_GRID) ? sScene.cameraGrid : sScene.cameraPlanet;
        glm::mat4 proj = cameraProjection(cam);
        glm::mat4 view = cameraView(cam);
        glm::vec3 camPos = cam.position;

        // RENDER SKYBOX
        if (sScene.useIBL) {
            glDepthMask(GL_FALSE);

            glUseProgram(sScene.shaderSkybox.id);

            // Remove translation from view matrix (keep only rotation)
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

            shaderUniform(sScene.shaderSkybox, "uView", skyboxView);
            shaderUniform(sScene.shaderSkybox, "uProjection", proj);
            shaderUniform(sScene.shaderSkybox, "uExposure", sScene.exposure);
            shaderUniform(sScene.shaderSkybox, "uSkybox", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.envCubemap);

            glBindVertexArray(sScene.skyboxMesh.vao);
            glDrawElements(GL_TRIANGLES, sScene.skyboxMesh.size_ibo, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
        }

        glUseProgram(sScene.shaderPBR.id);

        // Common shader uniforms
        shaderUniform(sScene.shaderPBR, "uExposure", sScene.exposure);
        shaderUniform(sScene.shaderPBR, "uUseTextures", (int)sScene.useTextures);
        shaderUniform(sScene.shaderPBR, "uUseIBL", (int)sScene.useIBL);
        shaderUniform(sScene.shaderPBR, "uUseVertexColors", 0);

        // Bind texture units
        shaderUniform(sScene.shaderPBR, "uARMMap", 0);
        shaderUniform(sScene.shaderPBR, "uDiffuseMap", 1);
        shaderUniform(sScene.shaderPBR, "uNormalMap", 2);

        // Bind IBL texture units
        shaderUniform(sScene.shaderPBR, "uIrradianceMap", 3);
        shaderUniform(sScene.shaderPBR, "uPrefilterMap", 4);
        shaderUniform(sScene.shaderPBR, "uBRDFLUT", 5);

        // bind IBL textures
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.irradianceMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.prefilterMap);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, sScene.brdfLUT);

        if (sScene.currentScene == SCENE_GRID) {
            /* render grid scene */
            Camera &cam = sScene.cameraGrid;
            glm::mat4 proj = cameraProjection(cam);
            glm::mat4 view = cameraView(cam);
            glm::vec3 camPos = cam.position;

            shaderUniform(sScene.shaderPBR, "uProj", proj);
            shaderUniform(sScene.shaderPBR, "uView", view);
            shaderUniform(sScene.shaderPBR, "uCamPos", camPos);
            shaderUniform(sScene.shaderPBR, "uUseVertexColors", 0);

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

                // Check if textures are enabled
                if (sScene.useTextures) {
                    const TextureSet &texSet = sScene.textureSets[sphere.textureSetIndex];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texSet.arm);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texSet.diffuse);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texSet.normal);

                    shaderUniform(sScene.shaderPBR, "uUseTextures", 1);

                } else {
                    shaderUniform(sScene.shaderPBR, "uAlbedo", sphere.albedo);
                    shaderUniform(sScene.shaderPBR, "uMetallic", sphere.metallic);
                    shaderUniform(sScene.shaderPBR, "uRoughness", sphere.roughness);
                    shaderUniform(sScene.shaderPBR, "uAO", 1.0f);
                    shaderUniform(sScene.shaderPBR, "uUseTextures", 0);
                }

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
            shaderUniform(sScene.shaderPBR, "uUseVertexColors", 1);
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
    shaderDelete(sScene.shaderSkybox);
    meshDelete(sScene.sphereMesh);
    meshDelete(sScene.skyboxMesh);

    // Cleanup IBL textures
    glDeleteTextures(1, &sScene.envCubemap);
    glDeleteTextures(1, &sScene.irradianceMap);
    glDeleteTextures(1, &sScene.prefilterMap);
    glDeleteTextures(1, &sScene.brdfLUT);

    windowDelete(window);

    return EXIT_SUCCESS;
}