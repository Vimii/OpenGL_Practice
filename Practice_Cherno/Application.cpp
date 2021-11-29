#pragma warning(disable: 4996)

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_map>

#include "Shader.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Texture.h"
#include "Debug.h"
#include "Camera.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include "vendor/tinyobjloader/tiny_obj_loader.h"
#include "vendor/stb_image/stb_image.h"


#define WINDOW_WIDTH 1280.0f
#define WINDOW_HEIGHT 720.0f

//camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float FOV = 45.0f;
float lastX = WINDOW_WIDTH / 2.0f, lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

//timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//call back function
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
bool mouseActive = true;

std::vector<DrawObject> gDrawObjects;

static std::string GetBaseDir(const std::string& filepath) {
    if (filepath.find_last_of("/\\") != std::string::npos)
        return filepath.substr(0, filepath.find_last_of("/\\"));
    return "";
}

static bool FileExists(const std::string& abs_filename) {
    bool ret;
    FILE* fp = fopen(abs_filename.c_str(), "rb");
    if (fp) {
        ret = true;
        fclose(fp);
    }
    else {
        ret = false;
    }

    return ret;
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
    float v10[3];
    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];

    float v20[3];
    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];

    N[0] = v10[1] * v20[2] - v10[2] * v20[1];
    N[1] = v10[2] * v20[0] - v10[0] * v20[2];
    N[2] = v10[0] * v20[1] - v10[1] * v20[0];

    float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
        float len = sqrtf(len2);

        N[0] /= len;
        N[1] /= len;
        N[2] /= len;
    }
}

namespace  // Local utility functions
{
    struct vec3 {
        float v[3];
        vec3() {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
        }
    };

    void normalizeVector(vec3& v) {
        float len2 = v.v[0] * v.v[0] + v.v[1] * v.v[1] + v.v[2] * v.v[2];
        if (len2 > 0.0f) {
            float len = sqrtf(len2);

            v.v[0] /= len;
            v.v[1] /= len;
            v.v[2] /= len;
        }
    }

    // Check if `mesh_t` contains smoothing group id.
    bool hasSmoothingGroup(const tinyobj::shape_t& shape)
    {
        for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++) {
            if (shape.mesh.smoothing_group_ids[i] > 0) {
                return true;
            }
        }
        return false;
    }

    void computeSmoothingNormals(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
        std::map<int, vec3>& smoothVertexNormals) {
        smoothVertexNormals.clear();
        std::map<int, vec3>::iterator iter;

        for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
            // Get the three indexes of the face (all faces are triangular)
            tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

            // Get the three vertex indexes and coordinates
            int vi[3];      // indexes
            float v[3][3];  // coordinates

            for (int k = 0; k < 3; k++) {
                vi[0] = idx0.vertex_index;
                vi[1] = idx1.vertex_index;
                vi[2] = idx2.vertex_index;
                assert(vi[0] >= 0);
                assert(vi[1] >= 0);
                assert(vi[2] >= 0);

                v[0][k] = attrib.vertices[3 * vi[0] + k];
                v[1][k] = attrib.vertices[3 * vi[1] + k];
                v[2][k] = attrib.vertices[3 * vi[2] + k];
            }

            // Compute the normal of the face
            float normal[3];
            CalcNormal(normal, v[0], v[1], v[2]);

            // Add the normal to the three vertexes
            for (size_t i = 0; i < 3; ++i) {
                iter = smoothVertexNormals.find(vi[i]);
                if (iter != smoothVertexNormals.end()) {
                    // add
                    iter->second.v[0] += normal[0];
                    iter->second.v[1] += normal[1];
                    iter->second.v[2] += normal[2];
                }
                else {
                    smoothVertexNormals[vi[i]].v[0] = normal[0];
                    smoothVertexNormals[vi[i]].v[1] = normal[1];
                    smoothVertexNormals[vi[i]].v[2] = normal[2];
                }
            }

        }  // f

        // Normalize the normals, that is, make them unit vectors
        for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end();
            iter++) {
            normalizeVector(iter->second);
        }

    }  // computeSmoothingNormals
}  // namespace

//test
tinyobj::attrib_t attrib_Global;
std::vector<tinyobj::shape_t> shapes_Global;

