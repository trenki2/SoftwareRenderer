#include "SDL.h"
#include "Rasterizer.h"
#include "VertexProcessor.h"

using namespace swr;

class NullRasterizer : public IRasterizer {
public:
	virtual void drawPoint(const Vertex &v) const {}
	virtual void drawLine(const Vertex &v0, const Vertex &v1) const {}
	virtual void drawTriangle(const Vertex& v0, const Vertex &v1, const Vertex &v2) const {}

	virtual void drawPointList(const Vertex *vertices, const int *indices, size_t indexCount) const {}
	virtual void drawLineList(const Vertex *vertices, const int *indices, size_t indexCount) const {}
	virtual void drawTriangleList(const Vertex *vertices, const int *indices, size_t indexCount) const {}
};

class PixelShader : public PixelShaderBase<PixelShader> {
public:
	static const bool InterpolateZ = false;
	static const bool InterpolateW = false;
	static const int VarCount = 3;
	
	static SDL_Surface* surface;

	static void drawPixel(const PixelData &p)
	{
		int rint = (int)(p.var[0] * 255);
		int gint = (int)(p.var[1] * 255);
		int bint = (int)(p.var[2] * 255);
		
		Uint32 color = rint << 16 | gint << 8 | bint;

		Uint32 *buffer = (Uint32*)((Uint8 *)surface->pixels + (int)p.y * surface->pitch + (int)p.x * 4);
		*buffer = color;
	}
};

struct VertexData {
	float x, y, z;
	float r, g, b;
};

class VertexShader : public VertexShaderBase<VertexShader> {
public:
	static const int AttribCount = 3;

	static void processVertex(VertexShaderInput in, VertexShaderOutput *out)
	{
		const VertexData *data = static_cast<const VertexData*>(in[0]);
		out->x = data->x;
		out->y = data->y;
		out->z = data->z;
		out->w = 1.0f;
		out->var[0] = data->r;
		out->var[1] = data->g;
		out->var[2] = data->b;
	}
};

SDL_Surface* PixelShader::surface;

void drawLines(SDL_Surface *screen)
{
	Rasterizer r;
	VertexProcessor v(&r);

	r.setScissorRect(0, 0, 640, 480);
	r.setPixelShader<PixelShader>();
	PixelShader::surface = screen;

	v.setViewport(100, 100, 640 - 200, 480 - 200);
	v.setCullMode(CullMode::None);
	v.setVertexShader<VertexShader>();

	VertexData vdata[2];
	int idata[2];

	vdata[0].x = -2.0f;
	vdata[0].y = -3.0f;
	vdata[0].z = -4.0f;
	vdata[0].r = 1.0f;
	vdata[0].g = 0.0f;
	vdata[0].b = 0.0f;

	vdata[1].x = 3.0f;
	vdata[1].y = 2.0f;
	vdata[1].z = 4.0f;
	vdata[1].r = 0.0f;
	vdata[1].g = 1.0f;
	vdata[1].b = 0.0f;

	idata[0] = 0;
	idata[1] = 1;
	
	v.setVertexAttribPointer(0, sizeof(VertexData), vdata);
	v.drawElements(DrawMode::Line, 2, idata);
}

void drawTriangle(SDL_Surface *s)
{
	Rasterizer r;
	VertexProcessor v(&r);

	r.setScissorRect(0, 0, 640, 480);
	r.setPixelShader<PixelShader>();
	PixelShader::surface = s;

	v.setViewport(100, 100, 640 - 200, 480 - 200);
	v.setCullMode(CullMode::CW);
	v.setVertexShader<VertexShader>();

	VertexData vdata[3];

	vdata[0].x = 0.0f;
	vdata[0].y = 0.5f;
	vdata[0].z = 0.0f;
	vdata[0].r = 1.0f;
	vdata[0].g = 0.0f;
	vdata[0].b = 0.0f;

	vdata[1].x = -0.5f;
	vdata[1].y = -0.5f;
	vdata[1].z = 0.0f;
	vdata[1].r = 0.0f;
	vdata[1].g = 0.0f;
	vdata[1].b = 0.0f;
	
	vdata[2].x = 0.5f;
	vdata[2].y = -0.5f;
	vdata[2].z = 0.0f;
	vdata[2].r = 0.0f;
	vdata[2].g = 0.0f;
	vdata[2].b = 0.0f;

	int idata[3];
	idata[0] = 0;
	idata[1] = 1;
	idata[2] = 2;

	v.setVertexAttribPointer(0, sizeof(VertexData), vdata);
	v.drawElements(DrawMode::Triangle, 3, idata);
}

