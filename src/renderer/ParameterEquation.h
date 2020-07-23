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