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