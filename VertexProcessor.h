#pragma once

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
    
    void drawElements(DrawMode type, int count, int *indices)
    {
        VertexInput vin;
        VertexOutput v0, v1, v2;

        prepareVertexInput(vin, 0); processVertex(vin, &v0);
        prepareVertexInput(vin, 1); processVertex(vin, &v1);
        prepareVertexInput(vin, 2); processVertex(vin, &v2);

        m_rasterizer->drawTriangle(v0, v1, v2);
    }

private:
    void processVertex(VertexInput in, VertexOutput *out)
    {
        (*m_processVertexFunc)(in, out);
    }

    const void *attribPointer(int attribIndex, int elementIndex)
    {
        Attribute &attrib = m_attributes[attribIndex];
        return (char*)attrib.buffer + attrib.stride * elementIndex;
    }

    void prepareVertexInput(VertexInput in, int index)
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