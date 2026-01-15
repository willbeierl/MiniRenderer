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
#include "gfx/Primitives.h"
#include <vector>

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

struct Transform
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotationEuler{ 0.0f }; // radians: x,y,z
    glm::vec3 scale{ 1.0f };
};

static glm::mat4 MakeModelMatrix(const Transform& t)
{
    glm::mat4 M(1.0f);
    M = glm::translate(M, t.position);
    M = glm::rotate(M, t.rotationEuler.x, glm::vec3(1, 0, 0));
    M = glm::rotate(M, t.rotationEuler.y, glm::vec3(0, 1, 0));
    M = glm::rotate(M, t.rotationEuler.z, glm::vec3(0, 0, 1));
    M = glm::scale(M, t.scale);
    return M;
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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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



    const unsigned int SHADOW_SIZE = 1024;   // try 1024 first, later 2048
    const float SHADOW_NEAR = 0.1f;
    const float SHADOW_FAR = 50.0f;         // must be >= your scene extents

    GLuint shadowFBO = 0;
    GLuint shadowCube = 0;

    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowCube);

    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCube);
    for (int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            GL_DEPTH_COMPONENT24, SHADOW_SIZE, SHADOW_SIZE, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCube, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Shadow cubemap FBO incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    float yaw = -90.0f;
    float pitch = 0.0f;
    bool firstMouse = true;
    double lastX = 0.0, lastY = 0.0;

    glm::vec3 camPos(0.0f, 0.0f, 3.0f);
    glm::vec3 camFront(0.0f, 0.0f, -1.0f);
    glm::vec3 camUp(0.0f, 1.0f, 0.0f);

    float lastTime = (float)glfwGetTime();

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    ShaderProgram program(
        std::string(ASSETS_DIR) + "/shaders/lit.vert",
        std::string(ASSETS_DIR) + "/shaders/lit.frag"
    );
    if (program.Id() == 0)
    {
        std::cerr << "Failed to create shader program.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }


    ShaderProgram shadowProg(
        std::string(ASSETS_DIR) + "/shaders/shadow_cube.vert",
        std::string(ASSETS_DIR) + "/shaders/shadow_cube.frag"
    );
    if (shadowProg.Id() == 0) 
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


    Mesh cube = CreateCube();

    struct RenderItem
    {
        Transform transform;
        Mesh* mesh = nullptr;
    };
    std::vector<RenderItem> scene;

    // Cube 1
    scene.push_back({ Transform{ glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(1,1,1) }, &cube });
    // Cube 2 (offset)
    scene.push_back({ Transform{ glm::vec3(2,0,0), glm::vec3(0,0,0), glm::vec3(1,1,1) }, &cube });
    // “Floor” (just a scaled cube)
    scene.push_back({ Transform{ glm::vec3(0,-1.0f,0), glm::vec3(0,0,0), glm::vec3(10.0f, 0.1f, 10.0f) }, &cube });



    GLint uModel = glGetUniformLocation(program.Id(), "uModel");
    GLint uView = glGetUniformLocation(program.Id(), "uView");
    GLint uProj = glGetUniformLocation(program.Id(), "uProj");
    GLint uTex0 = glGetUniformLocation(program.Id(), "uTex0");   
    GLint uCameraPosWS = glGetUniformLocation(program.Id(), "uCameraPosWS");
    GLint uUseTexture = glGetUniformLocation(program.Id(), "uUseTexture");
    GLint uLightPosWS = glGetUniformLocation(program.Id(), "uLightPosWS");
    GLint uLightColor = glGetUniformLocation(program.Id(), "uLightColor");
    GLint uIsLight = glGetUniformLocation(program.Id(), "uIsLight");
    GLint uShadowCube = glGetUniformLocation(program.Id(), "uShadowCube");
    GLint uFarPlane = glGetUniformLocation(program.Id(), "uFarPlane");

    GLint sh_uModel = glGetUniformLocation(shadowProg.Id(), "uModel");
    GLint sh_uLightVP = glGetUniformLocation(shadowProg.Id(), "uLightVP");
    GLint sh_uLightPos = glGetUniformLocation(shadowProg.Id(), "uLightPosWS");
    GLint sh_uFarPlane = glGetUniformLocation(shadowProg.Id(), "uFarPlane");


    auto WarnIfMissing = [](GLint loc, const char* name)
        {
            if (loc == -1)
                std::cerr << "Warning: " << name << " uniform not found (maybe optimized out).\n";
        };  
    WarnIfMissing(uTex0, "uTex0");
    WarnIfMissing(uCameraPosWS, "uCameraPosWS");
    WarnIfMissing(uModel, "uModel");
    WarnIfMissing(uView, "uView");
    WarnIfMissing(uProj, "uProj");
    WarnIfMissing(uUseTexture, "uUseTexture");
    WarnIfMissing(uLightPosWS, "uLightPosWS");
    WarnIfMissing(uLightColor, "uLightColor");
    WarnIfMissing(uShadowCube, "uShadowCube");
    WarnIfMissing(uFarPlane, "uFarPlane");

    WarnIfMissing(sh_uModel, "sh_uModel");
    WarnIfMissing(sh_uLightVP, "sh_uLightVP");
    WarnIfMissing(sh_uLightPos, "sh_uLightPos");
    WarnIfMissing(sh_uFarPlane, "sh_uFarPlane");




    
    glm::vec3 lightPos(1.5f, 1.5f, 1.5f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    

    bool wasRDown = false; // reload shader
    bool wasTDown = false; // wirefreame
    bool wasLDown = false; // reload texture
    bool wasFDown = false; // texture filtering (NEAREST vs LINEAR)

    bool wasKDown = false; // Anisotropic filtering
    bool anisoOn = false;

    bool wasUDown = false;
    int useTexture = 1;

    bool wireframe = false;
    bool nearest = false;
   
    // Basic render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();



        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        double x, y;
        glfwGetCursorPos(window, &x, &y);

        if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }

        float xoffset = (float)(x - lastX);
        float yoffset = (float)(lastY - y);
        lastX = x; lastY = y;

        float sensitivity = 0.08f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        camFront = glm::normalize(front);

        float speed = 3.0f * dt;
        glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;





        bool isUDown = glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS;
        if (isUDown && !wasUDown)
        {
            useTexture = 1 - useTexture;
            std::cout << "[Mat] UseTexture: " << (useTexture ? "ON" : "OFF") << "\n";
        }
        wasUDown = isUDown;

        bool isKDown = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
        if (isKDown && !wasKDown)
        {
            anisoOn = !anisoOn;
            tex.SetAnisotropy(anisoOn ? 16.0f : 1.0f);
            std::cout << "[Tex] Aniso: " << (anisoOn ? "ON" : "OFF") << "\n";
        }
        wasKDown = isKDown;

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
            {                             
                uModel = glGetUniformLocation(program.Id(), "uModel");
                uView = glGetUniformLocation(program.Id(), "uView");
                uProj = glGetUniformLocation(program.Id(), "uProj");               
                uTex0 = glGetUniformLocation(program.Id(), "uTex0");
                uCameraPosWS = glGetUniformLocation(program.Id(), "uCameraPosWS");
                uUseTexture = glGetUniformLocation(program.Id(), "uUseTexture");
                uLightPosWS = glGetUniformLocation(program.Id(), "uLightPosWS");
                uLightColor = glGetUniformLocation(program.Id(), "uLightColor");
                uIsLight = glGetUniformLocation(program.Id(), "uIsLight");
                uShadowCube = glGetUniformLocation(program.Id(), "uShadowCube");
                uFarPlane = glGetUniformLocation(program.Id(), "uFarPlane");

                sh_uModel = glGetUniformLocation(shadowProg.Id(), "uModel");
                sh_uLightVP = glGetUniformLocation(shadowProg.Id(), "uLightVP");
                sh_uLightPos = glGetUniformLocation(shadowProg.Id(), "uLightPosWS");
                sh_uFarPlane = glGetUniformLocation(shadowProg.Id(), "uFarPlane");
                             
                WarnIfMissing(uTex0, "uTex0");
                WarnIfMissing(uModel, "uModel");
                WarnIfMissing(uView, "uView");
                WarnIfMissing(uProj, "uProj");
                WarnIfMissing(uCameraPosWS, "uCameraPosWS"); 
                WarnIfMissing(uUseTexture, "uUseTexture");
                WarnIfMissing(uLightPosWS, "uLightPosWS");
                WarnIfMissing(uLightColor, "uLightColor");
                WarnIfMissing(uIsLight, "uIsLight");
                WarnIfMissing(uShadowCube, "uShadowCube");
                WarnIfMissing(uFarPlane, "uFarPlane");

                WarnIfMissing(sh_uModel, "sh_uModel");
                WarnIfMissing(sh_uLightVP, "sh_uLightVP");
                WarnIfMissing(sh_uLightPos, "sh_uLightPos");
                WarnIfMissing(sh_uFarPlane, "sh_uFarPlane");
            }
        }
        wasRDown = isRDown;

        bool isTDown = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        if (isTDown && !wasTDown)
        {
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }
        wasTDown = isTDown;

        
        float lightSpeed = 3.0f * dt;


        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) lightPos.x -= lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) lightPos.x += lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) lightPos.z -= lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) lightPos.z += lightSpeed;

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) lightPos.y += lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) lightPos.y -= lightSpeed;



        glClearColor(0.01f, 0.15f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        float aspect = (h == 0) ? 1.0f : (static_cast<float>(w) / static_cast<float>(h));
                       
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);





        glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, SHADOW_NEAR, SHADOW_FAR);
        glm::mat4 shadowVP[6] = {
            lightProj * glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
            lightProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
            lightProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
            lightProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
            lightProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
            lightProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
        };

        glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Optional: reduce acne
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        shadowProg.Use();
        if (sh_uLightPos != -1) glUniform3fv(sh_uLightPos, 1, glm::value_ptr(lightPos));
        if (sh_uFarPlane != -1) glUniform1f(sh_uFarPlane, SHADOW_FAR);

        for (int face = 0; face < 6; face++)
        {
            if (sh_uLightVP != -1)
                glUniformMatrix4fv(sh_uLightVP, 1, GL_FALSE, glm::value_ptr(shadowVP[face]));

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, shadowCube, 0);
            glClear(GL_DEPTH_BUFFER_BIT);

            for (auto& item : scene)
            {
                // IMPORTANT: do NOT render the light gizmo cube into the shadow map
                // (just skip that separate draw you do for the gizmo)
                glm::mat4 model = MakeModelMatrix(item.transform);
                if (sh_uModel != -1)
                    glUniformMatrix4fv(sh_uModel, 1, GL_FALSE, glm::value_ptr(model));

                item.mesh->Draw();
            }
        }
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

            


        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);    
       
        program.Use();
        tex.Bind(0);


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCube);

        if (uShadowCube != -1) 
            glUniform1i(uShadowCube, 1);
        if (uFarPlane != -1) 
            glUniform1f(uFarPlane, SHADOW_FAR);



        if (uTex0 != -1) 
            glUniform1i(uTex0, 0);       
        if (uView != -1)
            glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
        if (uProj != -1)
            glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));            
        if (uUseTexture != -1)
            glUniform1i(uUseTexture, useTexture);

        if (uLightPosWS != -1) 
            glUniform3fv(uLightPosWS, 1, glm::value_ptr(lightPos));
        if (uLightColor != -1) 
            glUniform3fv(uLightColor, 1, glm::value_ptr(lightColor));
        if (uCameraPosWS != -1) 
            glUniform3fv(uCameraPosWS, 1, glm::value_ptr(camPos));


        scene[1].transform.rotationEuler.y = (float)glfwGetTime(); // rotate cube 2
        if (uIsLight != -1) glUniform1i(uIsLight, 0);

        for (auto& item : scene)
        {
            glm::mat4 model = MakeModelMatrix(item.transform);

            if (uModel != -1)
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));

            item.mesh->Draw();
        }


        glm::mat4 lightModel(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.12f)); // small cube

        if (uModel != -1) 
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(lightModel));
        if (uIsLight != -1) 
            glUniform1i(uIsLight, 1);

        cube.Draw();

        if (uIsLight != -1) glUniform1i(uIsLight, 0);


        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
