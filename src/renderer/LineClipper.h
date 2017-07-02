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
	void clipToPlane(float a, float b, float c, float d)
	{
		if (fullyClipped)
			return;

		float dp0 = a * m_v0.x + b * m_v0.y + c * m_v0.z + d * m_v0.w;
		float dp1 = a * m_v1.x + b * m_v1.y + c * m_v1.z + d * m_v1.w;

		bool dp0neg = dp0 < 0;
		bool dp1neg = dp1 < 0;

		if (dp0neg && dp1neg) {
			fullyClipped = true;
			return;
		} 
		
		if (dp0neg)
		{
			float t = -dp0 / (dp1 - dp0);
			t0 = std::max(t0, t);
		}
		else
		{
			float t = dp0 / (dp0 - dp1);
			t1 = std::min(t1, t);
		}
	}
};

} // end namespace swr