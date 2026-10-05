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
    std::string read_file(const char* path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("Failed to open shader file: " + std::string(path));
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

    GLuint create_shader(GLenum type, const std::string& source, const char* name)
    {
        const GLuint shader = glCreateShader(type);
        const char* source_data = source.c_str();
        glShaderSource(shader, 1, &source_data, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512]{};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            glDeleteShader(shader);
            throw std::runtime_error(
                "Failed to compile " + std::string(name) + ":\n" + log);
        }

        return shader;
    }

    GLuint create_shader_program(const char* vertex_path, const char* fragment_path)
    {
        const GLuint vertex_shader =
            create_shader(GL_VERTEX_SHADER, read_file(vertex_path), "vertex shader");
        const GLuint fragment_shader =
            create_shader(GL_FRAGMENT_SHADER, read_file(fragment_path), "fragment shader");
        const GLuint program = glCreateProgram();

        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        if (!success)
        {
            char log[512]{};
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            glDeleteProgram(program);
            throw std::runtime_error("Failed to link shader program:\n" + std::string(log));
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
    glfwSetErrorCallback([](int error_code, const char* description)
    {
        std::cerr << "GLFW error (" << error_code << "): " << description << '\n';
    });

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

    try
    {
        const GLuint shader_program = create_shader_program(
            GRAPHICS_LAB_SHADER_DIR "/basic.vert",
            GRAPHICS_LAB_SHADER_DIR "/basic.frag");

        const float vertices[] =
        {
             0.0f,  0.6f,
            -0.6f, -0.6f,
             0.6f, -0.6f
        };

        GLuint vao = 0;
        GLuint vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        while (!glfwWindowShouldClose(window))
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shader_program);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteProgram(shader_program);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
