#pragma once

#include <vector>
#include "RawModel.h"

class Loader 
{
public:
	Loader();
	~Loader();


private:
	std::vector<VertexArray> vaos;
	
};