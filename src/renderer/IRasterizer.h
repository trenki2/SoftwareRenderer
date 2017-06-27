#pragma once

namespace swr {

const int MaxVar = 32;

struct Vertex {
	float x;
	float y;
	float z;
	float w;
	float var[MaxVar];
};

class IRasterizer {
public:
	virtual ~IRasterizer() {}

	virtual void drawPointList(const Vertex *vertices, const int *indices, size_t indexCount) const = 0;
	virtual void drawLineList(const Vertex *vertices, const int *indices, size_t indexCount) const = 0;
	virtual void drawTriangleList(const Vertex *vertices, const int *indices, size_t indexCount) const = 0;
};

} // end namespace swr