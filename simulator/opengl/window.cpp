#include "window.h"
#include <iostream>

namespace OpenGL {

Window::Window(int width, int height, const std::string& title)
    : window_(nullptr), width_(width), height_(height), title_(title) {}

Window::~Window() {
    shutdown();
}

bool Window::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Set OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Setup viewport
    glViewport(0, 0, width_, height_);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    std::cout << "OpenGL Context initialized successfully" << std::endl;
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Version: " << glGetString(GL_VERSION) << std::endl;

    return true;
}

void Window::shutdown() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

void Window::swapBuffers() {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::setCursorPosCallback(GLFWcursorposfun callback) {
    if (window_) {
        glfwSetCursorPosCallback(window_, callback);
    }
}

void Window::setKeyCallback(GLFWkeyfun callback) {
    if (window_) {
        glfwSetKeyCallback(window_, callback);
    }
}

void Window::setCursorMode(int mode) {
    if (window_) {
        glfwSetInputMode(window_, GLFW_CURSOR, mode);
    }
}

} // namespace OpenGL
