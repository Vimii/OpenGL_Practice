#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Shader.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"
#include "VertexArray.h"

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello OpenGL", NULL, NULL);
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
    {
        /*vertex_data*/
        float positions[] = {
            -0.5f,-0.5f,
            0.5f, -0.5f,
            0.5f, 0.5f,
            -0.5f, 0.5f
        };

        /*index_datar*/
        unsigned int indices[] = {
            0,1,2,
            2,3,0
        };

        /*make VertexArray*/
        VertexArray va;
        VertexBuffer vb(positions, sizeof(positions));

        /*make VertexBufferLayout*/
        VertexBufferLayout layout;
        layout.Push<float>(2);
        va.addBuffer(vb, layout);

        /*make IndexBuffer*/
        IndexBuffer ib(indices, sizeof(indices));

        /*make Shader*/
        Shader shader("practice1.shader");
        shader.Bind();
        shader.SetUniform4f("u_Color", 0.2f, 0.3f, 0.8f, 1.0f);

        /*UnBind all before main loop*/
        va.UnBind();
        vb.Unbind();
        ib.Unbind();
        shader.UnBind();

        /*make Renderer*/
        Renderer renderer;
        
        /*util valiables*/
        float r = 0.0f;
        float increment = 0.05f;

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            renderer.Clear();

            /*Draw Object*/
            {
                /*Shader update*/
                shader.Bind();
                shader.SetUniform4f("u_Color", r, 0.3f, 0.8f, 1.0f);

                /*Draw*/
                renderer.Draw(va, ib, shader);
            }

            /*Update valiables*/
            if (r > 1.0f)
                increment = -0.05f;
            else if (r < 0.0f)
                increment = 0.05f;

            r += increment;

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }
    }
    glfwTerminate();
    return 0;
}