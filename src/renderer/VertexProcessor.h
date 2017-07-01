#pragma once

/** @file */ 

#include <vector>
#include <cassert>
#include "IRasterizer.h"

namespace swr {

/// Maximum supported number of vertex attributes.
const int MaxVertexAttribs = 8;    

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

/// Vertex shader output.
typedef RasterizerVertex VertexShaderOutput;

/// Vertex shader input is an array of vertex attribute pointers.
typedef const void *VertexShaderInput[MaxVertexAttribs];

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
	void clipToPlane(float a, float b, float c, float d)
	{
		if (fullyClipped)
			return;

		float dp0 = a * m_v0.x + b * m_v0.y + c * m_v0.z + d * m_v0.w;
		float dp1 = a * m_v1.x + b * m_v1.y + c * m_v1.z + d * m_v1.w;

		bool dp0neg = dp0 < 0;
		bool dp1neg = dp1 < 0;

		if (dp0neg && dp1neg) {
			fullyClipped = true;
			return;
		} 
		
		if (dp0neg)
		{
			float t = -dp0 / (dp1 - dp0);
			t0 = std::max(t0, t);
		}
		else
		{
			float t = dp0 / (dp0 - dp1);
			t1 = std::min(t1, t);
		}
	}
};

class Helper {
public:
	static VertexShaderOutput interpolateVertex(const VertexShaderOutput &v0, const VertexShaderOutput &v1, float t, int attribCount)
	{
		VertexShaderOutput result;
		
		result.x = v0.x * (1.0f - t) + v1.x * t;
		result.y = v0.y * (1.0f - t) + v1.y * t;
		result.z = v0.z * (1.0f - t) + v1.z * t;
		result.w = v0.w * (1.0f - t) + v1.w * t;
		for (int i = 0; i < attribCount; ++i)
			result.avar[i] = v0.avar[i] * (1.0f - t) + v1.avar[i] * t;

		return result;
	}
};

class PolyClipper {
private:
	int m_attribCount;
	std::vector<int> *m_indicesIn;
	std::vector<int> *m_indicesOut;
	std::vector<VertexShaderOutput> *m_vertices;
	
public:
	PolyClipper()
	{
		m_indicesIn = new std::vector<int>();
		m_indicesOut = new std::vector<int>();
	}

	~PolyClipper()
	{
		delete m_indicesIn;
		delete m_indicesOut;
	}

	void init(std::vector<VertexShaderOutput> *vertices, int i1, int i2, int i3, int attribCount)
	{
		m_attribCount = attribCount;
		m_vertices = vertices;

		m_indicesIn->clear();
		m_indicesOut->clear();
		
		m_indicesIn->push_back(i1);
		m_indicesIn->push_back(i2);
		m_indicesIn->push_back(i3);
	}

	/// Clip the poly to the plane given by the formula a * x + b * y + c * z + d * w.
	void clipToPlane(float a, float b, float c, float d)
	{
		if (fullyClipped())
			return;

		m_indicesOut->clear();

		int idxPrev = (*m_indicesIn)[0];
		m_indicesIn->push_back(idxPrev);

		VertexShaderOutput *vPrev = &(*m_vertices)[idxPrev];
		float dpPrev = a * vPrev->x + b * vPrev->y + c * vPrev->z + d * vPrev->w;

		for (size_t i = 1; i < m_indicesIn->size(); ++i)
		{
			int idx = (*m_indicesIn)[i];
			VertexShaderOutput *v = &(*m_vertices)[idx];
			float dp = a * v->x + b * v->y + c * v->z + d * v->w;

			if (dpPrev >= 0)
				m_indicesOut->push_back(idxPrev);

			if (sgn(dp) != sgn(dpPrev))
			{
				float t = dp < 0 ? dpPrev / (dpPrev - dp) : -dpPrev / (dp - dpPrev);

				VertexShaderOutput vOut = Helper::interpolateVertex((*m_vertices)[idxPrev], (*m_vertices)[idx], t, m_attribCount);
				m_vertices->push_back(vOut);
				m_indicesOut->push_back((int)(m_vertices->size() - 1));
			}

			idxPrev = idx;
			dpPrev = dp;
		}

		std::swap(m_indicesIn, m_indicesOut);
	}

	std::vector<int> &indices() const
	{
		return *m_indicesIn;
	}

	bool fullyClipped() const
	{
		return m_indicesIn->size() < 3;
	}

private:
	template <typename T> int sgn(T val) 
	{
    	return (T(0) < val) - (val < T(0));
	}
};

/// Base class for vertex shaders.
/** Derive your own vertex shaders from this class and redefine AttribCount etc. */
template <class Derived>
class VertexShaderBase {
public:
	/// Number of vertex attribute pointers this vertex shader uses.
	static const int AttribCount = 0;

	/// Process a single vertex.
	/** Implement this in your own vertex shader. */
	static void processVertex(VertexShaderInput in, VertexShaderOutput *out)
	{

	}
};

class DummyVertexShader : public VertexShaderBase<DummyVertexShader> {};

class VertexCache {
private:
	static const int VertexCacheSize = 16;

	int inputIndex[VertexCacheSize];
	int outputIndex[VertexCacheSize];

public:
	VertexCache()
	{
		clear();
	}

	void clear()
	{
		for (size_t i = 0; i < VertexCacheSize; i++)
			inputIndex[i] = -1;
	}

	void set(int inIndex, int outIndex)
	{
		int cacheIndex = inIndex % VertexCacheSize;
		inputIndex[cacheIndex] = inIndex;
		outputIndex[cacheIndex] = outIndex;
	}

	int lookup(int inIndex) const
	{
		int cacheIndex = inIndex % VertexCacheSize;
		if (inputIndex[cacheIndex] == inIndex)
			return outputIndex[cacheIndex];
		else
			return -1;
	}
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