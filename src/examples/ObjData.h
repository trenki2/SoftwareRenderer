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

