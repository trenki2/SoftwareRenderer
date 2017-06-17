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
		verticesOut.clear();
		indicesOut.clear();

        for (size_t i = 0; i < count; i++)
        {
            int index = indices[i];

            VertexShaderInput vIn;    
            vertexInputInit(vIn, index);

            indicesOut.push_back(index);
            verticesOut.resize(verticesOut.size() + 1);
            VertexShaderOutput &vOut = verticesOut.back();

            processVertex(vIn, &vOut);
        }

        switch (mode)
        {
            case DrawMode::Triangle:
                m_rasterizer->drawTriangleList(&verticesOut[0], &indicesOut[0], indicesOut.size());
                break;
            case DrawMode::Line:
                m_rasterizer->drawLineList(&verticesOut[0], &indicesOut[0], indicesOut.size());
                break;
            case DrawMode::Point:
                m_rasterizer->drawPointList(&verticesOut[0], &indicesOut[0], indicesOut.size());
                break;
        }
    }

private:
    void processVertex(VertexShaderInput in, VertexShaderOutput *out) const
    {
        (*m_processVertexFunc)(in, out);
    }

    const void *attribPointer(int attribIndex, int elementIndex) const
    {
        const Attribute &attrib = m_attributes[attribIndex];
        return (char*)attrib.buffer + attrib.stride * elementIndex;
    }

    void vertexInputInit(VertexShaderInput in, int index) const
    {
        for (int i = 0; i < m_attribCount; ++i)
            in[i] = attribPointer(i, index);
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

	mutable std::vector<VertexShaderOutput> verticesOut;
	mutable std::vector<int> indicesOut;
};

} // end namespace swr