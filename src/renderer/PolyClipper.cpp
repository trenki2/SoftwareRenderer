#include "PolyClipper.h"

#include <algorithm>

namespace swr {

void PolyClipper::init(std::vector<VertexShaderOutput> *vertices, int i1, int i2, int i3, int attribCount)
{
	m_attribCount = attribCount;
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

			VertexShaderOutput vOut = Helper::interpolateVertex((*m_vertices)[idxPrev], (*m_vertices)[idx], t, m_attribCount);
			m_vertices->push_back(vOut);
			m_indicesOut->push_back((int)(m_vertices->size() - 1));
		}

		idxPrev = idx;
		dpPrev = dp;
	}

	std::swap(m_indicesIn, m_indicesOut);
}

} // end namespace swr