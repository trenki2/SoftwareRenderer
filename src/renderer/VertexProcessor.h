#pragma once

/** @file */ 

#include <vector>
#include <cassert>

#include "IRasterizer.h"
#include "VertexConfig.h"
#include "LineClipper.h"
#include "PolyClipper.h"
#include "VertexShaderBase.h"
#include "VertexCache.h"

namespace swr {

/// Primitive draw mode.
enum class DrawMode {
	Point,
	Line,
	Triangle
};

/// Triangle culling mode.
enum class CullMode {
	None,
	CCW,
	CW
};

/// Process vertices and pass them to a rasterizer.
class VertexProcessor {
public:
	/// Constructor.
	VertexProcessor(IRasterizer *rasterizer)
	{
		setRasterizer(rasterizer);
		setCullMode(CullMode::CW);
		setDepthRange(0.0f, 1.0f);
		setVertexShader<DummyVertexShader>();
	}

	/// Change the rasterizer where the primitives are sent.
	void setRasterizer(IRasterizer *rasterizer)
	{
		assert(rasterizer != nullptr);
		m_rasterizer = rasterizer;
	}

	/// Set the viewport.
	/** Top-Left is (0, 0) */
	void setViewport(int x, int y, int width, int height)
	{
		m_viewport.x = x;
		m_viewport.y = y;
		m_viewport.width = width;
		m_viewport.height = height;

		m_viewport.px = width / 2.0f;
		m_viewport.py = height / 2.0f;
		m_viewport.ox = (x + m_viewport.px);
		m_viewport.oy = (y + m_viewport.py);
	}

	/// Set the depth range.
	/** Default is (0, 1) */
	void setDepthRange(float n, float f)
	{
		m_depthRange.n = n;
		m_depthRange.f = f;
	}

	/// Set the cull mode.
	/** Default is CullMode::CW to cull clockwise triangles. */
	void setCullMode(CullMode mode)
	{
		m_cullMode = mode;
	}

	/// Set the vertex shader.
	template <class VertexShader>
	void setVertexShader()
	{
		assert(VertexShader::AttribCount <= MaxVertexAttribs);
		m_attribCount = VertexShader::AttribCount;
		m_processVertexFunc = VertexShader::processVertex;
	}

	/// Set a vertex attrib pointer.
	void setVertexAttribPointer(int index, int stride, const void *buffer)
	{
		assert(index < MaxVertexAttribs);
		m_attributes[index].buffer = buffer;
		m_attributes[index].stride = stride;
	}
	
	/// Draw a number of points, lines or triangles.
	void drawElements(DrawMode mode, size_t count, int *indices) const
	{
		m_verticesOut.clear();
		m_indicesOut.clear();

		// TODO: Max 1024 primitives per batch.
		VertexCache vCache;

		for (size_t i = 0; i < count; i++)
		{
			int index = indices[i];
			int outputIndex = vCache.lookup(index);
			
			if (outputIndex != -1)
			{
				m_indicesOut.push_back(outputIndex);
			}
			else
			{
				VertexShaderInput vIn;    
				initVertexInput(vIn, index);

				int outputIndex = (int)m_verticesOut.size();
				m_indicesOut.push_back(outputIndex);
				m_verticesOut.resize(m_verticesOut.size() + 1);
				VertexShaderOutput &vOut = m_verticesOut.back();
				
				processVertex(vIn, &vOut);

				vCache.set(index, outputIndex);
			}

			if (primitiveCount(mode) >= 1024)
			{
				processPrimitives(mode);
				m_verticesOut.clear();
				m_indicesOut.clear();
				vCache.clear();
			}
		}

		processPrimitives(mode);
	}

private:
	struct ClipMask {
		enum Enum {
			PosX = 0x01,
			NegX = 0x02,
			PosY = 0x04,
			NegY = 0x08,
			PosZ = 0x10,
			NegZ = 0x20
		};
	};

	int clipMask(VertexShaderOutput &v) const
	{
		int mask = 0;
		if (v.w - v.x < 0) mask |= ClipMask::PosX;
		if (v.x + v.w < 0) mask |= ClipMask::NegX;
		if (v.w - v.y < 0) mask |= ClipMask::PosY;
		if (v.y + v.w < 0) mask |= ClipMask::NegY;
		if (v.w - v.z < 0) mask |= ClipMask::PosZ;
		if (v.z + v.w < 0) mask |= ClipMask::NegZ;
		return mask;
	}

	const void *attribPointer(int attribIndex, int elementIndex) const
	{
		const Attribute &attrib = m_attributes[attribIndex];
		return (char*)attrib.buffer + attrib.stride * elementIndex;
	}

