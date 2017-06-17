#pragma once

#include <vector>
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

typedef Vertex VertexOutput;
typedef const void *VertexInput[MaxVertexAttribs];

template <class Derived>
class VertexShaderBase {
public:
    /// Number of vertex attribute pointers this vertex shader uses.
    static const int AttribCount = 0;

    static void processVertex(VertexInput in, VertexOutput *out)
    {

    }
};

class DummyVertexShader : public VertexShaderBase<DummyVertexShader> {};

class VertexProcessor {
public:
    VertexProcessor(IRasterizer *rasterizer)
    {
        m_rasterizer = rasterizer;
        setCullMode(CullMode::CCW);
        setVertexShader<DummyVertexShader>();
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
        m_processVertexFunc = VertexShader::processVertex;
        m_attribCount = VertexShader::AttribCount;
    }

    void setVertexAttribPointer(int index, int stride, const void *buffer)
    {
        m_attributes[index].buffer = buffer;
        m_attributes[index].stride = stride;
    }
    
    void drawElements(DrawMode mode, int count, int *indices) const
    {
        std::vector<VertexOutput> verticesOut;
        std::vector<int> indicesOut;

        for (int i = 0; i < count; i++)
        {
            int index = indices[i];

            VertexInput vIn;    
            vertexInputInit(vIn, index);

            indicesOut.push_back(index);
            verticesOut.resize(verticesOut.size() + 1);
            VertexOutput &vOut = verticesOut.back();

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
    void processVertex(VertexInput in, VertexOutput *out) const
    {
        (*m_processVertexFunc)(in, out);
    }

    const void *attribPointer(int attribIndex, int elementIndex) const
    {
        const Attribute &attrib = m_attributes[attribIndex];
        return (char*)attrib.buffer + attrib.stride * elementIndex;
    }

    void vertexInputInit(VertexInput in, int index) const
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
    
    void (*m_processVertexFunc)(VertexInput, VertexOutput*);
    int m_attribCount;

    struct Attribute {
        const void *buffer;
        int stride;
    } m_attributes[MaxVertexAttribs];
};

} // end namespace swr