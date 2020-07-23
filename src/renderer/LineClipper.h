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

#include "VertexConfig.h"

namespace swr {

class LineClipper {
private:
	const VertexShaderOutput &m_v0;
	const VertexShaderOutput &m_v1;

public:
	float t0;
	float t1;
	bool fullyClipped;

	LineClipper(const VertexShaderOutput &v0, const VertexShaderOutput &v1)
		: m_v0(v0)
		, m_v1(v1)
		, t0(0.0f)
		, t1(1.0f)
		, fullyClipped(false)
	{
	}

	/// Clip the line segment to the plane given by the formula a * x + b * y + c * z + d * w.
	void clipToPlane(float a, float b, float c, float d);
};

} // end namespace swr