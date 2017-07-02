#pragma once

#include "TriangleEquations.h"

namespace swr {

struct EdgeData {
	float ev0;
	float ev1;
	float ev2;

	// Initialize the edge data values.
	void init(const TriangleEquations &eqn, float x, float y)
	{
		ev0 = eqn.e0.evaluate(x, y);
		ev1 = eqn.e1.evaluate(x, y);
		ev2 = eqn.e2.evaluate(x, y);
	}

	// Step the edge values in the x direction.
	void stepX(const TriangleEquations &eqn)
	{
		ev0 = eqn.e0.stepX(ev0);
		ev1 = eqn.e1.stepX(ev1);
		ev2 = eqn.e2.stepX(ev2);
	}
	
	// Step the edge values in the x direction.
	void stepX(const TriangleEquations &eqn, float stepSize)
	{
		ev0 = eqn.e0.stepX(ev0, stepSize);
		ev1 = eqn.e1.stepX(ev1, stepSize);
		ev2 = eqn.e2.stepX(ev2, stepSize);
	}

	// Step the edge values in the y direction.
	void stepY(const TriangleEquations &eqn)
	{
		ev0 = eqn.e0.stepY(ev0);
		ev1 = eqn.e1.stepY(ev1);
		ev2 = eqn.e2.stepY(ev2);
	}

	// Step the edge values in the y direction.
	void stepY(const TriangleEquations &eqn, float stepSize)
	{
		ev0 = eqn.e0.stepY(ev0, stepSize);
		ev1 = eqn.e1.stepY(ev1, stepSize);
		ev2 = eqn.e2.stepY(ev2, stepSize);
	}

	// Test for triangle containment.
	bool test(const TriangleEquations &eqn)
	{
		return eqn.e0.test(ev0) && eqn.e1.test(ev1) && eqn.e2.test(ev2);
	}
};

} // end namespace swr