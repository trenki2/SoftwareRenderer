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

	/// Step the equation value to the x direction
	float stepX(float v) const
	{
		return v + a;
	}

	/// Step the equation value to the x direction
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	/// Step the equation value to the y direction
	float stepY(float v) const
	{
		return v + b;
	}

	/// Step the equation value to the y direction
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

	/// Step parameter value in x direction.
	float stepX(float v) const
	{
		return v + a;
	}

	/// Step parameter value in x direction.
	float stepX(float v, float stepSize) const
	{
		return v + a * stepSize;
	}

	/// Step parameter value in y direction.
	float stepY(float v) const
	{
		return v + b;
	}

	/// Step parameter value in y direction.
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

			float e0_00 = eqn.e0.evaluate(x, y);
			float e0_10 = eqn.e0.stepX(e0_00, s);
			float e0_01 = eqn.e0.stepY(e0_00, s);
			float e0_11 = eqn.e0.stepX(e0_01, s);

			float e1_00 = eqn.e1.evaluate(x, y);
			float e1_10 = eqn.e1.stepX(e1_00, s);
			float e1_01 = eqn.e1.stepY(e1_00, s);
			float e1_11 = eqn.e1.stepX(e1_01, s);

			float e2_00 = eqn.e2.evaluate(x, y);
			float e2_10 = eqn.e2.stepX(e2_00, s);
			float e2_01 = eqn.e2.stepY(e2_00, s);
			float e2_11 = eqn.e2.stepX(e2_01, s);
			
			int in0 = eqn.e0.test(e0_00) && eqn.e1.test(e1_00) && eqn.e2.test(e2_00);
			int in1 = eqn.e0.test(e0_10) && eqn.e1.test(e1_10) && eqn.e2.test(e2_10);
			int in2 = eqn.e0.test(e0_01) && eqn.e1.test(e1_01) && eqn.e2.test(e2_01);
			int in3 = eqn.e0.test(e0_11) && eqn.e1.test(e1_11) && eqn.e2.test(e2_11);
			int sum = in0 + in1 + in2 + in3;

			// All out.
			if (sum == 0)
				continue;

			if (sum == 4)
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
		float ev0o;
		float ev1o;
		float ev2o;

		if (TestEdges)
		{
			ev0o = eqn.e0.evaluate(x, y);
			ev1o = eqn.e1.evaluate(x, y);
			ev2o = eqn.e2.evaluate(x, y);
		}

		for (float yy = y; yy < y + BlockSize; yy += 1.0f)
		{
			float ev0i;
			float ev1i;
			float ev2i;

			if (TestEdges)
			{
				ev0i = ev0o;
				ev1i = ev1o;
				ev2i = ev2o;
			}

			for (float xx = x; xx < x + BlockSize; xx += 1.0f)
			{
				if (!TestEdges || (eqn.e0.test(ev0i) && eqn.e1.test(ev1i) && eqn.e2.test(ev2i)))
				{
					int rint = (int)(eqn.r.evaluate(xx, yy) * 255);
					int gint = (int)(eqn.g.evaluate(xx, yy) * 255);
					int bint = (int)(eqn.b.evaluate(xx, yy) * 255);
					Uint32 color = SDL_MapRGB(m_surface->format, rint, gint, bint);
					putpixel(m_surface, (int)xx, (int)yy, color);
				}

				if (TestEdges)
				{
					ev0i = eqn.e0.stepX(ev0i);
					ev1i = eqn.e1.stepX(ev1i);
					ev2i = eqn.e2.stepX(ev2i);
				}
			}

			if (TestEdges)
			{
				ev0o = eqn.e0.stepY(ev0o);
				ev1o = eqn.e1.stepY(ev1o);
				ev2o = eqn.e2.stepY(ev2o);
			}
		}
	}
};