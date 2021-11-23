#pragma once

#include "TexturedModel.h"

class Entity2
{
private:
	TexturedModel model;
	glm::vec3 position;
	glm::vec3 rotation;
	float scale;

public:
	inline Entity2(TexturedModel& m, const glm::vec3& pos,const glm::vec3& rot, const float& scl)
		: model(m), position(pos), rotation(rot), scale(scl)
	{

	}

	inline void increasePosition(float dx, float dy, float dz)
	{
		position += glm::vec3(dx, dy, dz);
	}

	inline void increaseRotation(float dx, float dy, float dz)
	{
		rotation += glm::vec3(dx, dy, dz);
	}

	inline TexturedModel getModel() const
	{
		return model;
	}

	inline void setModel(TexturedModel mod) {
		model = mod;
	}

	inline glm::vec3 getPosition() const
	{
		return position;
	}
};

