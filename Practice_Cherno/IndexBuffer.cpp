#include "IndexBuffer.h"


#include "Renderer.h"
#include "Debug.h"
IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count)
    :m_Count(count)
{
    ASSERT(sizeof(unsigned int) == sizeof(GLuint));

    GLCall(glGenBuffers(1, &m_RendererID_vtx));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_vtx));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}


IndexBuffer::IndexBuffer(std::vector<tinyobj::index_t> indices)
{
    m_Count = indices.size() * 3;
    std::vector<int> data_vtx;

    for (size_t f = 0; f < indices.size(); f++) {
        data_vtx.push_back(indices[f].vertex_index);
    }

    GLCall(glGenBuffers(1, &m_RendererID_vtx));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_vtx));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int) * 3, data_vtx.data(), GL_STATIC_DRAW));

    std::vector<int> data_nrm;

    for (size_t f = 0; f < indices.size(); f++) {
        data_nrm.push_back(indices[f].normal_index);
    }

    GLCall(glGenBuffers(1, &m_RendererID_nrm));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_nrm));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int) * 3, data_nrm.data(), GL_STATIC_DRAW));

    std::vector<int> data_uv;

    for (size_t f = 0; f < indices.size(); f++) {
        data_uv.push_back(indices[f].texcoord_index);
    }

    GLCall(glGenBuffers(1, &m_RendererID_uv));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_uv));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int) * 3, data_uv.data(), GL_STATIC_DRAW));
}

IndexBuffer::~IndexBuffer()
{
    GLCall(glDeleteBuffers(1, &m_RendererID_vtx));
    //if(m_RendererID_nrm != NULL)
    //    GLCall(glDeleteBuffers(1, &m_RendererID_nrm));
    //if (m_RendererID_uv != NULL)
    //    GLCall(glDeleteBuffers(1, &m_RendererID_uv));
}

void IndexBuffer::Bind_vtx() const
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_vtx));
}

void IndexBuffer::Bind_nrm() const
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_nrm));
}

void IndexBuffer::Bind_uv() const
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID_uv));
}

void IndexBuffer::Unbind() const
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}