#pragma once

#include "SDL.h"
#include <algorithm>

const int BlockSize = 8;
const int MaxVar = 32;

struct Vertex {
    float x;
    float y;
	float z;
	float w;

    float var[MaxVar];
};

struct EdgeEquation {
    float a;
    float b;
    float c;
    bool tie; 

    void init(const Vertex &v0, const Vertex &v1)
    {
        a = v0.y - v1.y;
        b = v1.x - v0.x;
        c = - (a * (v0.x + v1.x) + b * (v0.y + v1.y)) / 2;
        tie = a != 0 ? a > 0 : b > 0;
    }

    /// Evaluate the edge equation for the given point.
    float evaluate(float x, float y) const
    {
        return a * x + b * y + c;
    }

    /// Test if the given point is inside the edge.
    bool test(float x, float y) const
    {
        return test(evaluate(x, y));
    }

    /// Test for a given evaluated value.
    bool test(float v) const
    {
        return (v > 0 || (v == 0 && tie));
    }

	/// Step the equation value v to the x direction
	float stepX(float v) const
	{
		return v + a;
	}

	/// Step the equation value v to the x direction
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	/// Step the equation value v to the y direction
	float stepY(float v) const
	{
		return v + b;
	}

	/// Step the equation value vto the y direction
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
        float area)
    {
        float factor = 1.0f / (2.0f * area);

        a = factor * (p0 * e0.a + p1 * e1.a + p2 * e2.a);
        b = factor * (p0 * e0.b + p1 * e1.b + p2 * e2.b);
        c = factor * (p0 * e0.c + p1 * e1.c + p2 * e2.c);
    }

    /// Evaluate the parameter equation for the given point.
    float evaluate(float x, float y) const
    {
        return a * x + b * y + c;
    }

	/// Step parameter value v in x direction.
	float stepX(float v) const
	{
		return v + a;
	}

	/// Step parameter value v in x direction.
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	/// Step parameter value v in y direction.
	float stepY(float v) const
	{
		return v + b;
	}

	/// Step parameter value v in y direction.
	float stepY(float v, float stepSize) const
	{
		return v + b * stepSize;
	}
};

struct TriangleEquations {
	float area;

	EdgeEquation e0;
	EdgeEquation e1;
	EdgeEquation e2;

	ParameterEquation z;
	ParameterEquation w;
	ParameterEquation var[MaxVar];

	TriangleEquations(const Vertex &v0, const Vertex &v1, const Vertex &v2, int varCount)
	{
		e0.init(v0, v1);
		e1.init(v1, v2);
		e2.init(v2, v0);

		area = 0.5f * (e0.c + e1.c + e2.c);

		// Cull backfacing triangles.
		if (area < 0)
			return;
		
		z.init(v0.z, v1.z, v2.z, e0, e1, e2, area);
		w.init(v0.w, v1.w, v2.w, e0, e1, e2, area);
		for (int i = 0; i < varCount; ++i)
			var[i].init(v0.var[i], v1.var[i], v2.var[i], e0, e1, e2, area);
	}
};

struct PixelData {
	float x;
	float y;
	float z;
	float w;

	float var[MaxVar];

	/// Initialize pixel data for the given pixel coordinates.
	void init(const TriangleEquations &eqn, float x, float y, int varCount, bool interpolateZ, bool interpolateW)
	{
		if (interpolateZ) z = eqn.z.evaluate(x, y);
		if (interpolateW) w = eqn.w.evaluate(x, y);
		for (int i = 0; i < varCount; ++i)
			var[i] = eqn.var[i].evaluate(x, y);
	}

	/// Step all the pixel data in the x direction.
	void stepX(const TriangleEquations &eqn, int varCount, bool interpolateZ, bool interpolateW)
	{
		if (interpolateZ) z = eqn.z.stepX(z);
		if (interpolateW) w = eqn.w.stepX(w);
		for (int i = 0; i < varCount; ++i)
			var[i] = eqn.var[i].stepX(var[i]);
	}

	/// Step all the pixel data in the y direction.
	void stepY(const TriangleEquations &eqn, int varCount, bool interpolateZ, bool interpolateW)
	{
		if (interpolateZ) z = eqn.z.stepY(z);
		if (interpolateW) w = eqn.w.stepY(w);
		for (int i = 0; i < varCount; ++i)
			var[i] = eqn.var[i].stepY(var[i]);
	}
};

struct EdgeData {
	float ev0;
	float ev1;
	float ev2;

	/// Initialize the edge data values.
	void init(const TriangleEquations &eqn, float x, float y)
	{
		ev0 = eqn.e0.evaluate(x, y);
		ev1 = eqn.e1.evaluate(x, y);
		ev2 = eqn.e2.evaluate(x, y);
	}

	/// Step the edge values in the x direction.
	void stepX(const TriangleEquations &eqn)
	{
		ev0 = eqn.e0.stepX(ev0);
		ev1 = eqn.e1.stepX(ev1);
		ev2 = eqn.e2.stepX(ev2);
	}
	
	/// Step the edge values in the x direction.
	void stepX(const TriangleEquations &eqn, float stepSize)
	{
		ev0 = eqn.e0.stepX(ev0, stepSize);
		ev1 = eqn.e1.stepX(ev1, stepSize);
		ev2 = eqn.e2.stepX(ev2, stepSize);
	}

