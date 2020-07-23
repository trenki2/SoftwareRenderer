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

/** @file */

#include "TriangleEquations.h"
#include "PixelData.h"

namespace swr {

#pragma warning (push)
#pragma warning (disable: 6201 6294) 

/// Pixel shader base class.
/** Derive your own pixel shaders from this class and redefine the static
  variables to match your pixel shader requirements. */
template <class Derived>
class PixelShaderBase {
public:
	/// Tells the rasterizer to interpolate the z component.
	static const int InterpolateZ = false;

	/// Tells the rasterizer to interpolate the w component.
	static const int InterpolateW = false;

	/// Tells the rasterizer how many affine vars to interpolate.
	static const int AVarCount = 0;

	/// Tells the rasterizer how many perspective vars to interpolate.
	static const int PVarCount = 0;

	template <bool TestEdges>
	static void drawBlock(const TriangleEquations &eqn, int x, int y)
	{
		float xf = x + 0.5f;
		float yf = y + 0.5f;

		PixelData po;
		po.init(eqn, xf, yf, Derived::AVarCount, Derived::PVarCount, Derived::InterpolateZ, Derived::InterpolateW);

		EdgeData eo;
		if (TestEdges)
			eo.init(eqn, xf, yf);

		for (int yy = y; yy < y + BlockSize; yy++)
		{
			PixelData pi = copyPixelData(po);

			EdgeData ei;
			if (TestEdges)
				ei = eo;

			for (int xx = x; xx < x + BlockSize; xx++)
			{
				if (!TestEdges || ei.test(eqn))
				{
					pi.x = xx;
					pi.y = yy;
					Derived::drawPixel(pi);
				}

				pi.stepX(eqn, Derived::AVarCount, Derived::PVarCount, Derived::InterpolateZ, Derived::InterpolateW);
				if (TestEdges)
					ei.stepX(eqn);
			}

			po.stepY(eqn, Derived::AVarCount, Derived::PVarCount, Derived::InterpolateZ, Derived::InterpolateW);
			if (TestEdges)
				eo.stepY(eqn);
		}
	}

	static void drawSpan(const TriangleEquations &eqn, int x, int y, int x2)
	{
		float xf = x + 0.5f;
		float yf = y + 0.5f;

		PixelData p;
		p.y = y;
		p.init(eqn, xf, yf, Derived::AVarCount, Derived::PVarCount, Derived::InterpolateZ, Derived::InterpolateW);

		while (x < x2)
		{
			p.x = x;
			Derived::drawPixel(p);
			p.stepX(eqn, Derived::AVarCount, Derived::PVarCount, Derived::InterpolateZ, Derived::InterpolateW);
			x++;
		}
	}

	/// This is called per pixel. 
	/** Implement this in your derived class to display single pixels. */
	static void drawPixel(const PixelData &p)
	{

	}

protected:
	static PixelData copyPixelData(PixelData &po)
	{
		PixelData pi;
		if (Derived::InterpolateZ) pi.z = po.z;
		if (Derived::InterpolateW) { pi.w = po.w; pi.invw = po.invw; }
		for (int i = 0; i < Derived::AVarCount; ++i)
			pi.avar[i] = po.avar[i];
		for (int i = 0; i < Derived::PVarCount; ++i) 
		{
			pi.pvarTemp[i] = po.pvarTemp[i];
			pi.pvar[i] = po.pvar[i];
		}
		return pi;
	}
};

class NullPixelShader : public PixelShaderBase<NullPixelShader> {};

#pragma warning (pop)

} // end namespace swr