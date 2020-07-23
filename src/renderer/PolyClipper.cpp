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

#include "PolyClipper.h"

#include <algorithm>

namespace swr {

void PolyClipper::init(std::vector<VertexShaderOutput> *vertices, int i1, int i2, int i3, int avarCount, int pvarCount)
{
	m_avarCount = avarCount;
	m_pvarCount = pvarCount;
	m_vertices = vertices;

	m_indicesIn->clear();
	m_indicesOut->clear();
	
	m_indicesIn->push_back(i1);
	m_indicesIn->push_back(i2);
	m_indicesIn->push_back(i3);
}

void PolyClipper::clipToPlane(float a, float b, float c, float d)
{
	if (fullyClipped())
		return;

	m_indicesOut->clear();

	int idxPrev = (*m_indicesIn)[0];
	m_indicesIn->push_back(idxPrev);

	VertexShaderOutput *vPrev = &(*m_vertices)[idxPrev];
	float dpPrev = a * vPrev->x + b * vPrev->y + c * vPrev->z + d * vPrev->w;

	for (size_t i = 1; i < m_indicesIn->size(); ++i)
	{
		int idx = (*m_indicesIn)[i];
		VertexShaderOutput *v = &(*m_vertices)[idx];
		float dp = a * v->x + b * v->y + c * v->z + d * v->w;

		if (dpPrev >= 0)
			m_indicesOut->push_back(idxPrev);

		if (sgn(dp) != sgn(dpPrev))
		{
			float t = dp < 0 ? dpPrev / (dpPrev - dp) : -dpPrev / (dp - dpPrev);

			VertexShaderOutput vOut = Helper::interpolateVertex((*m_vertices)[idxPrev], (*m_vertices)[idx], t, m_avarCount, m_pvarCount);
			m_vertices->push_back(vOut);
			m_indicesOut->push_back((int)(m_vertices->size() - 1));
		}

		idxPrev = idx;
		dpPrev = dp;
	}

	std::swap(m_indicesIn, m_indicesOut);
}

} // end namespace swr