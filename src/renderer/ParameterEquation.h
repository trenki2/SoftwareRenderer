#pragma once

#include "EdgeEquation.h"

namespace swr {

struct ParameterEquation {
	float a;
	float b;
	float c;

	void init(
		float p0,
		float p1, 
		float p2, 
		const EdgeEquation &e0, 
		const EdgeEquation &e1, 
		const EdgeEquation &e2, 
		float factor)
	{
		a = factor * (p0 * e0.a + p1 * e1.a + p2 * e2.a);
		b = factor * (p0 * e0.b + p1 * e1.b + p2 * e2.b);
		c = factor * (p0 * e0.c + p1 * e1.c + p2 * e2.c);
	}

	// Evaluate the parameter equation for the given point.
	float evaluate(float x, float y) const
	{
		return a * x + b * y + c;
	}

	// Step parameter value v in x direction.
	float stepX(float v) const
	{
		return v + a;
	}

	// Step parameter value v in x direction.
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	// Step parameter value v in y direction.
	float stepY(float v) const
	{
		return v + b;
	}

	// Step parameter value v in y direction.
	float stepY(float v, float stepSize) const
	{
		return v + b * stepSize;
	}
};

} // end namespace swr