#pragma once

namespace swr {

const int MaxAVars = 16;
const int MaxPVars = 16;

struct Vertex {
	float x;
	float y;
	float z;
	float w;
	float avar[MaxAVars];
	float pvar[MaxPVars];
};

class IRasterizer {
public:
	virtual ~IRasterizer() {}

	virtual void drawPointList(const Vertex *vertices, const int *indices, size_t indexCount) const = 0;
	virtual void drawLineList(const Vertex *vertices, const int *indices, size_t indexCount) const = 0;
	virtual void drawTriangleList(const Vertex *vertices, const int *indices, size_t indexCount) const = 0;
};

} // end namespace swr