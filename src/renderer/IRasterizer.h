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

#pragma once

/** @file */ 

#include <cstddef>

namespace swr {

const int BlockSize = 8;

/// Maximum affine variables used for interpolation across the triangle.
const int MaxAVars = 16;

/// Maximum perspective variables used for interpolation across the triangle.
const int MaxPVars = 16;

/// Vertex input structure for the Rasterizer. Output from the VertexProcessor.
struct RasterizerVertex {
	float x; ///< The x component.
	float y; ///< The y component.
	float z; ///< The z component.
	float w; ///< The w component.

	/// Affine variables.
	float avar[MaxAVars];

	/// Perspective variables.
	float pvar[MaxPVars];
};

/// Interface for the rasterizer used by the VertexProcessor.
class IRasterizer {
public:
	virtual ~IRasterizer() {}

	/// Draw a list of points.
	/** Points with indices == -1 will be ignored. */
	virtual void drawPointList(const RasterizerVertex *vertices, const int *indices, size_t indexCount) const = 0;

	/// Draw a list if lines.
	/** Lines  with indices == -1 will be ignored. */
	virtual void drawLineList(const RasterizerVertex *vertices, const int *indices, size_t indexCount) const = 0;

	/// Draw a list of triangles.
	/** Triangles  with indices == -1 will be ignored. */
	virtual void drawTriangleList(const RasterizerVertex *vertices, const int *indices, size_t indexCount) const = 0;
};

} // end namespace swr