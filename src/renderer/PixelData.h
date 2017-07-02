#pragma once

/** @file */

#include "IRasterizer.h"
#include "TriangleEquations.h"

namespace swr {

/// PixelData passed to the pixel shader for display.
struct PixelData {
	int x; ///< The x coordinate.
	int y; ///< The y coordinate.

	float z; ///< The interpolated z value.
	float w; ///< The interpolated w value.
	float invw; ///< The interpolated 1 / w value.
	
	/// Affine variables.
	float avar[MaxAVars];
	
	/// Perspective variables.
	float pvar[MaxPVars];
	
	// Used internally.
	float pvarTemp[MaxPVars];

	PixelData() {}

	// Initialize pixel data for the given pixel coordinates.
	void init(const TriangleEquations &eqn, float x, float y, int aVarCount, int pVarCount, bool interpolateZ, bool interpolateW)
	{
		if (interpolateZ)
			z = eqn.z.evaluate(x, y);
		
		if (interpolateW || pVarCount > 0) 
		{
			invw = eqn.invw.evaluate(x, y);
			w = 1.0f / invw;
		}

		for (int i = 0; i < aVarCount; ++i)
			avar[i] = eqn.avar[i].evaluate(x, y);
		
		for (int i = 0; i < pVarCount; ++i)
		{
			pvarTemp[i] = eqn.pvar[i].evaluate(x, y);
			pvar[i] = pvarTemp[i] * w;
		}
	}

	// Step all the pixel data in the x direction.
	void stepX(const TriangleEquations &eqn, int aVarCount, int pVarCount, bool interpolateZ, bool interpolateW)
	{
		if (interpolateZ)
			z = eqn.z.stepX(z);
		
		if (interpolateW || pVarCount > 0) 
		{
			invw = eqn.invw.stepX(invw);
			w = 1.0f / invw;
		}

		for (int i = 0; i < aVarCount; ++i)
			avar[i] = eqn.avar[i].stepX(avar[i]);

		for (int i = 0; i < pVarCount; ++i)
		{
			pvarTemp[i] = eqn.pvar[i].stepX(pvarTemp[i]);			
			pvar[i] = pvarTemp[i] * w;
		}
	}

	// Step all the pixel data in the y direction.
	void stepY(const TriangleEquations &eqn, int aVarCount, int pVarCount, bool interpolateZ, bool interpolateW)
	{
		if (interpolateZ)
			z = eqn.z.stepY(z);
		
		if (interpolateW || pVarCount > 0) 
		{
			invw = eqn.invw.stepY(invw);
			w = 1.0f / invw;
		}

		for (int i = 0; i < aVarCount; ++i)
			avar[i] = eqn.avar[i].stepY(avar[i]);

		for (int i = 0; i < pVarCount; ++i)
		{
			pvarTemp[i] = eqn.pvar[i].stepY(pvarTemp[i]);			
			pvar[i] = pvarTemp[i] * w;
		}
	}
};

} // end namespace swr