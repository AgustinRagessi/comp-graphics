#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace OpenGL {

/**
 * @brief GLFW window wrapper with OpenGL context management
 */
class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    // No copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool initialize();
    void shutdown();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getHandle() const { return window_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // Callback setters
    void setCursorPosCallback(GLFWcursorposfun callback);
    void setKeyCallback(GLFWkeyfun callback);
    void setCursorMode(int mode);

private:
    GLFWwindow* window_;
    int width_;
    int height_;
    std::string title_;
};

} // namespace OpenGL
