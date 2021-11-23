#include "Renderer.h"

#include <iostream>
#include "Debug.h"
void Renderer::Clear() const
{
    GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void Renderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const
{
    shader.Bind();
    va.Bind();
    ib.Bind();
    GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}    

void Renderer::Draw(const std::vector<DrawObject>& drawObjects,
    std::vector<tinyobj::material_t>& materials,
    std::map<std::string, GLuint>& textures,
    const Shader& shader)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    GLsizei stride = (3 + 3 + 3 + 2) * sizeof(float);
    for (size_t i = 0; i < drawObjects.size(); i++) {
        DrawObject o = drawObjects[i];
        if (o.vb_id < 1) {
            continue;
        }

        glBindBuffer(GL_ARRAY_BUFFER, o.vb_id);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        //glBindTexture(GL_TEXTURE_2D, 0);
        //if ((o.material_id < materials.size())) {
        //    std::string diffuse_texname = materials[o.material_id].diffuse_texname;
        //    if (textures.find(diffuse_texname) != textures.end()) {
        //        glBindTexture(GL_TEXTURE_2D, textures[diffuse_texname]);
        //    }
        //}
        glVertexPointer(3, GL_FLOAT, stride, (const void*)0);
        glNormalPointer(GL_FLOAT, stride, (const void*)(sizeof(float) * 3));
        glColorPointer(3, GL_FLOAT, stride, (const void*)(sizeof(float) * 6));
        glTexCoordPointer(2, GL_FLOAT, stride, (const void*)(sizeof(float) * 9));

        shader.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3 * o.numTriangles);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}