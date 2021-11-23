#pragma once

#include "Texture.h"

class ModelTexture
{
private:
	Texture texture;

public:
	ModelTexture(Texture& tex)
		: texture(tex)
	{

	}

	inline Texture getTexrue() const
	{
		return texture;
	}
};