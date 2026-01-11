#include <cstdlib>
#include <iostream>

#include "mygl/shader.h"
#include "mygl/mesh.h"
#include "mygl/geometry.h"
#include "mygl/camera.h"

#include <glm/gtc/matrix_transform.hpp>

struct
{
    Camera camera;
    Mesh cubeMesh;

    ShaderProgram shaderColor;
} sScene;

struct
{
    bool mouseButtonPressed = false;
    glm::vec2 mousePressStart;
} sInput;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    /* called on keyboard event */

    /* close window on escape */
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        screenshotToPNG("screenshot.png");
    }
}

void mouse_pos_callback(GLFWwindow* window, double x, double y)
{
    /* called on cursor position change */
    if(sInput.mouseButtonPressed)
    {
        glm::vec2 diff = sInput.mousePressStart - glm::vec2(x, y);
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = glm::vec2(x, y);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    /* called on mouse button event */
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        sInput.mouseButtonPressed = (action == GLFW_PRESS);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = glm::vec2(x, y);
    }
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraUpdateOrbit(sScene.camera, {0, 0}, 0.1 * yoffset);
}

void window_resize_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    sScene.camera.width = width;
    sScene.camera.height = height;
}

int main(int argc, char** argv)
{
    /* create window/context */
    GLFWwindow* window = windowCreate("GLM Demo", 1280, 720);

    /* set window callbacks */
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(window, window_resize_callback);


    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* create opengl buffers for mesh */
    sScene.cubeMesh = meshCreate(cube::vertices, cube::indices);

    /* load shader from file */
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/color.frag");

    /* create camera */
    sScene.camera = cameraCreate(1280, 720, glm::radians(45.0f), 0.1, 15.0, {0, 0, -10.0});

    /*-------------- main loop ----------------*/
    float t = 0.0f;
    /* loop until user closes window */
    while(!glfwWindowShouldClose(window))
    {
        /* poll and process input and window events */
        glfwPollEvents();
        t += 1.0/60.0f;

        /*------------ default frambuffer -------------*/
        {
            glClearColor(1.0, 1.0, 1.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            /*------------ render cube (color) -------------*/
            /* use shader and set the uniforms (names match the ones in the shader) */
            {
                /* setup camera and model matrices */
                glm::mat4 proj = cameraProjection(sScene.camera);
                glm::mat4 view = cameraView(sScene.camera);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::rotate(model, (float)M_PI/4.0f*t, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, (float)M_PI/4.0f, glm::vec3(1.0f, 0.0f, 0.0f));
                
                glUseProgram(sScene.shaderColor.id);
                shaderUniform(sScene.shaderColor, "uProj",  proj);
                shaderUniform(sScene.shaderColor, "uView",  view);
                shaderUniform(sScene.shaderColor, "uModel", model);

                /* bind vertex array object and draw its content */
                glBindVertexArray(sScene.cubeMesh.vao);
                glDrawElements(GL_TRIANGLES, sScene.cubeMesh.size_ibo, GL_UNSIGNED_INT, nullptr);
            }

            /* cleanup opengl state */
            glBindVertexArray(0);
            glUseProgram(0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }


    /*-------- cleanup --------*/
    /* delete opengl shader and buffers */
    shaderDelete(sScene.shaderColor);
    meshDelete(sScene.cubeMesh);

    /* destroy window/context */
    windowDelete(window);

    return EXIT_SUCCESS;
}
