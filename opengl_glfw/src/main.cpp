#include <glad/glad.h>

// GLFW must be preceeded by GLAD or alikes
#include <GLFW/glfw3.h>

#include <iostream>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Could not load GLTF" << std::endl;
        return 1;
    }

    const auto WINDOW_WIDTH = 1024;
    const auto WINDOW_HEIGHT = 768;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL with GLFW", NULL, NULL);

    if (!window) {
        std::cerr << "Could not create GLFW window" << std::endl;
        return 1;
    }

    glfwMakeContextCurrent(window);

    // gladLoadGL(glfwGetProcAddress); ?
    int version = gladLoadGL();

    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        return 1;
    }

    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);

    bool isRunning = true;

    while (isRunning) {
        if (glfwWindowShouldClose(window)) {
            isRunning = false;
        }

        double time = glfwGetTime();

        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
