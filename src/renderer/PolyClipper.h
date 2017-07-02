#pragma once

#include "VertexConfig.h"

namespace swr {

class Helper {
public:
	static VertexShaderOutput interpolateVertex(const VertexShaderOutput &v0, const VertexShaderOutput &v1, float t, int attribCount)
	{
		VertexShaderOutput result;
		
		result.x = v0.x * (1.0f - t) + v1.x * t;
		result.y = v0.y * (1.0f - t) + v1.y * t;
		result.z = v0.z * (1.0f - t) + v1.z * t;
		result.w = v0.w * (1.0f - t) + v1.w * t;
		for (int i = 0; i < attribCount; ++i)
			result.avar[i] = v0.avar[i] * (1.0f - t) + v1.avar[i] * t;

		return result;
	}
};

class PolyClipper {
private:
	int m_attribCount;
	std::vector<int> *m_indicesIn;
	std::vector<int> *m_indicesOut;
	std::vector<VertexShaderOutput> *m_vertices;
	
public:
	PolyClipper()
	{
		m_indicesIn = new std::vector<int>();
		m_indicesOut = new std::vector<int>();
	}

	~PolyClipper()
	{
		delete m_indicesIn;
		delete m_indicesOut;
	}

	void init(std::vector<VertexShaderOutput> *vertices, int i1, int i2, int i3, int attribCount)
	{
		m_attribCount = attribCount;
		m_vertices = vertices;

		m_indicesIn->clear();
		m_indicesOut->clear();
		
		m_indicesIn->push_back(i1);
		m_indicesIn->push_back(i2);
		m_indicesIn->push_back(i3);
	}

	/// Clip the poly to the plane given by the formula a * x + b * y + c * z + d * w.
	void clipToPlane(float a, float b, float c, float d)
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