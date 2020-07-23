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

namespace swr {

/// Base class for vertex shaders.
/** Derive your own vertex shaders from this class and redefine AttribCount etc. */
template <class Derived>
class VertexShaderBase {
public:
	/// Number of vertex attribute pointers this vertex shader uses.
	static const int AttribCount = 0;

	/// Number of affine output variables.
	static const int AVarCount = 0;

	/// Number of perspective correct output variables.
	static const int PVarCount = 0;

	/// Process a single vertex.
	/** Implement this in your own vertex shader. */
	static void processVertex(VertexShaderInput in, VertexShaderOutput *out)
	{

	}
};

class DummyVertexShader : public VertexShaderBase<DummyVertexShader> {};

} // end namespace swr