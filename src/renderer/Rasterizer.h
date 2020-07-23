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

#include <algorithm>

#include "IRasterizer.h"
#include "EdgeEquation.h"
#include "ParameterEquation.h"
#include "TriangleEquations.h"
#include "PixelData.h"
#include "EdgeData.h"
#include "PixelShaderBase.h"

namespace swr {

#pragma warning (push)
#pragma warning (disable: 6294 6201)

/// Rasterizer mode.
enum class RasterMode {
	Span,
	Block,
	Adaptive
};

/// Rasterizer main class.
class Rasterizer : public IRasterizer {
private:
	int m_minX;
	int m_maxX;
	int m_minY;
	int m_maxY;

	RasterMode rasterMode;

	void (Rasterizer::*m_triangleFunc)(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const;
	void (Rasterizer::*m_lineFunc)(const RasterizerVertex &v0, const RasterizerVertex &v1) const;
	void (Rasterizer::*m_pointFunc)(const RasterizerVertex &v) const;

public:
	/// Constructor.
	Rasterizer()
	{
		setRasterMode(RasterMode::Span);
		setScissorRect(0, 0, 0, 0);
		setPixelShader<NullPixelShader>();
	}

	/// Set the raster mode. The default is RasterMode::Span.
	void setRasterMode(RasterMode mode)
	{
		rasterMode = mode;
	}

	/// Set the scissor rectangle.
	void setScissorRect(int x, int y, int width, int height)
	{
		m_minX = x;
		m_minY = y;
		m_maxX = x + width;
		m_maxY = y + height;
	}
	
	/// Set the pixel shader.
	template <class PixelShader>
	void setPixelShader()
	{
		m_triangleFunc = &Rasterizer::drawTriangleModeTemplate<PixelShader>;
		m_lineFunc = &Rasterizer::drawLineTemplate<PixelShader>;
		m_pointFunc = &Rasterizer::drawPointTemplate<PixelShader>;	
	}

	/// Draw a single point.
	void drawPoint(const RasterizerVertex &v) const
	{
		(this->*m_pointFunc)(v);
	}

	/// Draw a single line.
	void drawLine(const RasterizerVertex &v0, const RasterizerVertex &v1) const
	{
		(this->*m_lineFunc)(v0, v1);
	}
	
	/// Draw a single triangle.
	void drawTriangle(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		(this->*m_triangleFunc)(v0, v1, v2);
	}

	void drawPointList(const RasterizerVertex *vertices, const int *indices, size_t indexCount) const
	{
		for (size_t i = 0; i < indexCount; ++i) {
			if (indices[i] == -1)
				continue;
			drawPoint(vertices[indices[i]]);
		}
	}

	void drawLineList(const RasterizerVertex *vertices, const int *indices, size_t indexCount) const
	{
		for (size_t i = 0; i + 2 <= indexCount; i += 2) {
			if (indices[i] == -1)
				continue;
			drawLine(vertices[indices[i]], vertices[indices[i + 1]]);
		}
	}

	void drawTriangleList(const RasterizerVertex *vertices, const int *indices, size_t indexCount) const
	{
		for (size_t i = 0; i + 3 <= indexCount; i += 3) {
			if (indices[i] == -1)
				continue;
			drawTriangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
		}
	}

private:
	bool scissorTest(float x, float y) const
	{
		return (x >= m_minX && x < m_maxX && y >= m_minY && y < m_maxY);
	}

	template <class PixelShader>
	void drawPointTemplate(const RasterizerVertex &v) const
	{
		// Check scissor rect
		if (!scissorTest(v.x, v.y))
			return;

		PixelData p = pixelDataFromVertex<PixelShader>(v);
		PixelShader::drawPixel(p);
	}

	template<class PixelShader>
	PixelData pixelDataFromVertex(const RasterizerVertex & v) const
	{
		PixelData p;
		p.x = (int)v.x;
		p.y = (int)v.y;
		if (PixelShader::InterpolateZ) p.z = v.z;
		if (PixelShader::InterpolateW) { p.w = v.w; p.invw = 1.0f / v.w; }
		for (int i = 0; i < PixelShader::AVarCount; ++i)
			p.avar[i] = v.avar[i];
		for (int i = 0; i < PixelShader::PVarCount; ++i)
			p.pvar[i] = v.pvar[i];
		return p;
	}
	
	template <class PixelShader>
	void drawLineTemplate(const RasterizerVertex &v0, const RasterizerVertex &v1) const
	{
		int adx = std::abs((int)v1.x - (int)v0.x);
		int ady = std::abs((int)v1.y - (int)v0.y);
		int steps = std::max(adx, ady);

		RasterizerVertex step = computeVertexStep<PixelShader>(v0, v1, steps);

		RasterizerVertex v = v0;
		while (steps-- > 0)
		{
			PixelData p = pixelDataFromVertex<PixelShader>(v);

			if (scissorTest(v.x, v.y))
				PixelShader::drawPixel(p);
			
			stepVertex<PixelShader>(v, step);
		}
	}

	template<class PixelShader>
	void stepVertex(RasterizerVertex &v, RasterizerVertex &step) const
	{
		v.x += step.x;
		v.y += step.y;
		if (PixelShader::InterpolateZ) v.z += step.z;
		if (PixelShader::InterpolateW) v.w += step.w;
		for (int i = 0; i < PixelShader::AVarCount; ++i)
			v.avar[i] += step.avar[i];
		for (int i = 0; i < PixelShader::PVarCount; ++i)
			v.pvar[i] += step.pvar[i];
	}

	template<class PixelShader>
	RasterizerVertex computeVertexStep(const RasterizerVertex &v0, const RasterizerVertex &v1, int adx) const
	{
		RasterizerVertex step;
		step.x = (v1.x - v0.x) / adx;
		step.y = (v1.y - v0.y) / adx;
		if (PixelShader::InterpolateZ) step.z = (v1.z - v0.z) / adx;
		if (PixelShader::InterpolateW) step.w = (v1.w - v0.w) / adx;
		for (int i = 0; i < PixelShader::AVarCount; ++i)
			step.avar[i] = (v1.avar[i] - v0.avar[i]) / adx;
		for (int i = 0; i < PixelShader::PVarCount; ++i)
			step.pvar[i] = (v1.pvar[i] - v0.pvar[i]) / adx;
		return step;
	}

	template <class PixelShader>
	void drawTriangleBlockTemplate(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		// Compute triangle equations.
		TriangleEquations eqn(v0, v1, v2, PixelShader::AVarCount, PixelShader::PVarCount);

		// Check if triangle is backfacing.
		if (eqn.area2 <= 0)
			return;

		// Compute triangle bounding box.
		int minX = (int)std::min(std::min(v0.x, v1.x), v2.x);
		int maxX = (int)std::max(std::max(v0.x, v1.x), v2.x);
		int minY = (int)std::min(std::min(v0.y, v1.y), v2.y);
		int maxY = (int)std::max(std::max(v0.y, v1.y), v2.y);

		// Clip to scissor rect.
		minX = std::max(minX, m_minX);
		maxX = std::min(maxX, m_maxX);
		minY = std::max(minY, m_minY);
		maxY = std::min(maxY, m_maxY);

		// Round to block grid.
		minX = minX & ~(BlockSize - 1);
		maxX = maxX & ~(BlockSize - 1);
		minY = minY & ~(BlockSize - 1);
		maxY = maxY & ~(BlockSize - 1);

		float s = BlockSize - 1;

		int stepsX = (maxX - minX) / BlockSize + 1;
		int stepsY = (maxY - minY) / BlockSize + 1;

		#pragma omp parallel for
		for (int i = 0; i < stepsX * stepsY; ++i)
		{
			int sx = i % stepsX;
			int sy = i / stepsX;

			// Add 0.5 to sample at pixel centers.
			int x = minX + sx * BlockSize;
			int y = minY + sy * BlockSize;

			float xf = x + 0.5f;
			float yf = y + 0.5f;

			// Test if block is inside or outside triangle or touches it.
			EdgeData e00; e00.init(eqn, xf, yf);
			EdgeData e01 = e00; e01.stepY(eqn, s);
			EdgeData e10 = e00; e10.stepX(eqn, s);
			EdgeData e11 = e01; e11.stepX(eqn, s);

			bool e00_0 = eqn.e0.test(e00.ev0), e00_1 = eqn.e1.test(e00.ev1), e00_2 = eqn.e2.test(e00.ev2), e00_all = e00_0 && e00_1 && e00_2;
			bool e01_0 = eqn.e0.test(e01.ev0), e01_1 = eqn.e1.test(e01.ev1), e01_2 = eqn.e2.test(e01.ev2), e01_all = e01_0 && e01_1 && e01_2;
			bool e10_0 = eqn.e0.test(e10.ev0), e10_1 = eqn.e1.test(e10.ev1), e10_2 = eqn.e2.test(e10.ev2), e10_all = e10_0 && e10_1 && e10_2;
			bool e11_0 = eqn.e0.test(e11.ev0), e11_1 = eqn.e1.test(e11.ev1), e11_2 = eqn.e2.test(e11.ev2), e11_all = e11_0 && e11_1 && e11_2;

			int result = e00_all + e01_all + e10_all + e11_all;

			// Potentially all out.
			if (result == 0)
			{
				// Test for special case.
				bool e00Same = e00_0 == e00_1 == e00_2;
				bool e01Same = e01_0 == e01_1 == e01_2;
				bool e10Same = e10_0 == e10_1 == e10_2;
				bool e11Same = e11_0 == e11_1 == e11_2;

				if (!e00Same || !e01Same || !e10Same || !e11Same)
					PixelShader::template drawBlock<true>(eqn, x, y);
			}
			else if (result == 4)
			{
				// Fully Covered.
				PixelShader::template drawBlock<false>(eqn, x, y);
			}
			else
			{
				// Partially Covered.
				PixelShader::template drawBlock<true>(eqn, x, y);
			}
		}
	}

	template <class PixelShader>
	void drawTriangleSpanTemplate(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		// Compute triangle equations.
		TriangleEquations eqn(v0, v1, v2, PixelShader::AVarCount, PixelShader::PVarCount);

		// Check if triangle is backfacing.
		if (eqn.area2 <= 0)
			return;

		const RasterizerVertex *t = &v0;
		const RasterizerVertex *m = &v1;
		const RasterizerVertex *b = &v2;

		// Sort vertices from top to bottom.
		if (t->y > m->y) std::swap(t, m);
		if (m->y > b->y) std::swap(m, b);
		if (t->y > m->y) std::swap(t, m);

		float dy = (b->y - t->y);
		float iy = (m->y - t->y);

		if (m->y == t->y)
		{
			const RasterizerVertex *l = m, *r = t;
			if (l->x > r->x) std::swap(l, r);
			drawTopFlatTriangle<PixelShader>(eqn, *l, *r, *b);
		}
		else if (m->y == b->y)
		{
			const RasterizerVertex *l = m, *r = b;
			if (l->x > r->x) std::swap(l, r);
			drawBottomFlatTriangle<PixelShader>(eqn, *t, *l, *r);
		} 
		else
		{
			RasterizerVertex v4;
			v4.y = m->y;
			v4.x = t->x + ((b->x - t->x) / dy) * iy;
			if (PixelShader::InterpolateZ) v4.z = t->z + ((b->z - t->z) / dy) * iy;
			if (PixelShader::InterpolateW) v4.w = t->w + ((b->w - t->w) / dy) * iy;
			for (int i = 0; i < PixelShader::AVarCount; ++i)
				v4.avar[i] = t->avar[i] + ((b->avar[i] - t->avar[i]) / dy) * iy;

			const RasterizerVertex *l = m, *r = &v4;
			if (l->x > r->x) std::swap(l, r);

			drawBottomFlatTriangle<PixelShader>(eqn, *t, *l, *r);
			drawTopFlatTriangle<PixelShader>(eqn, *l, *r, *b);
		}
	}

	template <class PixelShader>
	void drawBottomFlatTriangle(const TriangleEquations &eqn, const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		float invslope1 = (v1.x - v0.x) / (v1.y - v0.y);
		float invslope2 = (v2.x - v0.x) / (v2.y - v0.y);

		//float curx1 = v0.x;
		//float curx2 = v0.x;

		#pragma omp parallel for
		for (int scanlineY = int(v0.y + 0.5f); scanlineY < int(v1.y + 0.5f); scanlineY++)
		{
			float dy = (scanlineY - v0.y) + 0.5f;
			float curx1 = v0.x + invslope1 * dy + 0.5f;
			float curx2 = v0.x + invslope2 * dy + 0.5f;

			// Clip to scissor rect
			int xl = std::max(m_minX, (int)curx1);
			int xr = std::min(m_maxX, (int)curx2);

			PixelShader::drawSpan(eqn, xl, scanlineY, xr);
			
			// curx1 += invslope1;
			// curx2 += invslope2;
		}
	}

	template <class PixelShader>
	void drawTopFlatTriangle(const TriangleEquations &eqn, const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		float invslope1 = (v2.x - v0.x) / (v2.y - v0.y);
		float invslope2 = (v2.x - v1.x) / (v2.y - v1.y);

		// float curx1 = v2.x;
		// float curx2 = v2.x;

		#pragma omp parallel for
		for (int scanlineY = int(v2.y - 0.5f); scanlineY > int(v0.y - 0.5f); scanlineY--)
		{
			float dy = (scanlineY - v2.y) + 0.5f;
			float curx1 = v2.x + invslope1 * dy + 0.5f;
			float curx2 = v2.x + invslope2 * dy + 0.5f;

			// Clip to scissor rect
			int xl = std::max(m_minX, (int)curx1);
			int xr = std::min(m_maxX, (int)curx2);

			PixelShader::drawSpan(eqn, xl, scanlineY, xr);
			// curx1 -= invslope1;
			// curx2 -= invslope2;
		}
	}

	template <class PixelShader>
	void drawTriangleAdaptiveTemplate(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		// Compute triangle bounding box.
		float minX = (float)std::min(std::min(v0.x, v1.x), v2.x);
		float maxX = (float)std::max(std::max(v0.x, v1.x), v2.x);
		float minY = (float)std::min(std::min(v0.y, v1.y), v2.y);
		float maxY = (float)std::max(std::max(v0.y, v1.y), v2.y);

		float orient = (maxX - minX) / (maxY - minY);

		if (orient > 0.4 && orient < 1.6)
			drawTriangleBlockTemplate<PixelShader>(v0, v1, v2);
		else
			drawTriangleSpanTemplate<PixelShader>(v0, v1, v2);
	}

	template <class PixelShader>
	void drawTriangleModeTemplate(const RasterizerVertex &v0, const RasterizerVertex &v1, const RasterizerVertex &v2) const
	{
		switch (rasterMode)
		{
			case RasterMode::Span:
				drawTriangleSpanTemplate<PixelShader>(v0, v1, v2);
				break;
			case RasterMode::Block:
				drawTriangleBlockTemplate<PixelShader>(v0, v1, v2);
				break;
			case RasterMode::Adaptive:
				drawTriangleAdaptiveTemplate<PixelShader>(v0, v1, v2);
				break;
		}
	}
};

#pragma warning (pop)

} // end namespace swr