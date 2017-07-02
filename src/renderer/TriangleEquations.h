#pragma once

#include "IRasterizer.h"
#include "ParameterEquation.h"

namespace swr {

struct TriangleEquations {
	float area2;

	EdgeEquation e0;
	EdgeEquation e1;
	EdgeEquation e2;

	ParameterEquation z;
	ParameterEquation invw;
	ParameterEquation avar[MaxAVars];
	ParameterEquation pvar[MaxPVars];

	TriangleEquations(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2, int aVarCount, int pVarCount)
	{
		e0.init(v1, v2);
		e1.init(v2, v0);
		e2.init(v0, v1);

		area2 = e0.c + e1.c + e2.c;

		// Cull backfacing triangles.
		if (area2 <= 0)
			return;
		
		float factor = 1.0f / area2;
		z.init(v0.z, v1.z, v2.z, e0, e1, e2, factor);

		float invw0 = 1.0f / v0.w;
		float invw1 = 1.0f / v1.w;
		float invw2 = 1.0f / v2.w;

		invw.init(invw0, invw1, invw2, e0, e1, e2, factor);
		for (int i = 0; i < aVarCount; ++i)
			avar[i].init(v0.avar[i], v1.avar[i], v2.avar[i], e0, e1, e2, factor);
		for (int i = 0; i < pVarCount; ++i)
			pvar[i].init(v0.pvar[i] * invw0, v1.pvar[i] * invw1, v2.pvar[i] * invw2, e0, e1, e2, factor);
	}
};

} // end namespace swr