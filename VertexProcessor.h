#pragma once

#include <vector>
#include <cassert>
#include "IRasterizer.h"

namespace swr {

const int MaxVertexAttribs = 8;    

enum class DrawMode {
	Point,
	Line,
	Triangle
};

enum class CullMode {
	None,
	CCW,
	CW
};

typedef Vertex VertexShaderOutput;
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

template <class Derived>
class VertexShaderBase {
public:
	/// Number of vertex attribute pointers this vertex shader uses.
	static const int AttribCount = 0;

	static void processVertex(VertexShaderInput in, VertexShaderOutput *out)
	{

	}
};

class DummyVertexShader : public VertexShaderBase<DummyVertexShader> {};

class VertexProcessor {
public:
	VertexProcessor(IRasterizer *rasterizer)
	{
		setRasterizer(rasterizer);
		setCullMode(CullMode::CCW);
		setVertexShader<DummyVertexShader>();
	}

	void setRasterizer(IRasterizer *rasterizer)
	{
		assert(rasterizer != nullptr);
		m_rasterizer = rasterizer;
	}

	void setViewport(int x, int y, int width, int height)
	{
		m_viewport.x = x;
		m_viewport.y = y;
		m_viewport.width = width;
		m_viewport.height = height;
	}

	void setDepthRange(float n, float f)
	{
		m_depthRange.n = n;
		m_depthRange.f = f;
	}

	void setCullMode(CullMode mode)
	{
		m_cullMode = mode;
	}

	template <class VertexShader>
	void setVertexShader()
	{
		assert(VertexShader::AttribCount <= MaxVertexAttribs);
		m_attribCount = VertexShader::AttribCount;
		m_processVertexFunc = VertexShader::processVertex;
	}

	void setVertexAttribPointer(int index, int stride, const void *buffer)
	{
		assert(index < MaxVertexAttribs);
		m_attributes[index].buffer = buffer;
		m_attributes[index].stride = stride;
	}
	
	void drawElements(DrawMode mode, size_t count, int *indices) const
	{
		m_verticesOut.clear();
		m_indicesOut.clear();

		for (size_t i = 0; i < count; i++)
		{
			int index = indices[i];

			VertexShaderInput vIn;    
			initVertexInput(vIn, index);

			m_indicesOut.push_back(index);
			m_verticesOut.resize(m_verticesOut.size() + 1);
			VertexShaderOutput &vOut = m_verticesOut.back();

			processVertex(vIn, &vOut);
		}

		clipPrimitives(mode);
		transformVertices();
		drawPrimitives(mode);
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
				VertexShaderOutput newV = interpolateVertex(v0, v1, lineClipper.t0);
				m_verticesOut.push_back(newV);
				m_indicesOut[i] = (int)(m_verticesOut.size() - 1);
			}

			if (m_clipMask[index1])
			{
				VertexShaderOutput newV = interpolateVertex(v0, v1, lineClipper.t1);
				m_verticesOut.push_back(newV);
				m_indicesOut[i + 1] = (int)(m_verticesOut.size() - 1);
			}
		}
	}

	void clipTriangles() const
	{
		// TODO:
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

	void drawPrimitives(DrawMode mode) const
	{
		switch (mode)
		{
			case DrawMode::Triangle:
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

	VertexShaderOutput interpolateVertex(const VertexShaderOutput &v0, const VertexShaderOutput &v1, float t) const
	{
		VertexShaderOutput result;
		
		result.x = v0.x * (1.0f - t) + v1.x * t;
		result.y = v0.y * (1.0f - t) + v1.y * t;
		result.z = v0.z * (1.0f - t) + v1.z * t;
		result.w = v0.w * (1.0f - t) + v1.w * t;
		for (int i = 0; i < m_attribCount; ++i)
			result.var[i] = v0.var[i] * (1.0f - t) + v1.var[i] * t;

		return result;
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
			vOut.x = (0.5f * m_viewport.width * vOut.x + m_viewport.x);
			vOut.y = (0.5f * m_viewport.height * -vOut.y + m_viewport.y);
			vOut.z = 0.5f * (m_depthRange.f - m_depthRange.n) * vOut.z + 0.5f * (m_depthRange.n + m_depthRange.f);

			m_alreadyProcessed[index] = true;
		}
	}

private:
	struct {
		int x, y, width, height;
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
	mutable std::vector<VertexShaderOutput> m_verticesOut;
	mutable std::vector<int> m_indicesOut;
	mutable std::vector<int> m_clipMask;
	mutable std::vector<bool> m_alreadyProcessed;
};

} // end namespace swr