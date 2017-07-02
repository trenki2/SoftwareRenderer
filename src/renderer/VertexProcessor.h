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
	VertexProcessor(IRasterizer *rasterizer);

	/// Change the rasterizer where the primitives are sent.
	void setRasterizer(IRasterizer *rasterizer);

	/// Set the viewport.
	/** Top-Left is (0, 0) */
	void setViewport(int x, int y, int width, int height);

	/// Set the depth range.
	/** Default is (0, 1) */
	void setDepthRange(float n, float f);

	/// Set the cull mode.
	/** Default is CullMode::CW to cull clockwise triangles. */
	void setCullMode(CullMode mode);

	/// Set the vertex shader.
	template <class VertexShader>
	void setVertexShader()
	{
		assert(VertexShader::AttribCount <= MaxVertexAttribs);
		m_attribCount = VertexShader::AttribCount;
		m_processVertexFunc = VertexShader::processVertex;
	}

	/// Set a vertex attrib pointer.
	void setVertexAttribPointer(int index, int stride, const void *buffer);
	
	/// Draw a number of points, lines or triangles.
	void drawElements(DrawMode mode, size_t count, int *indices) const;

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

	int clipMask(VertexShaderOutput &v) const;
	const void *attribPointer(int attribIndex, int elementIndex) const;
	void processVertex(VertexShaderInput in, VertexShaderOutput *out) const;
	void initVertexInput(VertexShaderInput in, int index) const;

	void clipPoints() const;
	void clipLines() const;
	void clipTriangles() const;

	void clipPrimitives(DrawMode mode) const;
	void processPrimitives(DrawMode mode) const;
	int primitiveCount(DrawMode mode) const;

	void drawPrimitives(DrawMode mode) const;
	void cullTriangles() const;
	void transformVertices() const;

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