	/// Step the edge values in the y direction.
	void stepY(const TriangleEquations &eqn)
	{
		ev0 = eqn.e0.stepY(ev0);
		ev1 = eqn.e1.stepY(ev1);
		ev2 = eqn.e2.stepY(ev2);
	}

	/// Step the edge values in the y direction.
	void stepY(const TriangleEquations &eqn, float stepSize)
	{
		ev0 = eqn.e0.stepY(ev0, stepSize);
		ev1 = eqn.e1.stepY(ev1, stepSize);
		ev2 = eqn.e2.stepY(ev2, stepSize);
	}

	/// Test for triangle containment.
	bool test(const TriangleEquations &eqn)
	{
		return eqn.e0.test(ev0) && eqn.e1.test(ev1) && eqn.e2.test(ev2);
	}
};

template <class Derived>
class PixelShaderBase {
public:
	/// Tells the rasterizer to interpolate the z component.
	static const int InterpolateZ = false;

	/// Tells the rasterizer to interpolate the w component.
	static const int InterpolateW = false;

	/// Tells the rasterizer how many vars to interpolate;
	static const int VarCount = 0;

	template <bool TestEdges>
	static void rasterizeBlock(const TriangleEquations &eqn, float x, float y)
	{
		PixelData po;
		po.init(eqn, x, y, Derived::VarCount, Derived::InterpolateZ, Derived::InterpolateW);

		EdgeData eo;
		if (TestEdges)
			eo.init(eqn, x, y);

		for (float yy = y; yy < y + BlockSize; yy += 1.0f)
		{
			PixelData pi = po;

			EdgeData ei;
			if (TestEdges)
				ei = eo;

			for (float xx = x; xx < x + BlockSize; xx += 1.0f)
			{
				if (!TestEdges || ei.test(eqn))
				{
					pi.x = xx;
					pi.y = yy;
					Derived::drawPixel(pi);
				}

				pi.stepX(eqn, Derived::VarCount, Derived::InterpolateZ, Derived::InterpolateW);
				if (TestEdges)
					ei.stepX(eqn);
			}

			po.stepY(eqn, Derived::VarCount, Derived::InterpolateZ, Derived::InterpolateW);
			if (TestEdges)
				eo.stepY(eqn);
		}
	}

	/// This is called per pixel.
	static void drawPixel(const PixelData &p)
	{

	}
};

class DummyShader : public PixelShaderBase<DummyShader> {};

class Rasterizer {
private:
    int m_minX;
    int m_maxX;
    int m_minY;
    int m_maxY;

	void (Rasterizer::*m_triangleFunc)(const Vertex& v0, const Vertex &v1, const Vertex &v2) const;
	void (Rasterizer::*m_pointFunc)(const Vertex& v) const;

public:
	Rasterizer()
	{
		setScissorRect(0, 0, 0, 0);
		setPixelShader<DummyShader>();
	}

    void setScissorRect(int minX, int minY, int maxX, int maxY)
    {
        m_minX = minX;
        m_minY = minY;
        m_maxX = maxX;
        m_maxY = maxY;
    }
	
	template <class PixelShader>
	void setPixelShader()
	{
		m_triangleFunc = &Rasterizer::drawTriangleTemplate<PixelShader>;
		m_pointFunc = &Rasterizer::drawPointTemplate<PixelShader>;
	}

	void drawTriangle(const Vertex& v0, const Vertex &v1, const Vertex &v2) const
	{
		(this->*m_triangleFunc)(v0, v1, v2);
	}

	void drawPoint(const Vertex &v) const
	{
		(this->*m_pointFunc)(v);
	}

private:
	template <class PixelShader>
    void drawTriangleTemplate(const Vertex& v0, const Vertex &v1, const Vertex &v2) const
    {
		// Compute triangle equations.
		TriangleEquations eqn(v0, v1, v2, PixelShader::VarCount);

		// Check if triangle is backfacing.
		if (eqn.area < 0)
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

		float s = (float)BlockSize - 1;

        // Add 0.5 to sample at pixel centers.
		for (float x = minX + 0.5f, xm = maxX + 0.5f; x <= xm; x += BlockSize)
		for (float y = minY + 0.5f, ym = maxY + 0.5f; y <= ym; y += BlockSize)
        {
			// Test if block is inside or outside triangle or touches it.
			EdgeData e00; e00.init(eqn, x, y);
			EdgeData e01 = e00; e01.stepY(eqn, s);
			EdgeData e10 = e00; e10.stepX(eqn, s);
			EdgeData e11 = e01; e11.stepX(eqn, s);

			int result = e00.test(eqn) + e01.test(eqn) + e10.test(eqn) + e11.test(eqn);

			// All out.
			if (result == 0)
				continue;

			if (result == 4)
				// Fully Covered
				PixelShader::template rasterizeBlock<false>(eqn, x, y);
			else
				// Partially Covered
				PixelShader::template rasterizeBlock<true>(eqn, x, y);
        }
    }

	template <class PixelShader>
	void drawPointTemplate(const Vertex &v) const
	{
		// Check scissor rect
		if (v.x < m_minX || v.x > m_maxX || v.y < m_minY || v.y > m_maxY)
			return;

		PixelData p;

		p.x = v.x;
		p.y = v.y;
		p.z = v.z;
		p.w = v.w;
		for (int i = 0; i < PixelShader::VarCount; ++i)
			p.var[i] = v.var[i];

		PixelShader::drawPixel(p);
	}
};