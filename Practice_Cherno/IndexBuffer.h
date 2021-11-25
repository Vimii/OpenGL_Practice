#pragma once
#include "tinyobjloader/tiny_obj_loader.h"
#include <vector>
class IndexBuffer
{
private:
	unsigned int m_RendererID_vtx;
	unsigned int m_RendererID_nrm;
	unsigned int m_RendererID_uv;
	unsigned int m_Count;
public:
	IndexBuffer(const unsigned int* data, unsigned int count);
	IndexBuffer(std::vector<tinyobj::index_t> indices);
	~IndexBuffer();

	void Bind_vtx() const;
	void Bind_nrm() const;
	void Bind_uv() const;
	void Unbind() const;

	inline unsigned int GetCount() const { return m_Count; }
};