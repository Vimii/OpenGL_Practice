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
Camera camera(glm::vec3(-640.0f, 100.0f, -200.0f));
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
bool mouseActive = true , debug = true;



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
        printf("material[%d].specular_texname = %s\n", int(i),
            materials[i].specular_texname.c_str());
        printf("material[%d].reflection_texname = %s\n", int(i),
            materials[i].reflection_texname.c_str());
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

    // Load specular textures
    {
        for (size_t m = 0; m < materials.size(); m++) {
            tinyobj::material_t* mp = &materials[m];

            if (mp->specular_texname.length() > 0) {
                // Only load the texture if it is not already loaded
                if (textures.find(mp->specular_texname) == textures.end()) {
                    GLuint texture_id;
                    int w, h;
                    int comp;

                    std::string texture_filename = mp->specular_texname;
                    if (!FileExists(texture_filename)) {
                        // Append base dir.
                        texture_filename = base_dir + mp->specular_texname;
                        if (!FileExists(texture_filename)) {
                            std::cerr << "Unable to find file: " << mp->specular_texname
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
                    textures.insert(std::make_pair(mp->specular_texname, texture_id));
                }
            }
        }
    }

    // Load reflection textures
    {
        for (size_t m = 0; m < materials.size(); m++) {
            tinyobj::material_t* mp = &materials[m];

            if (mp->reflection_texname.length() > 0) {
                // Only load the texture if it is not already loaded
                if (textures.find(mp->reflection_texname) == textures.end()) {
                    GLuint texture_id;
                    int w, h;
                    int comp;

                    std::string texture_filename = mp->reflection_texname;
                    if (!FileExists(texture_filename)) {
                        // Append base dir.
                        texture_filename = base_dir + mp->reflection_texname;
                        if (!FileExists(texture_filename)) {
                            std::cerr << "Unable to find file: " << mp->reflection_texname
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
                    textures.insert(std::make_pair(mp->reflection_texname, texture_id));
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

/*Skybox*/
unsigned int loadCubemap(std::string dir, std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load((dir + faces[i]).c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

float gauss(float x, float y, float sigma)
{
    return  1.0f / (2.0f * 3.141592 * sigma * sigma) * exp(-(x * x + y * y) / (2.0f * sigma * sigma));
}

int main(void) {

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Secret Cove 3D", NULL, NULL);
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

        /*Skybox*/

        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };
        Shader skyboxShader("res/shaders/skybox.shader");

        std::string skyboxDir = "res/cubemaps/02/";
        std::vector<std::string> faces
        {
                "px.png",
                "nx.png",
                "py.png",
                "ny.png",
                "pz.png",
                "nz.png"
        };
        unsigned int cubemapTexture = loadCubemap(skyboxDir, faces);

        VertexArray skyboxVAO;
        VertexBuffer skyboxVBO(skyboxVertices, sizeof(skyboxVertices));
        VertexBufferLayout skyboxLayout;
        skyboxLayout.Push<float>(3); //Box_position
        skyboxVAO.addBuffer(skyboxVBO, skyboxLayout);
        
        /*ObjModel*/
        std::vector<DrawObject> gDrawObjects1, gDrawObjects2, gDrawObjects3;
        
        //std::string filepath = "models/sponza_bump/sponza.obj";
        //std::string filepath = "models/dragon.obj";
        //std::string filepath = "models/bunny.obj";
        //std::string filepath = "models/flag.obj";

        std::string filepath1 = "models/Cove/Cove.obj";
        std::string filepath2 = "models/Cove/unlitObjects.obj";
        std::string filepath3 = "models/tyra.obj";

        float bmin[3], bmax[3];
        glm::vec3 Translation_Model(-470.0f,164.0f,535.0f);
        float Rotation_Model(19.0f);


        VertexBufferLayout layout_Obj;
        layout_Obj.Push<float>(3); //vtx
        layout_Obj.Push<float>(3); //normal
        layout_Obj.Push<float>(3); //color
        layout_Obj.Push<float>(2); //uv

        //Shader shader_Models("res/shaders/objShader.shader");
        Shader shader_Models("res/shaders/objShader_Reflection.shader");
        Shader shader_Refraction("res/shaders/objShader_Refraction.shader");
        //Shader shader_Models("res/shaders/objShader_NormalMapping.shader"); //未実装
        shader_Models.Bind();
        shader_Models.SetUniform4f("u_Color", 1.0f, 0.4f, 0.9f, 1.0f);
        shader_Models.SetUniform1i("bool_Tex_Dif", 0);
        shader_Models.SetUniform1i("bool_Tex_Spec", 0);
        float ShineDamper = 16.0;
        float Reflectivity = 0.5;
        shader_Models.SetUniform1f("shineDamper", ShineDamper);
        shader_Models.SetUniform1f("reflectivity", Reflectivity);

        /*Reflection Option*/
        GLCall(glActiveTexture(GL_TEXTURE8));
        GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture));
        shader_Models.SetUniform1i("skybox", cubemapTexture);

        shader_Models.UnBind();

        //refraction shader
        shader_Refraction.Bind();
        shader_Refraction.SetUniform4f("u_Color", 1.0f, 0.4f, 0.9f, 1.0f);
        shader_Refraction.SetUniform1i("bool_Tex_Dif", 0);
        shader_Refraction.SetUniform1i("bool_Tex_Spec", 0);
        float transparency = 1.0;
        shader_Refraction.SetUniform1f("shineDamper", ShineDamper);


        /*Refraction Option*/
        GLCall(glActiveTexture(GL_TEXTURE8));
        GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture));
        shader_Refraction.SetUniform1i("skybox", cubemapTexture);

        shader_Refraction.UnBind();

        std::vector<tinyobj::material_t> materials1;
        std::map<std::string, GLuint> textures1;
        if (false == LoadObjAndConvert(bmin, bmax, &gDrawObjects1, materials1, textures1,
            filepath1.c_str(),layout_Obj)) {
            return -1;
        }

        std::vector<tinyobj::material_t> materials2;
        std::map<std::string, GLuint> textures2;
        if (false == LoadObjAndConvert(bmin, bmax, &gDrawObjects2, materials2, textures2,
            filepath2.c_str(), layout_Obj)) {
            return -1;
        }

        std::vector<tinyobj::material_t> materials3;
        std::map<std::string, GLuint> textures3;
        if (false == LoadObjAndConvert(bmin, bmax, &gDrawObjects3, materials3, textures3,
            filepath3.c_str(), layout_Obj)) {
            return -1;
        }

        /*bugfix*/
        for (size_t i = 0; i < gDrawObjects2.size(); i++) {
            DrawObject& o = gDrawObjects2[i];
            if (o.vb_id < 1) {
                continue;
            }
            
            if ((o.material_id < materials2.size())) {
                if (o.numTriangles == 2) gDrawObjects2[i].material_id = 2 + rand() % 2;
                if (o.numTriangles == 6) gDrawObjects2[i].material_id = 7;
            }
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
        float planePositions[] = {
             -50.0f, -50.0f, 0.0f, 0.0f, // 0
             50.0f, -50.0f, 1.0f, 0.0f, // 1
             50.0f, 50.0f, 1.0f, 1.0f, // 2
             -50.0f, 50.0f, 0.0f, 1.0f  // 3
        };

        /*index_datar*/
        unsigned int planeIndices[] = {
            0,1,2,
            2,3,0
        };


        /*make VertexArray*/
        VertexArray va;
        VertexBuffer vb(planePositions, sizeof(planePositions));

        /*make VertexBufferLayout*/
        VertexBufferLayout layout;
        layout.Push<float>(2); //Plane_position
        //layout.Push<float>(3); //Box_position
        layout.Push<float>(2); //uv
        va.addBuffer(vb, layout);

        /*make IndexBuffer*/
        IndexBuffer ib(planeIndices, sizeof(planeIndices));

        /*make Shader*/
        Shader shader("res/shaders/practice1.shader");

        Texture texture("res/textures/uvColor.png");
        texture.Bind(0);
        shader.SetUniform1i("u_Texture", 0);
        shader.SetUniform4f("u_Color", 1.0f,0.4f,0.9f,1.0f);

        /*UnBind all before main loop*/
        va.UnBind();
        //vb.Unbind();
        ib.Unbind();
        shader.UnBind();

        //water
        VertexArray waterVAO;
        VertexBuffer waterVBO(planePositions, sizeof(planePositions));

        VertexBufferLayout planeLayout;
        planeLayout.Push<float>(2); //Plane_position
        planeLayout.Push<float>(2); //uv
        waterVAO.addBuffer(waterVBO, planeLayout);

        /*make IndexBuffer*/
        IndexBuffer planeIB(planeIndices, sizeof(planeIndices));

        /*make Shader*/
        Shader WaterShader("res/shaders/WaterShader.shader");
        WaterShader.Bind();
        WaterShader.SetUniform4f("u_Color", 1.0f, 0.4f, 0.9f, 1.0f);


        /*Light*/
        glm::vec3 LightColor(1.0f, 1.0f, 1.0f);
        glm::vec3 LightPosition(100.0f, 1000.0f, 0.0f);
        shader_Models.SetUniform3f("lightColor",LightColor);
        shader_Models.SetUniform3f("lightPosition", LightPosition);


        /*make Renderer*/
        Renderer renderer;

        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, true);
        ImGui::StyleColorsDark();
        

        /*ScreenQuad*/

        float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
        };

        // screen quad VAO
        unsigned int quadVAO, quadVBO;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        /*framebuffer*/
        unsigned int fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        unsigned int textureColorbuffer;
        glGenTextures(1, &textureColorbuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

        unsigned int textureDepthbuffer;
        /* テクスチャオブジェクトの生成 */
        glGenTextures(1, &textureDepthbuffer);
        glBindTexture(GL_TEXTURE_2D, textureDepthbuffer);

        /* デプステクスチャの割り当て */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
            GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

        /* テクスチャを拡大・縮小する方法の指定 */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        /* テクスチャの繰り返し方法の指定 */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        /* テクスチャオブジェクトの結合解除 */
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepthbuffer, 0);

        //glDrawBuffer(GL_NONE);
        //glReadBuffer(GL_NONE);

        //unsigned int rbo;
        //glGenRenderbuffers(1, &rbo);
        //glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Shader framebufferShader("res/shaders/Framebuffer.shader");
        bool postprocess = false;
        
        //postprocess

        //bloom
        unsigned int splitFBO;
        glGenFramebuffers(1, &splitFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, splitFBO);
        unsigned int splitBuffers[2];
        glGenTextures(2, splitBuffers);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, splitBuffers[i]);
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, splitBuffers[i], 0);
        }
        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1 };
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        unsigned int bloomFBO[2];
        unsigned int bloomBuffers[2];
        glGenFramebuffers(2, bloomFBO);
        glGenTextures(2, bloomBuffers);
        for (size_t i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[i]);
            glBindTexture(GL_TEXTURE_2D, bloomBuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBuffers[i], 0
            );
        }

        Shader brightSplitShader("res/shaders/bloomBrightColorSplitter.shader");
        float bloomThreshold = 0.7;

        Shader blurShader("res/shaders/Blur.shader");
        int bloomBlurAmount = 10;

        Shader bloomShader("res/shaders/bloom.shader");
        float bloomIntensity = 1.0f;

        /*util valiables*/        

        float r = 0.0f;
        float increment = 0.05f;
        glm::vec3 translationB(400, 200, 0);
        float time = 0.0f;

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {    
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            processInput(window);

            /* Render here */
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);
            //glEnable(GL_CLIP_DISTANCE0);
            
            renderer.Clear();

            ImGui_ImplGlfwGL3_NewFrame();

            /*View Update*/
            camera.SetZoom(FOV);
            glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 2000.0f);
            glm::mat4 view = camera.GetViewMatrix();

            /*Skybox Update*/
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // remove translation from the view matrix
            GLCall(glDepthFunc(GL_LEQUAL));
            skyboxShader.Bind();
            skyboxShader.SetUniformMat4f("projection", proj);
            skyboxShader.SetUniformMat4f("view",skyboxView);
            skyboxVAO.Bind();
            GLCall(glActiveTexture(GL_TEXTURE0));
            GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture));
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 36));
            GLCall(glDepthFunc(GL_LESS));

            /*light Update*/
            shader_Models.Bind();
            shader_Models.SetUniform3f("lightPosition", LightPosition);
            shader_Models.SetUniform3f("lightColor", LightColor);
            
            /*material Update*/
            shader_Models.SetUniform1f("shineDamper", ShineDamper);
            shader_Models.SetUniform1f("reflectivity", Reflectivity);

            
            {
                /*Draw Object*/
                //{
                //    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(-55.0f), translationB);
                //    model = glm::translate(model, translationB);
                //    glm::mat4 mvp = proj * view * model;
                //    texture.Bind();
                //    shader.Bind();
                //    shader.SetUniformMat4f("u_MVP", mvp);
                //    renderer.Draw(va, ib, shader);
                //    shader.UnBind();
                //}
                //{
                //    glm::mat4 model = glm::translate(glm::mat4(1.0f), translationB);
                //    glm::mat4 mvp = proj * view * model;
                //    shader.Bind();
                //    shader.SetUniformMat4f("u_MVP", mvp);
                //    renderer.Draw(va, ib, shader);
                //}
                {
                    glm::mat4 model = glm::mat4(1.0f);
                    //model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
                    model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
                    //model = glm::translate(model, Translation_Model);
                    GLCall(glActiveTexture(GL_TEXTURE7));
                    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture));
                    shader_Models.Bind();
                    shader_Models.SetUniform1i("u_Texture_Normal", 3);
                    shader_Models.SetUniform1i("skybox", 7);
                    shader_Models.SetUniformMat4f("u_Projection", proj);
                    shader_Models.SetUniformMat4f("u_View", view);
                    shader_Models.SetUniformMat4f("u_Model", model);
                    shader_Models.SetUniform4f("plane", 0, -1, 0 ,Translation_Model.g);
                    renderer.DrawObj(gDrawObjects1, materials1, textures1, shader_Models);
                    shader_Models.UnBind();
                }
                {
                    /*unlitObjects*/
                    glm::mat4 model = glm::mat4(1.0f);
                    //model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
                    model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
                    //model = glm::translate(model, Translation_Model);
                    GLCall(glActiveTexture(GL_TEXTURE7));
                    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture));
                    shader_Models.Bind();
                    shader_Models.SetUniform1f("reflectivity", 0);
                    shader_Models.SetUniform1i("u_Texture_Normal", 3);
                    shader_Models.SetUniform1i("skybox", 7);
                    shader_Models.SetUniformMat4f("u_Projection", proj);
                    shader_Models.SetUniformMat4f("u_View", view);
                    shader_Models.SetUniformMat4f("u_Model", model);
                    shader_Models.SetUniform4f("plane", 0, -1, 0, Translation_Model.g);
                    renderer.DrawObj(gDrawObjects2, materials2, textures2, shader_Models);
                    shader_Models.UnBind();
                }
                {
                    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f,10.0f,10.0f));
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    //model = glm::translate(model, translationB);
                    glm::mat4 mvp = proj * view * model;
                    WaterShader.Bind();
                    WaterShader.SetUniformMat4f("u_Projection", proj);
                    WaterShader.SetUniformMat4f("u_View", view);
                    WaterShader.SetUniformMat4f("u_Model", model);
                    WaterShader.SetUniform1f("u_time", time);
                    WaterShader.SetUniform3f("cPos", camera.Position);
                    WaterShader.SetUniform3f("cDir", camera.Front);
                    WaterShader.SetUniform3f("cUp", camera.Up);
                    WaterShader.SetUniform2f("u_resolution", WINDOW_WIDTH, WINDOW_HEIGHT);
                    renderer.Draw(waterVAO, planeIB, WaterShader);
                    WaterShader.UnBind();
                }
                {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, Translation_Model);
                    model = glm::scale(model, glm::vec3(100.0f, 100.0f, 100.0f));
                    model = glm::rotate(model, glm::radians(Rotation_Model), glm::vec3(0.0, 1.0, 0.0));
                    GLCall(glActiveTexture(GL_TEXTURE7));
                    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture));
                    shader_Refraction.Bind();
                    shader_Refraction.SetUniform1i("u_Texture_Normal", 3);
                    shader_Refraction.SetUniform1i("skybox", 7);
                    shader_Refraction.SetUniformMat4f("u_Projection", proj);
                    shader_Refraction.SetUniformMat4f("u_View", view);
                    shader_Refraction.SetUniformMat4f("u_Model", model);
                    shader_Refraction.SetUniform1f("transparency", transparency);
                    shader_Refraction.SetUniform1f("shineDamper", ShineDamper);
                    shader_Refraction.SetUniform4f("plane", 0, -1, 0, Translation_Model.g);
                    renderer.DrawObj(gDrawObjects3, materials3, textures3, shader_Refraction);
                    shader_Refraction.UnBind();
                }
            }

            /*Draw Light*/
            {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), LightPosition);
                model = glm::scale(model, glm::vec3(0.01, 0.01, 0.01));
                glm::mat4 mvp = proj * view * model;
                shader.Bind();
                shader.SetUniform3f("u_Color", LightColor);
                shader.SetUniformMat4f("u_MVP", mvp);
                renderer.Draw(va, ib, shader);
                shader.UnBind();
            }



            // second pass
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(1.0f, 1.0f, 1.0f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            //Bloom Effect

            brightSplitShader.Bind();
            glBindVertexArray(quadVAO);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
            brightSplitShader.SetUniform1i("colorTexture", 0);
            brightSplitShader.SetUniform1f("threshold", bloomThreshold);
            
            glBindFramebuffer(GL_FRAMEBUFFER, splitFBO);
            glDrawBuffers(2, attachments);

            glDrawArrays(GL_TRIANGLES,0,6);

            glBindFramebuffer(GL_FRAMEBUFFER,0);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            bool horizontal = true, first_iteration = true;
            blurShader.Bind();
            for (size_t i = 0; i < bloomBlurAmount; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO[horizontal]);
                blurShader.SetUniform1i("horizontal", horizontal);
                glBindTexture(GL_TEXTURE_2D, first_iteration ? splitBuffers[1] : bloomBuffers[!horizontal]);
                blurShader.SetUniform1i("image", 0);
                glBindVertexArray(quadVAO);
                glDisable(GL_DEPTH_TEST);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                horizontal = !horizontal;
                if (first_iteration)
                    first_iteration = false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            bloomShader.Bind();
            glBindVertexArray(quadVAO);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, splitBuffers[0]);
            bloomShader.SetUniform1i("colorTexture", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, bloomBuffers[!horizontal]);
            bloomShader.SetUniform1i("brightTexture", 1);
            bloomShader.SetUniform1f("bloomIntensity", bloomIntensity);



            //framebufferShader.Bind();
            //glBindVertexArray(quadVAO);
            //glDisable(GL_DEPTH_TEST);
            //glActiveTexture(GL_TEXTURE0);
            //glBindTexture(GL_TEXTURE_2D, splitBuffers[0]);
            //framebufferShader.SetUniform1i("colorTexture", 0);
            //glActiveTexture(GL_TEXTURE1);
            //glBindTexture(GL_TEXTURE_2D, textureDepthbuffer);
            //framebufferShader.SetUniform1i("depthTexture", 1);
            //if(postprocess) framebufferShader.SetUniform1i("postprocess", 1);
            //else framebufferShader.SetUniform1i("postprocess", 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            /*Update valiables*/
            if (r > 1.0f)
                increment = -0.05f;
            else if (r < 0.0f)
                increment = 0.05f;

            r += increment;

            time += 0.1f;

            if (debug) {
                {
                    ImGui::Text("Camera");
                    ImGui::SliderFloat("FOV", &FOV, 0.0f, 90.0f);
                    ImGui::SliderFloat3("CameraPosition", &camera.Position.x, 0.0f, 1000.0f);
                    ImGui::Text("Light");
                    ImGui::SliderFloat3("Color", &LightColor.x, 0.0f, 10.0f);
                    ImGui::SliderFloat3("Position", &LightPosition.x, -1000.0f, 1000.0f);

                    ImGui::Text("Model");
                    ImGui::SliderFloat3("Translation", &Translation_Model.x, -1000.0f, 1000.0f);
                    ImGui::SliderFloat("Rotation", &Rotation_Model, -360.0f, 360.0f);
                    ImGui::Text("MaterialTest");
                    ImGui::SliderFloat("shineDamper", &ShineDamper, 0.0f, 20.0f);
                    ImGui::SliderFloat("reflectivity", &Reflectivity, 0.0f, 1.0f);
                    ImGui::SliderFloat("transparency", &transparency, 0.0f, 1.0f);

                    ImGui::Text("PostProcess");
                    ImGui::SliderFloat("BloomIntensity", &bloomIntensity, 0.0f, 3.0f);
                    ImGui::SliderFloat("BloomThreshold", &bloomThreshold, 0.0f, 1.0f);
                    ImGui::SliderInt("bloomBlurAmount", &bloomBlurAmount, 0, 50);
                    ImGui::Image((void*)(intptr_t)textureColorbuffer, ImVec2(WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3), ImVec2(0, 1), ImVec2(1, 0));


                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                }
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
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        debug = false;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
    {
        debug = true;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}