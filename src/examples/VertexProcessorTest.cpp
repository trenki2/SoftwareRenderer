#include "SDL.h"
#include "Renderer.h"

using namespace swr;

class PixelShader : public PixelShaderBase<PixelShader> {
public:
	static const bool InterpolateZ = false;
	static const bool InterpolateW = false;
	static const int VarCount = 3;
	
	static SDL_Surface* surface;

	static void drawPixel(const PixelData &p)
	{
		int rint = (int)(p.avar[0] * 255);
		int gint = (int)(p.avar[1] * 255);
		int bint = (int)(p.avar[2] * 255);
		
		Uint32 color = rint << 16 | gint << 8 | bint;

		Uint32 *buffer = (Uint32*)((Uint8 *)surface->pixels + (int)p.y * surface->pitch + (int)p.x * 4);
		*buffer = color;
	}
};

SDL_Surface* PixelShader::surface;

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
		out->avar[0] = data->r;
		out->avar[1] = data->g;
		out->avar[2] = data->b;
	}
};

void drawTriangles(SDL_Surface *s)
{
	Rasterizer r;
	VertexProcessor v(&r);

	r.setScissorRect(0, 0, 640, 480);
	r.setPixelShader<PixelShader>();
	PixelShader::surface = s;

	v.setViewport(100, 100, 640 - 200, 480 - 200);
	v.setCullMode(CullMode::None);
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
	
	vdata[2].x = 1.5f;
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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow(
		"VertexProcessorTest",
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
		640, 
		480,
		0
	);

	SDL_Surface *screen = SDL_GetWindowSurface(window);
	srand(1234);

	drawTriangles(screen);

	SDL_UpdateWindowSurface(window);

	SDL_Event e;
	while (SDL_WaitEvent(&e) && e.type != SDL_QUIT);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}