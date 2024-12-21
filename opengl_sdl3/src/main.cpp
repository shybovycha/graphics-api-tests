#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <SDL3/SDL_opengl.h>
#include <iostream>

int main() {
    const auto windowWidth = 1024;
    const auto windowHeight = 768;

    std::cout << "Init SDL..." << std::endl;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Could not initialize SDL" << std::endl;
        return 1;
    }

    std::cout << "Create SDL window..." << std::endl;

    SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL;
    SDL_Window *window = SDL_CreateWindow("OpenGL with SDL3", windowWidth, windowHeight, windowFlags);

    if (!window) {
        std::cerr << "Could not create SDL window" << std::endl;
        return 1;
    }

    std::cout << "Create OpenGL context..." << std::endl;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetSwapInterval(1);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (!SDL_GL_MakeCurrent(window, context)) {
        std::cerr << "Could not activate OpenGL context" << std::endl;
        return 1;
    }

    std::cout << "Load GLAD..." << std::endl;

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        return 1;
    }

    bool isRunning = true;

    while (isRunning) {
        SDL_Event evt;

        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
        }

        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    return 0;
}