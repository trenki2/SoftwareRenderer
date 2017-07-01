#pragma once

/** @file */ 

#include <algorithm>
#include "IRasterizer.h"

namespace swr {

const int BlockSize = 8;

struct EdgeEquation {
	float a;
	float b;
	float c;
	bool tie; 

	void init(const RasterizerVertex &v0, const RasterizerVertex &v1)
	{
		a = v0.y - v1.y;
		b = v1.x - v0.x;
		c = - (a * (v0.x + v1.x) + b * (v0.y + v1.y)) / 2;
		tie = a != 0 ? a > 0 : b > 0;
	}

	// Evaluate the edge equation for the given point.
	float evaluate(float x, float y) const
	{
		return a * x + b * y + c;
	}

	// Test if the given point is inside the edge.
	bool test(float x, float y) const
	{
		return test(evaluate(x, y));
	}

	// Test for a given evaluated value.
	bool test(float v) const
	{
		return (v > 0 || (v == 0 && tie));
	}

	// Step the equation value v to the x direction
	float stepX(float v) const
	{
		return v + a;
	}

	// Step the equation value v to the x direction
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	// Step the equation value v to the y direction
	float stepY(float v) const
	{
		return v + b;
	}

	// Step the equation value vto the y direction
	float stepY(float v, float stepSize) const
	{
		return v + b * stepSize;
	}
};

struct ParameterEquation {
	float a;
	float b;
	float c;

	void init(
		float p0,
		float p1, 
		float p2, 
		const EdgeEquation &e0, 
		const EdgeEquation &e1, 
		const EdgeEquation &e2, 
		float factor)
	{
		a = factor * (p0 * e0.a + p1 * e1.a + p2 * e2.a);
		b = factor * (p0 * e0.b + p1 * e1.b + p2 * e2.b);
		c = factor * (p0 * e0.c + p1 * e1.c + p2 * e2.c);
	}

	// Evaluate the parameter equation for the given point.
	float evaluate(float x, float y) const
	{
		return a * x + b * y + c;
	}

	// Step parameter value v in x direction.
	float stepX(float v) const
	{
		return v + a;
	}

	// Step parameter value v in x direction.
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	// Step parameter value v in y direction.
	float stepY(float v) const
	{
		return v + b;
	}

	// Step parameter value v in y direction.
	float stepY(float v, float stepSize) const
	{
		return v + b * stepSize;
	}
};

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

struct EdgeData {
	float ev0;
	float ev1;
	float ev2;

	// Initialize the edge data values.
	void init(const TriangleEquations &eqn, float x, float y)
	{
		ev0 = eqn.e0.evaluate(x, y);
		ev1 = eqn.e1.evaluate(x, y);
		ev2 = eqn.e2.evaluate(x, y);
	}

	// Step the edge values in the x direction.
	void stepX(const TriangleEquations &eqn)
	{
		ev0 = eqn.e0.stepX(ev0);
		ev1 = eqn.e1.stepX(ev1);
		ev2 = eqn.e2.stepX(ev2);
	}
	
	// Step the edge values in the x direction.
	void stepX(const TriangleEquations &eqn, float stepSize)
	{
		ev0 = eqn.e0.stepX(ev0, stepSize);
		ev1 = eqn.e1.stepX(ev1, stepSize);
		ev2 = eqn.e2.stepX(ev2, stepSize);
	}

	// Step the edge values in the y direction.
	void stepY(const TriangleEquations &eqn)
	{
		ev0 = eqn.e0.stepY(ev0);
		ev1 = eqn.e1.stepY(ev1);
		ev2 = eqn.e2.stepY(ev2);
	}

	// Step the edge values in the y direction.
	void stepY(const TriangleEquations &eqn, float stepSize)
	{
		ev0 = eqn.e0.stepY(ev0, stepSize);
		ev1 = eqn.e1.stepY(ev1, stepSize);
		ev2 = eqn.e2.stepY(ev2, stepSize);
	}

	// Test for triangle containment.
	bool test(const TriangleEquations &eqn)
	{
		return eqn.e0.test(ev0) && eqn.e1.test(ev1) && eqn.e2.test(ev2);
	}
};

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
		if (Derived::InterpolateW) pi.invw = po.invw;
		for (int i = 0; i < Derived::AVarCount; ++i)
			pi.avar[i] = po.avar[i];
		return pi;
	}
};

class DummyPixelShader : public PixelShaderBase<DummyPixelShader> {};

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
		setPixelShader<DummyPixelShader>();
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
		if (PixelShader::InterpolateW) p.invw = 1.0f / v.w;
		for (int i = 0; i < PixelShader::AVarCount; ++i)
			p.avar[i] = v.avar[i];
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

} // end namespace swr