void drawTriangleClip(SDL_Surface *s)
{
	Rasterizer r;
	VertexProcessor v(&r);

	r.setScissorRect(0, 0, 640, 480);
	r.setPixelShader<PixelShader>();
	PixelShader::surface = s;

	v.setViewport(100, 100, 640 - 200, 480 - 200);
	v.setCullMode(CullMode::CW);
	v.setVertexShader<VertexShader>();

	VertexData vdata[3];

	vdata[0].x = 0.0f;
	vdata[0].y = 0.5f;
	vdata[0].z = 0.0f;
	vdata[0].r = 1.0f;
	vdata[0].g = 0.0f;
	vdata[0].b = 0.0f;

	vdata[1].x = -1.5f;
	vdata[1].y = -0.5f;
	vdata[1].z = 0.0f;
	vdata[1].r = 0.0f;
	vdata[1].g = 1.0f;
	vdata[1].b = 0.0f;

	vdata[2].x = 0.5f;
	vdata[2].y = -0.5f;
	vdata[2].z = 0.0f;
	vdata[2].r = 0.0f;
	vdata[2].g = 0.0f;
	vdata[2].b = 1.0f;

	int idata[3];
	idata[0] = 0;
	idata[1] = 1;
	idata[2] = 2;

	v.setVertexAttribPointer(0, sizeof(VertexData), vdata);
	v.drawElements(DrawMode::Triangle, 3, idata);
}

VertexData randomVertexData()
{
	VertexData r;

	r.x = float(rand()) / RAND_MAX * 4.0f - 2.0f;
	r.y = float(rand()) / RAND_MAX * 4.0f - 2.0f;
	r.y = float(rand()) / RAND_MAX * 4.0f - 2.0f;

	r.r = float(rand()) / RAND_MAX;
	r.g = float(rand()) / RAND_MAX;
	r.b = float(rand()) / RAND_MAX;

	return r;
}

void drawManyTriangles(SDL_Surface *s)
{
	Rasterizer r;
	VertexProcessor v(&r);

	r.setScissorRect(0, 0, 640, 480);
	r.setPixelShader<PixelShader>();
	PixelShader::surface = s;

	v.setViewport(100, 100, 640 - 200, 480 - 200);
	v.setCullMode(CullMode::None);
	v.setVertexShader<VertexShader>();

	for (size_t i = 0; i < 10000; i++)
	{
		VertexData vdata[3];

		vdata[0] = randomVertexData();
		vdata[1] = randomVertexData();
		vdata[2] = randomVertexData();

		int idata[3];
		idata[0] = 0;
		idata[1] = 1;
		idata[2] = 2;

		v.setVertexAttribPointer(0, sizeof(VertexData), vdata);
		v.drawElements(DrawMode::Triangle, 3, idata);
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow(
		"Software Renderer",
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
		640, 
		480,
		0
	);

	SDL_Surface *screen = SDL_GetWindowSurface(window);

	srand(1234);

	//drawManyTriangles(screen);
	drawTriangle(screen);
	//drawLines(screen);

	SDL_UpdateWindowSurface(window);

	SDL_Event e;
	while (SDL_WaitEvent(&e) && e.type != SDL_QUIT);
	
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
