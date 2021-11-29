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

void Renderer::DrawObj(const std::vector<DrawObject>& drawObjects,
    std::vector<tinyobj::material_t>& materials,
    std::map<std::string, GLuint>& textures,
    Shader& shader)
{
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    GLsizei stride = (3 + 3 + 3 + 2) * sizeof(float);
    int texnum = 0;
    for (size_t i = 0; i < drawObjects.size(); i++) {
        DrawObject o = drawObjects[i];
        if (o.vb_id < 1) {
            continue;
        }

        shader.Bind();
        GLCall(glBindTexture(GL_TEXTURE_2D, 0));
        //GLCall(glActiveTexture(GL_TEXTURE0));
        //GLCall(glActiveTexture(GL_TEXTURE3));
        //GLCall(glActiveTexture(GL_TEXTURE5));
        if ((o.material_id < materials.size())) {
            std::string diffuse_texname = materials[o.material_id].diffuse_texname;
            if (textures.find(diffuse_texname) != textures.end()) {
                GLCall(glActiveTexture(GL_TEXTURE0));
                GLCall(glBindTexture(GL_TEXTURE_2D, textures[diffuse_texname]));
                std::string tex = "u_Texture" + std::to_string(textures[diffuse_texname]);
                shader.SetUniform1i("u_Texture" , 0);
                texnum++;
            }
        }
        GLCall(glBindVertexArray(o.va_id));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, o.ib_id));
        GLCall(glDrawElements(GL_TRIANGLES, o.numTriangles * 3, GL_UNSIGNED_INT, nullptr));

    }
}