	void processVertex(VertexShaderInput in, VertexShaderOutput *out) const
	{
		(*m_processVertexFunc)(in, out);
	}

	void initVertexInput(VertexShaderInput in, int index) const
	{
		for (int i = 0; i < m_attribCount; ++i)
			in[i] = attribPointer(i, index);
	}

	void clipPoints() const
	{
		m_clipMask.clear();
		m_clipMask.resize(m_verticesOut.size());
		
		for (size_t i = 0; i < m_verticesOut.size(); i++)
			m_clipMask[i] = clipMask(m_verticesOut[i]);

		for (size_t i = 0; i < m_indicesOut.size(); i++)
		{
			if (m_clipMask[m_indicesOut[i]])    
				m_indicesOut[i] = -1;
		}
	}

	void clipLines() const
	{
		m_clipMask.clear();
		m_clipMask.resize(m_verticesOut.size());

		for (size_t i = 0; i < m_verticesOut.size(); i++)
			m_clipMask[i] = clipMask(m_verticesOut[i]);

		for (size_t i = 0; i < m_indicesOut.size(); i += 2)
		{
			int index0 = m_indicesOut[i];
			int index1 = m_indicesOut[i + 1];

			VertexShaderOutput v0 = m_verticesOut[index0];
			VertexShaderOutput v1 = m_verticesOut[index1];

			int clipMask = m_clipMask[index0] | m_clipMask[index1];

			LineClipper lineClipper(v0, v1);

			if (clipMask & ClipMask::PosX) lineClipper.clipToPlane(-1, 0, 0, 1);
			if (clipMask & ClipMask::NegX) lineClipper.clipToPlane( 1, 0, 0, 1);
			if (clipMask & ClipMask::PosY) lineClipper.clipToPlane( 0,-1, 0, 1);
			if (clipMask & ClipMask::NegY) lineClipper.clipToPlane( 0, 1, 0, 1);
			if (clipMask & ClipMask::PosZ) lineClipper.clipToPlane( 0, 0,-1, 1);
			if (clipMask & ClipMask::NegZ) lineClipper.clipToPlane( 0, 0, 1, 1);

			if (lineClipper.fullyClipped)
			{
				m_indicesOut[i] = -1;
				m_indicesOut[i + 1] = -1;
				continue;
			}

			if (m_clipMask[index0])
			{
				VertexShaderOutput newV = Helper::interpolateVertex(v0, v1, lineClipper.t0, m_attribCount);
				m_verticesOut.push_back(newV);
				m_indicesOut[i] = (int)(m_verticesOut.size() - 1);
			}

			if (m_clipMask[index1])
			{
				VertexShaderOutput newV = Helper::interpolateVertex(v0, v1, lineClipper.t1, m_attribCount);
				m_verticesOut.push_back(newV);
				m_indicesOut[i + 1] = (int)(m_verticesOut.size() - 1);
			}
		}
	}

	void clipTriangles() const
	{
		m_clipMask.clear();
		m_clipMask.resize(m_verticesOut.size());

		for (size_t i = 0; i < m_verticesOut.size(); i++)
			m_clipMask[i] = clipMask(m_verticesOut[i]);

		size_t n = m_indicesOut.size();

		for (size_t i = 0; i < n; i += 3)
		{
			int i0 = m_indicesOut[i];
			int i1 = m_indicesOut[i + 1];
			int i2 = m_indicesOut[i + 2];

			int clipMask = m_clipMask[i0] | m_clipMask[i1] | m_clipMask[i2];

			polyClipper.init(&m_verticesOut, i0, i1, i2, m_attribCount);

			if (clipMask & ClipMask::PosX) polyClipper.clipToPlane(-1, 0, 0, 1);
			if (clipMask & ClipMask::NegX) polyClipper.clipToPlane( 1, 0, 0, 1);
			if (clipMask & ClipMask::PosY) polyClipper.clipToPlane( 0,-1, 0, 1);
			if (clipMask & ClipMask::NegY) polyClipper.clipToPlane( 0, 1, 0, 1);
			if (clipMask & ClipMask::PosZ) polyClipper.clipToPlane( 0, 0,-1, 1);
			if (clipMask & ClipMask::NegZ) polyClipper.clipToPlane( 0, 0, 1, 1);

			if (polyClipper.fullyClipped())
			{
				m_indicesOut[i] = -1;
				m_indicesOut[i + 1] = -1;
				m_indicesOut[i + 2] = -1;
				continue;
			}

			std::vector<int> &indices = polyClipper.indices();

			m_indicesOut[i] = indices[0];
			m_indicesOut[i + 1] = indices[1];
			m_indicesOut[i + 2] = indices[2];
			for (size_t idx = 3; idx < indices.size(); ++idx) {
				m_indicesOut.push_back(indices[0]);
				m_indicesOut.push_back(indices[idx - 1]);
				m_indicesOut.push_back(indices[idx]);
			}
		}
	}