static bool LoadObjAndConvert(float bmin[3], float bmax[3],
    std::vector<DrawObject>* drawObjects,
    std::vector<tinyobj::material_t>& materials,
    std::map<std::string, GLuint>& textures,
    const char* filename,
    VertexBufferLayout& layout) 
{

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;

    std::string base_dir = GetBaseDir(filename);
    if (base_dir.empty()) {
        base_dir = ".";
    }
#ifdef _WIN32
    base_dir += "\\";
#else
    base_dir += "/";
#endif

    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename,
        base_dir.c_str());
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cerr << "Failed to load " << filename << std::endl;
        return false;
    }

    printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
    printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
    printf("# of colors   = %d\n", (int)(attrib.colors.size()) / 3);
    printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
    printf("# of materials = %d\n", (int)materials.size());
    printf("# of shapes    = %d\n", (int)shapes.size());

    // Append `default` material
    materials.push_back(tinyobj::material_t());

    for (size_t i = 0; i < materials.size(); i++) {
        printf("material[%d].diffuse_texname = %s\n", int(i),
            materials[i].diffuse_texname.c_str());
    }

    // Load diffuse textures
    {
        for (size_t m = 0; m < materials.size(); m++) {
            tinyobj::material_t* mp = &materials[m];

            if (mp->diffuse_texname.length() > 0) {
                // Only load the texture if it is not already loaded
                if (textures.find(mp->diffuse_texname) == textures.end()) {
                    GLuint texture_id;
                    int w, h;
                    int comp;

                    std::string texture_filename = mp->diffuse_texname;
                    if (!FileExists(texture_filename)) {
                        // Append base dir.
                        texture_filename = base_dir + mp->diffuse_texname;
                        if (!FileExists(texture_filename)) {
                            std::cerr << "Unable to find file: " << mp->diffuse_texname
                                << std::endl;
                            exit(1);
                        }
                    }

                    unsigned char* image =
                        stbi_load(texture_filename.c_str(), &w, &h, &comp, STBI_default);
                    if (!image) {
                        std::cerr << "Unable to load texture: " << texture_filename
                            << std::endl;
                        exit(1);
                    }
                    std::cout << "Loaded texture: " << texture_filename << ", w = " << w
                        << ", h = " << h << ", comp = " << comp << std::endl;

                    glGenTextures(1, &texture_id);
                    glBindTexture(GL_TEXTURE_2D, texture_id);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    if (comp == 3) {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
                            GL_UNSIGNED_BYTE, image);
                    }
                    else if (comp == 4) {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                            GL_UNSIGNED_BYTE, image);
                    }
                    else {
                        assert(0);  // TODO
                    }
                    glBindTexture(GL_TEXTURE_2D, 0);
                    stbi_image_free(image);
                    textures.insert(std::make_pair(mp->diffuse_texname, texture_id));
                }
            }
        }
    }

    bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
    bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

    {
        for (size_t s = 0; s < shapes.size(); s++) {
            DrawObject o;
            std::vector<float> buffer;  // pos(3float), normal(3float), color(3float)
            std::vector<unsigned int> ibuffer;

            // Check for smoothing group and compute smoothing normals
            std::map<int, vec3> smoothVertexNormals;
            if (hasSmoothingGroup(shapes[s]) > 0) {
                std::cout << "Compute smoothingNormal for shape [" << s << "]" << std::endl;
                computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
            }

            for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
                tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
                tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
                tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

                ibuffer.push_back(3 * f + 0);
                ibuffer.push_back(3 * f + 1);
                ibuffer.push_back(3 * f + 2);

                int current_material_id = shapes[s].mesh.material_ids[f];

                if ((current_material_id < 0) ||
                    (current_material_id >= static_cast<int>(materials.size()))) {
                    // Invaid material ID. Use default material.
                    current_material_id =
                        materials.size() -
                        1;  // Default material is added to the last item in `materials`.
                }
                // if (current_material_id >= materials.size()) {
                //    std::cerr << "Invalid material index: " << current_material_id <<
                //    std::endl;
                //}
                //
                float diffuse[3];
                for (size_t i = 0; i < 3; i++) {
                    diffuse[i] = materials[current_material_id].diffuse[i];
                }
                float tc[3][2];
                if (attrib.texcoords.size() > 0) {
                    if ((idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) ||
                        (idx2.texcoord_index < 0)) {
                        // face does not contain valid uv index.
                        tc[0][0] = 0.0f;
                        tc[0][1] = 0.0f;
                        tc[1][0] = 0.0f;
                        tc[1][1] = 0.0f;
                        tc[2][0] = 0.0f;
                        tc[2][1] = 0.0f;
                    }
                    else {
                        assert(attrib.texcoords.size() >
                            size_t(2 * idx0.texcoord_index + 1));
                        assert(attrib.texcoords.size() >
                            size_t(2 * idx1.texcoord_index + 1));
                        assert(attrib.texcoords.size() >
                            size_t(2 * idx2.texcoord_index + 1));

                        // Flip Y coord.
                        tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
                        tc[0][1] = 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1];
                        tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
                        tc[1][1] = 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1];
                        tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
                        tc[2][1] = 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1];
                    }
                }
                else {
                    tc[0][0] = 0.0f;
                    tc[0][1] = 0.0f;
                    tc[1][0] = 0.0f;
                    tc[1][1] = 0.0f;
                    tc[2][0] = 0.0f;
                    tc[2][1] = 0.0f;
                }

                float v[3][3];
                for (int k = 0; k < 3; k++) {
                    int f0 = idx0.vertex_index;
                    int f1 = idx1.vertex_index;
                    int f2 = idx2.vertex_index;
                    assert(f0 >= 0);
                    assert(f1 >= 0);
                    assert(f2 >= 0);

                    v[0][k] = attrib.vertices[3 * f0 + k];
                    v[1][k] = attrib.vertices[3 * f1 + k];
                    v[2][k] = attrib.vertices[3 * f2 + k];
                    bmin[k] = std::min(v[0][k], bmin[k]);
                    bmin[k] = std::min(v[1][k], bmin[k]);
                    bmin[k] = std::min(v[2][k], bmin[k]);
                    bmax[k] = std::max(v[0][k], bmax[k]);
                    bmax[k] = std::max(v[1][k], bmax[k]);
                    bmax[k] = std::max(v[2][k], bmax[k]);
                }

                float n[3][3];
                {
                    bool invalid_normal_index = false;
                    if (attrib.normals.size() > 0) {
                        int nf0 = idx0.normal_index;
                        int nf1 = idx1.normal_index;
                        int nf2 = idx2.normal_index;

                        if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0)) {
                            // normal index is missing from this face.
                            invalid_normal_index = true;
                        }
                        else {
                            for (int k = 0; k < 3; k++) {
                                assert(size_t(3 * nf0 + k) < attrib.normals.size());
                                assert(size_t(3 * nf1 + k) < attrib.normals.size());
                                assert(size_t(3 * nf2 + k) < attrib.normals.size());
                                n[0][k] = attrib.normals[3 * nf0 + k];
                                n[1][k] = attrib.normals[3 * nf1 + k];
                                n[2][k] = attrib.normals[3 * nf2 + k];
                            }
                        }
                    }
                    else {
                        invalid_normal_index = true;
                    }

                    if (invalid_normal_index && !smoothVertexNormals.empty()) {
                        // Use smoothing normals
                        int f0 = idx0.vertex_index;
                        int f1 = idx1.vertex_index;
                        int f2 = idx2.vertex_index;

                        if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                            n[0][0] = smoothVertexNormals[f0].v[0];
                            n[0][1] = smoothVertexNormals[f0].v[1];
                            n[0][2] = smoothVertexNormals[f0].v[2];

                            n[1][0] = smoothVertexNormals[f1].v[0];
                            n[1][1] = smoothVertexNormals[f1].v[1];
                            n[1][2] = smoothVertexNormals[f1].v[2];

                            n[2][0] = smoothVertexNormals[f2].v[0];
                            n[2][1] = smoothVertexNormals[f2].v[1];
                            n[2][2] = smoothVertexNormals[f2].v[2];

                            invalid_normal_index = false;
                        }
                    }

                    if (invalid_normal_index) {
                        // compute geometric normal
                        CalcNormal(n[0], v[0], v[1], v[2]);
                        n[1][0] = n[0][0];
                        n[1][1] = n[0][1];
                        n[1][2] = n[0][2];
                        n[2][0] = n[0][0];
                        n[2][1] = n[0][1];
                        n[2][2] = n[0][2];
                    }
                }

                for (int k = 0; k < 3; k++) {
                    buffer.push_back(v[k][0]);
                    buffer.push_back(v[k][1]);
                    buffer.push_back(v[k][2]);
                    buffer.push_back(n[k][0]);
                    buffer.push_back(n[k][1]);
                    buffer.push_back(n[k][2]);
                     /*Combine normal and diffuse to get color.*/
                    float normal_factor = 0.2;
                    float diffuse_factor = 1 - normal_factor;
                    float c[3] = { n[k][0] * normal_factor + diffuse[0] * diffuse_factor,
                                  n[k][1] * normal_factor + diffuse[1] * diffuse_factor,
                                  n[k][2] * normal_factor + diffuse[2] * diffuse_factor };
                    float len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
                    if (len2 > 0.0f) {
                        float len = sqrtf(len2);

                        c[0] /= len;
                        c[1] /= len;
                        c[2] /= len;
                    }
                    buffer.push_back(c[0] * 0.5 + 0.5);
                    buffer.push_back(c[1] * 0.5 + 0.5);
                    buffer.push_back(c[2] * 0.5 + 0.5);

                    buffer.push_back(tc[k][0]);
                    buffer.push_back(tc[k][1]);
                }
            }

            o.vb_id = 0;
            o.numTriangles = 0;

            // OpenGL viewer does not support texturing with per-face material.
            if (shapes[s].mesh.material_ids.size() > 0 &&
                shapes[s].mesh.material_ids.size() > s) {
                o.material_id = shapes[s].mesh.material_ids[0];  // use the material ID
                                                                 // of the first face.
            }
            else {
                o.material_id = materials.size() - 1;  // = ID for default material.
            }
            printf("shape[%d] material_id %d\n", int(s), int(o.material_id));

            if (buffer.size() > 0) {
                glGenBuffers(1, &o.vb_id);
                glBindBuffer(GL_ARRAY_BUFFER, o.vb_id);
                glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
                    &buffer.at(0), GL_STATIC_DRAW);

                GLCall(glGenBuffers(1, &o.ib_id));
                GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, o.ib_id));
                GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibuffer.size() * sizeof(unsigned int), &ibuffer.at(0), GL_STATIC_DRAW));

                GLCall(glGenVertexArrays(1, &o.va_id));
                GLCall(glBindVertexArray(o.va_id));
                GLCall(glBindBuffer(GL_ARRAY_BUFFER, o.vb_id));
                const auto& elements = layout.GetElements();
                unsigned int offset = 0;
                for (unsigned int i = 0; i < elements.size(); i++)
                {
                    const auto& element = elements[i];
                    GLCall(glEnableVertexAttribArray(i));
                    GLCall(glVertexAttribPointer(i, element.count, element.type,
                        element.normalized, layout.GetStride(), (const void*)offset));
                    offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
                }

                o.numTriangles = ibuffer.size() / 3;

                printf("shape[%d] # of triangles = %d\n", static_cast<int>(s),
                    o.numTriangles);
            }

            drawObjects->push_back(o);
        }
    }

    printf("bmin = %f, %f, %f\n", bmin[0], bmin[1], bmin[2]);
    printf("bmax = %f, %f, %f\n", bmax[0], bmax[1], bmax[2]);

    return true;
}//LoadObjAndConvert


