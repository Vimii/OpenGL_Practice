#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Shader.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Texture.h"
#include "Debug.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#define WINDOW_WIDTH 1280.0f
#define WINDOW_HEIGHT 720.0f

float FOV = 45.0f;
glm::vec3 Cam_Pos(0.0f, 0.0f, 3.0f);
glm::vec3 Cam_Target(0.0f, 0.0f, 0.0f);
glm::vec3 Cam_Direction = glm::normalize(Cam_Pos - Cam_Target);
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 Cam_Right = glm::normalize(glm::cross(up, Cam_Direction));
glm::vec3 Cam_Up = glm::vec3(0.0f,1.0f,0.0f);
const float radius = 10.0f;
glm::vec3 Cam_Front(0.0f, 0.0f, -1.0f);
glm::vec3 direction;
float yaw = 0.0f, pitch = 0.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);


//timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello OpenGL", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /*frameSpeed*/
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
        std::cout << "Error!" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    void mouse_callback(GLFWwindow*, double xpos, double ypos);
    glfwSetCursorPosCallback(window, mouse_callback);

    {
        /*Plane*/
        float positions[] = {
             -50.0f, -50.0f, 0.0f, 0.0f, // 0
             50.0f, -50.0f, 1.0f, 0.0f, // 1
             50.0f, 50.0f, 1.0f, 1.0f, // 2
             -50.0f, 50.0f, 0.0f, 1.0f  // 3
        };

        /*Box*/
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        };

        /*index_datar*/
        unsigned int indices[] = {
            0,1,2,
            2,3,0
        };

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        /*make VertexArray*/
        VertexArray va;
        VertexBuffer vb(positions, sizeof(positions));

        /*make VertexBufferLayout*/
        VertexBufferLayout layout;
        layout.Push<float>(2); //Plane_position
        //layout.Push<float>(3); //Box_position
        layout.Push<float>(2); //uv
        va.addBuffer(vb, layout);

        /*make IndexBuffer*/
        IndexBuffer ib(indices, sizeof(indices));


        /*make Shader*/
        Shader shader("res/shaders/practice1.shader");
        shader.Bind();

        Texture texture("res/textures/icon.png");
        texture.Bind(0);
        shader.SetUniform1i("u_Texture", 0);

        /*UnBind all before main loop*/
        va.UnBind();
        vb.Unbind();
        ib.Unbind();
        shader.UnBind();

        /*make Renderer*/
        Renderer renderer;

        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, true);
        ImGui::StyleColorsDark();

        /*util valiables*/        

        float r = 0.0f;
        float increment = 0.05f;
        glm::vec3 translationA(200, 200, 0);
        glm::vec3 translationB(400, 200, 0);

        

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            processInput(window);

            /* Render here */
            renderer.Clear();

            ImGui_ImplGlfwGL3_NewFrame();

            /*Draw Object*/
            {
                glm::mat4 proj = glm::perspective(glm::radians(FOV), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);
                float camX = sin(glfwGetTime()) * radius;
                float camZ = cos(glfwGetTime()) * radius;
                glm::mat4 view = glm::lookAt(Cam_Pos, Cam_Pos + Cam_Front, Cam_Up);
                {
                    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(-55.0f), translationA);
                    glm::mat4 mvp = proj * view * model;
                    shader.SetUniformMat4f("u_MVP", mvp);
                    shader.Bind();
                    renderer.Draw(va, ib, shader);
                }
                {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), translationB);
                    glm::mat4 mvp = proj * view * model;
                    shader.SetUniformMat4f("u_MVP", mvp);
                    shader.Bind();
                    renderer.Draw(va, ib, shader);
                }
            }

            /*Update valiables*/
            if (r > 1.0f)
                increment = -0.05f;
            else if (r < 0.0f)
                increment = 0.05f;

            r += increment;

            {
                ImGui::Text("Camera");
                ImGui::SliderFloat("FOV", &FOV, 0.0f,90.0f);
                ImGui::SliderFloat3("Translation A", &translationA.x, 0.0f, WINDOW_WIDTH);
                ImGui::SliderFloat3("Translation B", &translationB.x, 0.0f, WINDOW_WIDTH);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }

            ImGui::Render();
            ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }
    }
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}



void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    Cam_Front = glm::normalize(direction);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    float cameraSpeed; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed = 100.0f * deltaTime;
    else
        cameraSpeed = 10.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Cam_Pos += cameraSpeed * Cam_Front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Cam_Pos -= cameraSpeed * Cam_Front;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        Cam_Pos += cameraSpeed * Cam_Up;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        Cam_Pos -= cameraSpeed * Cam_Up;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Cam_Pos -= glm::normalize(glm::cross(Cam_Front, Cam_Up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Cam_Pos += glm::normalize(glm::cross(Cam_Front, Cam_Up)) * cameraSpeed;

}