#pragma once

#include "VertexArray.h"
#include "IndexBuffer.h"

class RawModel {
private:
	VertexArray m_Vao;
	IndexBuffer m_Ibo;
	
public:
	RawModel(VertexArray& vao, IndexBuffer& ibo)
		: m_Vao(vao), m_Ibo(ibo)
	{

	}

	VertexArray getVao() {
		return m_Vao;
	}

	IndexBuffer getIbo() {
		return m_Ibo;
	}
};