	void clipPrimitives(DrawMode mode) const
	{
		switch (mode)
		{
			case DrawMode::Point:
				clipPoints();
				break;
			case DrawMode::Line:
				clipLines();
				break;
			case DrawMode::Triangle:
				clipTriangles();
				break;
		}
	}

	void processPrimitives(DrawMode mode) const
	{
		clipPrimitives(mode);
		transformVertices();
		drawPrimitives(mode);
	}

	int primitiveCount(DrawMode mode) const
	{
		int factor;
		
		switch (mode)
		{
			case DrawMode::Point: factor = 1; break;
			case DrawMode::Line: factor = 2; break;
			case DrawMode::Triangle: factor = 3; break;
		}

		return m_indicesOut.size() / factor;
	}

	void drawPrimitives(DrawMode mode) const
	{
		switch (mode)
		{
			case DrawMode::Triangle:
				cullTriangles();
				m_rasterizer->drawTriangleList(&m_verticesOut[0], &m_indicesOut[0], m_indicesOut.size());
				break;
			case DrawMode::Line:
				m_rasterizer->drawLineList(&m_verticesOut[0], &m_indicesOut[0], m_indicesOut.size());
				break;
			case DrawMode::Point:
				m_rasterizer->drawPointList(&m_verticesOut[0], &m_indicesOut[0], m_indicesOut.size());
				break;
		}
	}

	void cullTriangles() const
	{
		for (size_t i = 0; i + 3 <= m_indicesOut.size(); i += 3)
		{
			if (m_indicesOut[i] == -1)
				continue;

			VertexShaderOutput &v0 = m_verticesOut[m_indicesOut[i]];
			VertexShaderOutput &v1 = m_verticesOut[m_indicesOut[i + 1]];
			VertexShaderOutput &v2 = m_verticesOut[m_indicesOut[i + 2]];

			float facing = (v0.x - v1.x) * (v2.y - v1.y) - (v2.x - v1.x) * (v0.y - v1.y);

			if (facing < 0)
			{
				if (m_cullMode == CullMode::CW)
					m_indicesOut[i] = m_indicesOut[i + 1] = m_indicesOut[i + 2] = -1;
			}
			else
			{
				if (m_cullMode == CullMode::CCW)
					m_indicesOut[i] = m_indicesOut[i + 1] = m_indicesOut[i + 2] = -1;
				else
					std::swap(m_indicesOut[i], m_indicesOut[i + 2]);
			}
		}
	}

	void transformVertices() const
	{
		m_alreadyProcessed.clear();
		m_alreadyProcessed.resize(m_verticesOut.size());

		for (size_t i = 0; i < m_indicesOut.size(); i++)
		{
			int index = m_indicesOut[i];

			if (index == -1)
				continue;

			if (m_alreadyProcessed[index])
				continue;

			VertexShaderOutput &vOut = m_verticesOut[index];

			// Perspective divide
			float invW = 1.0f / vOut.w;
			vOut.x *= invW;
			vOut.y *= invW;
			vOut.z *= invW;

			// Viewport transform
			vOut.x = (m_viewport.px * vOut.x + m_viewport.ox);
			vOut.y = (m_viewport.py * -vOut.y + m_viewport.oy);
			vOut.z = 0.5f * (m_depthRange.f - m_depthRange.n) * vOut.z + 0.5f * (m_depthRange.n + m_depthRange.f);

			m_alreadyProcessed[index] = true;
		}
	}

private:
	struct {
		int x, y, width, height;
		float px, py, ox, oy;
	} m_viewport;

	struct {
		float n, f;
	} m_depthRange;

	CullMode m_cullMode;
	IRasterizer *m_rasterizer;
	
	void (*m_processVertexFunc)(VertexShaderInput, VertexShaderOutput*);
	int m_attribCount;

	struct Attribute {
		const void *buffer;
		int stride;
	} m_attributes[MaxVertexAttribs];

	// Some temporary variables for speed
	mutable PolyClipper polyClipper;
	mutable std::vector<VertexShaderOutput> m_verticesOut;
	mutable std::vector<int> m_indicesOut;
	mutable std::vector<int> m_clipMask;
	mutable std::vector<bool> m_alreadyProcessed;
};

} // end namespace swr