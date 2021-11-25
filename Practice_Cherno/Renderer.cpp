#include "Renderer.h"

#include <iostream>
#include "Debug.h"
void Renderer::Clear() const
{
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Renderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const
{
    shader.Bind();
    va.Bind();
    ib.Bind_vtx();
    GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}    

void Renderer::Draw(const std::vector<DrawObject>& drawObjects,
    std::vector<tinyobj::material_t>& materials,
    std::map<std::string, GLuint>& textures,
    const VertexArray& va,
    const Shader& shader)
{
    //glEnable(GL_TEXTURE_2D);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glPolygonOffset(1.0, 1.0);
    GLsizei stride = (3 + 3 + 3 + 2) * sizeof(float);

    va.Bind();

    for (size_t i = 0; i < drawObjects.size(); i++) {
        DrawObject o = drawObjects[i];
        if (o.vb_id < 1) {
            continue;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        if ((o.material_id < materials.size())) {
            std::string diffuse_texname = materials[o.material_id].diffuse_texname;
            if (textures.find(diffuse_texname) != textures.end()) {
                glBindTexture(GL_TEXTURE_2D, textures[diffuse_texname]);
            }
        }
        shader.Bind();
        o.ibos[i].Bind_vtx();
        GLCall(glDrawElements(GL_TRIANGLES, o.ibos[i].GetCount(), GL_UNSIGNED_INT, nullptr));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}