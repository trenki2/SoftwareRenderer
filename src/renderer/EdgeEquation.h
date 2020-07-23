/*
MIT License

Copyright (c) 2017-2020 Markus Trenkwalder

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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