int main(void) {

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
    glfwSetScrollCallback(window, scroll_callback);

    {
        /*GL_Settings*/
        GLCall(glEnable(GL_BLEND));
        glEnable(GL_DEPTH_TEST);
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        
        /*ObjModel*/
        std::string filepath = "models/king.obj";

        float bmin[3], bmax[3];


        VertexBufferLayout layout_Obj;
        layout_Obj.Push<float>(3); //vtx
        layout_Obj.Push<float>(3); //normal
        layout_Obj.Push<float>(3); //color
        layout_Obj.Push<float>(2); //uv

        Shader shader_Models("res/shaders/objShader.shader");
        shader_Models.Bind();
        shader_Models.SetUniform4f("u_Color", 1.0f, 0.4f, 0.9f, 1.0f);
        shader_Models.SetUniform1i("bool_Tex", 0);
        shader_Models.UnBind();

        std::vector<tinyobj::material_t> materials;
        std::map<std::string, GLuint> textures;
        if (false == LoadObjAndConvert(bmin, bmax, &gDrawObjects, materials, textures,
            filepath.c_str(),layout_Obj)) {
            return -1;
        }

        float maxExtent = 0.5f * (bmax[0] - bmin[0]);
        if (maxExtent < 0.5f * (bmax[1] - bmin[1])) {
            maxExtent = 0.5f * (bmax[1] - bmin[1]);
        }
        if (maxExtent < 0.5f * (bmax[2] - bmin[2])) {
            maxExtent = 0.5f * (bmax[2] - bmin[2]);
        }


        /*Box*/
        float positions[] =
        {
            -50.0f, -50.0f, 50.f, 0.0f, 0.0f, // 0
            50.0f, -50.0f, 50.f, 1.0f, 0.0f, // 1
            50.0f, 50.0f, 50.f, 1.0f, 1.0f, // 2
            -50.0f, 50.0f, 50.f,0.0f, 1.0f,  // 3
            -50.0f, -50.0f, -50.f, 0.0f, 0.0f, // 4
            50.0f, -50.0f, -50.f, 1.0f, 0.0f, // 5
            50.0f, 50.0f, -50.f, 1.0f, 1.0f, // 6
            -50.0f, 50.0f, -50.f, 0.0f, 1.0f  // 7
        };

        /*index_datar*/
        unsigned int indices[] = {
            0,1,2,
            2,3,0,

            1,5,6,
            6,2,1,

            5,4,7,
            7,6,5,

            4,0,3,
            3,7,4,

            3,2,6,
            6,7,3,

            4,5,1,
            1,0,4,
        };

        /*Plane*/
        //float positions[] = {
        //     -50.0f, -50.0f, 0.0f, 0.0f, // 0
        //     50.0f, -50.0f, 1.0f, 0.0f, // 1
        //     50.0f, 50.0f, 1.0f, 1.0f, // 2
        //     -50.0f, 50.0f, 0.0f, 1.0f  // 3
        //};

        ///*index_datar*/
        //unsigned int indices[] = {
        //    0,1,2,
        //    2,3,0
        //};


        /*make VertexArray*/
        VertexArray va;
        VertexBuffer vb(positions, sizeof(positions));

        /*make VertexBufferLayout*/
        VertexBufferLayout layout;
        //layout.Push<float>(2); //Plane_position
        layout.Push<float>(3); //Box_position
        layout.Push<float>(2); //uv
        va.addBuffer(vb, layout);

        /*make IndexBuffer*/
        IndexBuffer ib(indices, sizeof(indices));

        /*make Shader*/
        Shader shader("res/shaders/practice1.shader");

        Texture texture("res/textures/uvColor.png");
        texture.Bind(0);
        Texture texture1("res/textures/uvColor.png");
        texture.Bind(0);
        shader.SetUniform1i("u_Texture", 0);
        shader.SetUniform4f("u_Color", 1.0f,0.4f,0.9f,1.0f);

        /*UnBind all before main loop*/
        va.UnBind();
        //vb.Unbind();
        ib.Unbind();
        shader.UnBind();


        /*Light*/

        shader.SetUniform3f("Light_Color",1.0f,1.0f,1.0f);
        shader.SetUniform3f("Light_Position", 1.0f, 1.0f, 1.0f);


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
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            processInput(window);

            /* Render here */
            renderer.Clear();

            ImGui_ImplGlfwGL3_NewFrame();

            /*View Update*/
            camera.SetZoom(FOV);
            glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);
            glm::mat4 view = camera.GetViewMatrix();

            /*Draw Object*/
            {
                {
                    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(-55.0f), translationB);
                    model = glm::translate(model, translationB);
                    glm::mat4 mvp = proj * view * model;
                    texture.Bind();
                    shader.Bind();
                    shader.SetUniformMat4f("u_MVP", mvp);
                    renderer.Draw(va, ib, shader);
                    shader.UnBind();
                }
                //{
                //    glm::mat4 model = glm::translate(glm::mat4(1.0f), translationB);
                //    glm::mat4 mvp = proj * view * model;
                //    shader.Bind();
                //    shader.SetUniformMat4f("u_MVP", mvp);
                //    renderer.Draw(va, ib, shader);
                //}
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
                glm::mat4 mvp = proj * view * model;
                shader_Models.Bind();
                shader_Models.SetUniformMat4f("u_MVP", mvp);
                renderer.DrawObj(gDrawObjects, materials, textures, shader_Models);
                shader_Models.UnBind();
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

	if(mouseActive) camera.ProcessMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.SpeedManager(true);
	else
		camera.SpeedManager(false);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseActive = false;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseActive = true;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}