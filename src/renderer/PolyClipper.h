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
#include <vector>

namespace swr {

class Helper {
public:
	static VertexShaderOutput interpolateVertex(const VertexShaderOutput &v0, const VertexShaderOutput &v1, float t, int avarCount, int pvarCount)
	{
		VertexShaderOutput result;
		
		result.x = v0.x * (1.0f - t) + v1.x * t;
		result.y = v0.y * (1.0f - t) + v1.y * t;
		result.z = v0.z * (1.0f - t) + v1.z * t;
		result.w = v0.w * (1.0f - t) + v1.w * t;
		for (int i = 0; i < avarCount; ++i)
			result.avar[i] = v0.avar[i] * (1.0f - t) + v1.avar[i] * t;
		for (int i = 0; i < pvarCount; ++i)
			result.pvar[i] = v0.pvar[i] * (1.0f - t) + v1.pvar[i] * t;

		return result;
	}
};

class PolyClipper {
private:
	int m_avarCount;
	int m_pvarCount;
	std::vector<int> *m_indicesIn;
	std::vector<int> *m_indicesOut;
	std::vector<VertexShaderOutput> *m_vertices;
	
public:
	PolyClipper()
	{
		m_avarCount = 0;
		m_pvarCount = 0;
		m_vertices = NULL;
		m_indicesIn = new std::vector<int>();
		m_indicesOut = new std::vector<int>();
	}

	~PolyClipper()
	{
		delete m_indicesIn;
		delete m_indicesOut;
	}

	void init(std::vector<VertexShaderOutput> *vertices, int i1, int i2, int i3, int avarCount, int pvarCount);

	// Clip the poly to the plane given by the formula a * x + b * y + c * z + d * w.
	void clipToPlane(float a, float b, float c, float d);

	std::vector<int> &indices() const
	{
		return *m_indicesIn;
	}

	bool fullyClipped() const
	{
		return m_indicesIn->size() < 3;
	}

private:
	template <typename T> int sgn(T val) 
	{
    	return (T(0) < val) - (val < T(0));
	}
};

} // end namespace swr