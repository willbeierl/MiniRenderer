#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include "gfx/ShaderProgram.h"
#include "gfx/Buffer.h"
#include "gfx/Texture2D.h"
#include "gfx/VertexArray.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

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

    glEnable(GL_DEPTH_TEST);

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

    Texture2D tex(std::string(ASSETS_DIR) + "/textures/checker.png");
    if (tex.Id() == 0)
    {
        std::cerr << "Failed to load texture.\n";
    }


    GLint uMVP = glGetUniformLocation(program.Id(), "uMVP");
    GLint uTime = glGetUniformLocation(program.Id(), "uTime");
    GLint uPulse = glGetUniformLocation(program.Id(), "uPulse");
    GLint uTex0 = glGetUniformLocation(program.Id(), "uTex0");

    auto WarnIfMissing = [](GLint loc, const char* name)
        {
            if (loc == -1)
                std::cerr << "Warning: " << name << " uniform not found (maybe optimized out).\n";
        };
    WarnIfMissing(uMVP, "uMVP");
    WarnIfMissing(uTime, "uTime");
    WarnIfMissing(uPulse, "uPulse");
    WarnIfMissing(uTex0, "uTex0");

    float vertices[] = {
        // x,     y,     z,      r, g, b,      u, v
        -0.5f, -0.5f, 0.0f,     1, 0, 0,      0, 0,
         0.5f, -0.5f, 0.0f,     0, 1, 0,      1, 0,
         0.5f,  0.5f, 0.0f,     0, 0, 1,      1, 1,
        -0.5f,  0.5f, 0.0f,     1, 1, 0,      0, 1
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
    vao.SetAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    vao.SetAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 3 * sizeof(float));
    vao.SetAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 6 * sizeof(float));

    // unbind to avoid accidental edits later
    Buffer::Unbind(GL_ARRAY_BUFFER);
    VertexArray::Unbind();

    bool wasRDown = false; // reload shader
    bool wasTDown = false; // wirefreame
    bool wasLDown = false; // L = reload texture
    bool wasFDown = false; // texture filtering (NEAREST vs LINEAR)

    bool wireframe = false;
    bool nearest = false;

    float pulseValue = 1.0f;

    // Basic render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        bool isFDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (isFDown && !wasFDown)
        {
            nearest = !nearest;

            if (nearest)
            {
                // crisp pixels
                tex.SetFiltering(GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
                std::cout << "[Tex] Filtering: NEAREST\n";
            }
            else
            {
                // smooth sampling
                tex.SetFiltering(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                std::cout << "[Tex] Filtering: LINEAR\n";
            }
        }
        wasFDown = isFDown;

        bool isLDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        if (isLDown && !wasLDown)
        {
            if (!tex.LoadFromFile(std::string(ASSETS_DIR) + "/textures/checker.png"))
                std::cerr << "Texture reload failed.\n";
        }
        wasLDown = isLDown;

        bool isRDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (isRDown && !wasRDown)
        {
            if (program.Reload())
                uMVP = glGetUniformLocation(program.Id(), "uMVP");
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        float aspect = (h == 0) ? 1.0f : (static_cast<float>(w) / static_cast<float>(h));

        float t = (float)glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)   pulseValue += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) pulseValue -= 0.01f;
        pulseValue = std::clamp(pulseValue, 0.0f, 100.0f);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0, 1, 0)); // rotate around Y
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 3.0f);

        //glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 2.0f);
        glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 0.8f);
        glm::vec3 camTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(camPos, camTarget, camUp);

        glm::mat4 mvp = proj * view * model;

        program.Use();

        tex.Bind(0);

        if (uTex0 != -1) 
            glUniform1i(uTex0, 0);
        if (uMVP != -1)
            glUniformMatrix4fv(uMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        if (uTime != -1)
            glUniform1f(uTime, t);
        if (uPulse != -1)
            glUniform1f(uPulse, pulseValue);

        vao.Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
