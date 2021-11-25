#pragma once

#include <GL/glew.h>
#include <map>
#include "vendor/tinyobjloader/tiny_obj_loader.h"


#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

typedef struct {
    GLuint vb_id;  // vertex buffer id
    int numTriangles;
    size_t material_id;
    std::vector<IndexBuffer> ibos;
} DrawObject;

class Renderer
{
public:
    void Clear() const;
	void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;
    void Draw(const std::vector<DrawObject>& drawObjects,
        std::vector<tinyobj::material_t>& materials,
        std::map<std::string, GLuint>& textures,
        const VertexArray& va,
        const Shader& shader);

};