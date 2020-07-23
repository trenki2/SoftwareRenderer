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

#include "VertexProcessor.h"

namespace swr {

VertexProcessor::VertexProcessor(IRasterizer *rasterizer)
{
	setRasterizer(rasterizer);
	setCullMode(CullMode::CW);
	setDepthRange(0.0f, 1.0f);
	setVertexShader<DummyVertexShader>();
}

void VertexProcessor::setRasterizer(IRasterizer *rasterizer)
{
	assert(rasterizer != nullptr);
	m_rasterizer = rasterizer;
}

void VertexProcessor::setViewport(int x, int y, int width, int height)
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

void VertexProcessor::setDepthRange(float n, float f)
{
	m_depthRange.n = n;
	m_depthRange.f = f;
}

void VertexProcessor::setCullMode(CullMode mode)
{
	m_cullMode = mode;
}

void VertexProcessor::setVertexAttribPointer(int index, int stride, const void *buffer)
{
	assert(index < MaxVertexAttribs);
	
	#pragma warning (push)
	#pragma warning (disable : 6386) // the assert causes the VC++ x64 compiler to think that we have a buffer overrun
	m_attributes[index].buffer = buffer;
	m_attributes[index].stride = stride;
	#pragma warning (pop)
}

void VertexProcessor::drawElements(DrawMode mode, size_t count, int *indices) const
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

int VertexProcessor::clipMask(VertexShaderOutput &v) const
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

const void *VertexProcessor::attribPointer(int attribIndex, int elementIndex) const
{
	const Attribute &attrib = m_attributes[attribIndex];
	size_t offset = (size_t)attrib.stride * (size_t)elementIndex;
	return (char*)attrib.buffer + offset;
}

void VertexProcessor::processVertex(VertexShaderInput in, VertexShaderOutput *out) const
{
	(*m_processVertexFunc)(in, out);
}

void VertexProcessor::initVertexInput(VertexShaderInput in, int index) const
{
	for (int i = 0; i < m_attribCount; ++i)
		in[i] = attribPointer(i, index);
}

void VertexProcessor::clipPoints() const
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

void VertexProcessor::clipLines() const
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
			VertexShaderOutput newV = Helper::interpolateVertex(v0, v1, lineClipper.t0, m_avarCount, m_pvarCount);
			m_verticesOut.push_back(newV);
			m_indicesOut[i] = (int)(m_verticesOut.size() - 1);
		}

		if (m_clipMask[index1])
		{
			VertexShaderOutput newV = Helper::interpolateVertex(v0, v1, lineClipper.t1, m_avarCount, m_pvarCount);
			m_verticesOut.push_back(newV);
			m_indicesOut[i + 1] = (int)(m_verticesOut.size() - 1);
		}
	}
}

void VertexProcessor::clipTriangles() const
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

		polyClipper.init(&m_verticesOut, i0, i1, i2, m_avarCount, m_pvarCount);

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

void VertexProcessor::clipPrimitives(DrawMode mode) const
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

void VertexProcessor::processPrimitives(DrawMode mode) const
{
	clipPrimitives(mode);
	transformVertices();
	drawPrimitives(mode);
}

int VertexProcessor::primitiveCount(DrawMode mode) const
{
	int factor = 1;
	
	switch (mode)
	{
		case DrawMode::Point: factor = 1; break;
		case DrawMode::Line: factor = 2; break;
		case DrawMode::Triangle: factor = 3; break;
	}

	return (int)(m_indicesOut.size() / factor);
}

void VertexProcessor::drawPrimitives(DrawMode mode) const
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

void VertexProcessor::cullTriangles() const
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

void VertexProcessor::transformVertices() const
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

} // end namespace swr