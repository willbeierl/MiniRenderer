#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include "gfx/ShaderProgram.h"
#include "gfx/Buffer.h"
#include "gfx/VertexArray.h"

static void glfwErrorCallback(int code, const char* description)
{
    std::cerr << "GLFW Error (" << code << "): " << description << "\n";
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Compiles a vertex or fragment shader from source
// Returns shader ID or 0 on failure
GLuint CompileShader(GLenum type, const char* source) 
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {             
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);

        const char* shaderType =
            (type == GL_VERTEX_SHADER) ? "VERTEX" :
            (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" : "UNKNOWN";

        std::cerr << shaderType << " shader compile error:\n" << infoLog << "\n";

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Links a shader program from vertex + fragment sources
// Returns program ID or 0 on failure
GLuint CreateProgram(const char* vsSource, const char* fsSource)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    if (vs == 0) return 0;

    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);
    if (fs == 0)
    {
        glDeleteShader(vs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    // delete shaders after linking; program keeps what it needs
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Program link error:\n" << infoLog << "\n";

        glDeleteProgram(program);
        return 0;
    }

    return program;
}

std::string LoadTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << path << "\n";
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW\n";
        return 1;
    }

    // Request modern OpenGL core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MiniRenderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    ShaderProgram program(
        std::string(ASSETS_DIR) + "/shaders/basic.vert",
        std::string(ASSETS_DIR) + "/shaders/basic.frag"
    );

    if (program.Id() == 0)
    {
        std::cerr << "Failed to create shader program.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    float vertices[] = {
        // x,     y,     z
        -0.5f, -0.5f, 0.0f,  // 0 bottom-left
         0.5f, -0.5f, 0.0f,  // 1 bottom-right
         0.5f,  0.5f, 0.0f,  // 2 top-right
        -0.5f,  0.5f, 0.0f   // 3 top-left
    };

    unsigned int indices[] = {
    0, 1, 2,   // first triangle
    2, 3, 0    // second triangle
    };

    VertexArray vao;
    Buffer vbo(GL_ARRAY_BUFFER);
    Buffer ebo(GL_ELEMENT_ARRAY_BUFFER);

    vao.Bind();

    // VBO data
    vbo.Bind();
    vbo.SetData(vertices, sizeof(vertices), GL_STATIC_DRAW);

    // EBO data 
    ebo.Bind();
    ebo.SetData(indices, sizeof(indices), GL_STATIC_DRAW);

    // vertex layout
    vao.SetAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    // unbind to avoid accidental edits later
    Buffer::Unbind(GL_ARRAY_BUFFER);
    VertexArray::Unbind();

    bool wasRDown = false;
    bool wasTDown = false;
    bool wireframe = false;

    // Basic render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        bool isRDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (isRDown && !wasRDown)
        {
            program.Reload();
        }
        wasRDown = isRDown;

        bool isTDown = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        if (isTDown && !wasTDown)
        {
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }
        wasTDown = isTDown;

        glClearColor(0.01f, 0.15f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        program.Use();
        vao.Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
