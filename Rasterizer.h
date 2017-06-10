#pragma once

#include "SDL.h"
#include <algorithm>

struct Vertex {
    float x;
    float y;
	float z;
	float w;

    float r;
    float g;
    float b;
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
        return (v > 0 || v == 0 && tie);
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

	ParameterEquation r;
	ParameterEquation g;
	ParameterEquation b;

	TriangleEquations(const Vertex &v0, const Vertex &v1, const Vertex &v2)
	{
		e0.init(v0, v1);
		e1.init(v1, v2);
		e2.init(v2, v0);

		area = 0.5f * (e0.c + e1.c + e2.c);

		// Cull backfacing triangles.
		if (area < 0)
			return;

		r.init(v0.r, v1.r, v2.r, e0, e1, e2, area);
		g.init(v0.g, v1.g, v2.g, e0, e1, e2, area);
		b.init(v0.b, v1.b, v2.b, e0, e1, e2, area);
	}
};

struct PixelData {
	float r;
	float g;
	float b;

	/// Initialize pixel data for the given pixel coordinates.
	void init(const TriangleEquations &eqn, float x, float y)
	{
		r = eqn.r.evaluate(x, y);
		g = eqn.g.evaluate(x, y);
		b = eqn.b.evaluate(x, y);
	}

	/// Step all the pixel data in the x direction.
	void stepX(const TriangleEquations &eqn)
	{
		r = eqn.r.stepX(r);
		g = eqn.g.stepX(g);
		b = eqn.b.stepX(b);
	}

	/// Step all the pixel data in the y direction.
	void stepY(const TriangleEquations &eqn)
	{
		r = eqn.r.stepY(r);
		g = eqn.g.stepY(g);
		b = eqn.b.stepY(b);
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

class Rasterizer {
private:
	const int BlockSize = 8;

    SDL_Surface *m_surface;

    int m_minX;
    int m_maxX;
    int m_minY;
    int m_maxY;

public:
    void setSurface(SDL_Surface *surface)
    {
        m_surface = surface;
    }

    void setScissorRect(int minX, int minY, int maxX, int maxY)
    {
        m_minX = minX;
        m_minY = minY;
        m_maxX = maxX;
        m_maxY = maxY;
    }

    /*
    * Set the pixel at (x, y) to the given value
    * NOTE: The surface must be locked before calling this!
    */
    void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
    {
        int bpp = surface->format->BytesPerPixel;

        /* Here p is the address to the pixel we want to set */
        Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

        switch(bpp) {
        case 1:
            *p = pixel;
            break;

        case 2:
            *(Uint16 *)p = pixel;
            break;

        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32 *)p = pixel;
            break;
        }
    }

	void drawTriangleSimple(const Vertex& v0, const Vertex &v1, const Vertex &v2)
	{
		// Compute edge equations.
		EdgeEquation e0;
		EdgeEquation e1;
		EdgeEquation e2;

		e0.init(v0, v1);
		e1.init(v1, v2);
		e2.init(v2, v0);

		float area = 0.5f * (e0.c + e1.c + e2.c);

		// Check if triangle is backfacing.
		if (area < 0)
			return;

		ParameterEquation r;
		ParameterEquation g;
		ParameterEquation b;

		r.init(v0.r, v1.r, v2.r, e0, e1, e2, area);
		g.init(v0.g, v1.g, v2.g, e0, e1, e2, area);
		b.init(v0.b, v1.b, v2.b, e0, e1, e2, area);

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

		// Add 0.5 to sample at pixel centers.
		for (float x = minX + 0.5f, xm = maxX + 0.5f; x <= xm; x += 1.0f)
			for (float y = minY + 0.5f, ym = maxY + 0.5f; y <= ym; y += 1.0f)
			{
				if (e0.test(x, y) && e1.test(x, y) && e2.test(x, y))
				{
					int rint = (int)(r.evaluate(x, y) * 255);
					int gint = (int)(g.evaluate(x, y) * 255);
					int bint = (int)(b.evaluate(x, y) * 255);
					Uint32 color = SDL_MapRGB(m_surface->format, rint, gint, bint);
					putpixel(m_surface, (int)x, (int)y, color);
				}
			}
	}

    void drawTriangle(const Vertex& v0, const Vertex &v1, const Vertex &v2)
    {
		// Compute triangle equations.
		TriangleEquations eqn(v0, v1, v2);

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

        // Add 0.5 to sample at pixel centers.
		// Add 0.5 to sample at pixel centers.
		for (float x = minX + 0.5f, xm = maxX + 0.5f; x <= xm; x += BlockSize)
		for (float y = minY + 0.5f, ym = maxY + 0.5f; y <= ym; y += BlockSize)
        {
			// Test if block is inside or outside triangle or touches it
			float s = (float)BlockSize - 1;

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
				rasterizeBlock<false>(eqn, x, y);
			else
				// Partially Covered
				rasterizeBlock<true>(eqn, x, y);
        }
    }

	template <bool TestEdges>
	void rasterizeBlock(const TriangleEquations &eqn, float x, float y)
	{
		PixelData po;
		po.init(eqn, x, y);

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
				if (!TestEdges || (eqn.e0.test(ei.ev0) && eqn.e1.test(ei.ev1) && eqn.e2.test(ei.ev2)))
				{
					int rint = (int)(pi.r * 255);
					int gint = (int)(pi.g * 255);
					int bint = (int)(pi.b * 255);
					Uint32 color = SDL_MapRGB(m_surface->format, rint, gint, bint);
					putpixel(m_surface, (int)xx, (int)yy, color);
				}

				pi.stepX(eqn);
				if (TestEdges)
					ei.stepX(eqn);
			}

			po.stepY(eqn);
			if (TestEdges)
				eo.stepY(eqn);
		}
	}
};