#pragma once

#include "IRasterizer.h"

namespace swr {

struct EdgeEquation {
	float a;
	float b;
	float c;
	bool tie; 

	void init(const RasterizerVertex &v0, const RasterizerVertex &v1)
	{
		a = v0.y - v1.y;
		b = v1.x - v0.x;
		c = - (a * (v0.x + v1.x) + b * (v0.y + v1.y)) / 2;
		tie = a != 0 ? a > 0 : b > 0;
	}

	// Evaluate the edge equation for the given point.
	float evaluate(float x, float y) const
	{
		return a * x + b * y + c;
	}

	// Test if the given point is inside the edge.
	bool test(float x, float y) const
	{
		return test(evaluate(x, y));
	}

	// Test for a given evaluated value.
	bool test(float v) const
	{
		return (v > 0 || (v == 0 && tie));
	}

	// Step the equation value v to the x direction
	float stepX(float v) const
	{
		return v + a;
	}

	// Step the equation value v to the x direction
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	// Step the equation value v to the y direction
	float stepY(float v) const
	{
		return v + b;
	}

	// Step the equation value vto the y direction
	float stepY(float v, float stepSize) const
	{
		return v + b * stepSize;
	}
};

} // end namespace swr