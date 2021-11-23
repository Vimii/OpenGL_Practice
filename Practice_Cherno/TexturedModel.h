#pragma once

#include "RawModel.h"
#include "ModelTexture.h"

class TexturedModel {
private:
	RawModel rawModel;
	ModelTexture texture;

public:
	inline TexturedModel(RawModel model, ModelTexture tex)
		: rawModel(model), texture(tex)
	{

	}

	inline RawModel getRawModel() const
	{
		return rawModel;
	}

	inline ModelTexture getTexture() const
	{
		return texture;
	}
};