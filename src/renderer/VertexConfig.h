#pragma once

/** @file */

namespace swr {

/// Maximum supported number of vertex attributes.
const int MaxVertexAttribs = 8;  

/// Vertex shader output.
typedef RasterizerVertex VertexShaderOutput;

/// Vertex shader input is an array of vertex attribute pointers.
typedef const void *VertexShaderInput[MaxVertexAttribs];

} // end namespace swr

