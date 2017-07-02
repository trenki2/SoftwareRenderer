#pragma once

/** @file */

namespace swr {

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

} // end namespace swr