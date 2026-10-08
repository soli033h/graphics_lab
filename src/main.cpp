#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace 
{
    std::string read_file(const std::string& path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("Failed to open shader file: " + path);
        }

        std::stringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

    GLuint compile_shader(GLenum type, const std::string& source, const char* name)
    {
        const GLuint shader = glCreateShader(type);
        const char* source_text = source.c_str();
        glShaderSource(shader, 1, &source_text, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE)
        {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            std::string log(static_cast<std::size_t>(log_length), '\0');
            glGetShaderInfoLog(shader, log_length, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error(std::string("Failed to compile ") + name + ":\n" + log);
        }

        return shader;
    }

    GLuint create_shader_program(const std::string& vertex_path, const std::string& fragment_path)
    {
        const std::string vertex_source = read_file(vertex_path);
        const std::string fragment_source = read_file(fragment_path);
        const GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source, "vertex shader");
        const GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source, "fragment shader");
        const GLuint program = glCreateProgram();

        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        if (success != GL_TRUE)
        {
            GLint log_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
            std::string log(static_cast<std::size_t>(log_length), '\0');
            glGetProgramInfoLog(program, log_length, nullptr, log.data());
            glDeleteProgram(program);
            throw std::runtime_error("Failed to link shader program:\n" + log);
        }

        return program;
    }

    void framebuffer_size_callback(GLFWwindow*, int width, int height)
    {
        glViewport(0, 0, width, height);
    }
} 

int main()
{
    glfwSetErrorCallback([](int error_code, const char* description) { std::cerr << "GLFW error (" << error_code << "): " << description << '\n';});

    if (!glfwInit()) return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
    
    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLuint shader_program = 0;
    GLuint vertex_array = 0;
    GLuint vertex_buffer = 0;

    try
    {
        shader_program = create_shader_program(
            std::string(GRAPHICS_LAB_SHADER_DIR) + "/basic.vert",
            std::string(GRAPHICS_LAB_SHADER_DIR) + "/basic.frag");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    const float vertices[] = {
         0.0f,  0.6f,  1.0f, 0.0f, 0.0f, // position, red
        -0.6f, -0.6f, 0.0f, 1.0f, 0.0f, // position, green
         0.6f, -0.6f, 0.0f, 0.0f, 1.0f  // position, blue
    };

    glGenVertexArrays(1, &vertex_array);
    glGenBuffers(1, &vertex_buffer);
    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    constexpr GLsizei stride = 5 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vertex_array);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vertex_array);
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteProgram(shader_program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
