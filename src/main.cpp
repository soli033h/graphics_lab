#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    glfwSetErrorCallback([](int error_code, const char* description)
    {
        std::cerr << "GLFW error (" << error_code << "): "
                  << description << '\n';
    });

    if (!glfwInit())
        return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);

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

    glEnable(GL_DEPTH_TEST);

    GLuint shader_program = 0;
    GLuint vertex_array = 0;
    GLuint vertex_buffer = 0;

    GLint model_location = -1;
    GLint view_location = -1;
    GLint projection_location = -1;

    try
    {
        shader_program = create_shader_program(
            std::string(GRAPHICS_LAB_SHADER_DIR) + "/shader.vert",
            std::string(GRAPHICS_LAB_SHADER_DIR) + "/shader.frag");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    model_location = glGetUniformLocation(shader_program, "model");

    view_location = glGetUniformLocation(shader_program, "view");

    projection_location = glGetUniformLocation(shader_program, "projection");

    const float vertices[] = {
        // Front face
        -0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,

        // Back face
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Left face
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,

        // Right face
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,

        // Top face
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        // Bottom face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f
    };
    
    glGenVertexArrays(1, &vertex_array);
    glGenBuffers(1, &vertex_buffer);

    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        nullptr);
    
    glEnableVertexAttribArray(0);

    int framebuffer_width = 0;
    int framebuffer_height = 0;

    glfwGetFramebufferSize(
        window,
        &framebuffer_width,
        &framebuffer_height);

    const glm::vec3 camera_position(0.0f, 0.0f, 3.0f);
    const glm::vec3 camera_target(0.0f, 0.0f, 0.0f);
    const glm::vec3 camera_up(0.0f, 1.0f, 0.0f);

    const glm::mat4 view = glm::lookAt(
        camera_position,
        camera_target,
        camera_up);

    const float aspect_ratio = static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height);

    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        aspect_ratio,
        0.1f,
        100.0f);

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader_program);

        const float time = static_cast<float>(glfwGetTime());

        glm::mat4 model(1.0f);

        model = glm::rotate(model, time, glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(
            model_location,
            1,
            GL_FALSE,
            glm::value_ptr(model));

        glUniformMatrix4fv(
            view_location,
            1,
            GL_FALSE,
            glm::value_ptr(view));

        glUniformMatrix4fv(
            projection_location,
            1,
            GL_FALSE,
            glm::value_ptr(projection));

        glBindVertexArray(vertex_array);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
