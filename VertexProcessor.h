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

        // TODO: Perspective divide and viewport transform
        
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
        // TODO:
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

	mutable std::vector<VertexShaderOutput> m_verticesOut;
	mutable std::vector<int> m_indicesOut;
    mutable std::vector<int> m_clipMask;
};

} // end namespace swr