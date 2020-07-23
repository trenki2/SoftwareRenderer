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

#include "vector_math.h"
#include <vector>

// Use this class to load Obj files from disk 
// and convert them to vertex arrays.
struct ObjData {
	struct VertexArrayData {
		vmath::vec3<float> vertex;
		vmath::vec3<float> normal;
		vmath::vec2<float> texcoord;
	};

	struct VertexRef {
		unsigned vertexIndex;
		unsigned normalIndex;
		unsigned texcoordIndex;
	};
	
	typedef std::vector<VertexRef> Face;

	std::vector< vmath::vec3<float> > vertices;
	std::vector< vmath::vec3<float> > normals;
	std::vector< vmath::vec2<float> > texcoords;
	std::vector<Face> faces;

	// Load the .obj file
	static ObjData loadFromFile(const char *filename);

	// Convert to vertex and index array
	void toVertexArray(std::vector<VertexArrayData> &vdata, std::vector<int> &idata);
};

