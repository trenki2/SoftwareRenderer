#include "SDL.h"
#include "Rasterizer.h"

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
*/
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
	int bpp = surface->format->BytesPerPixel;

	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *)p = pixel;
		break;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}

class PixelShader : public PixelShaderBase<PixelShader> {
public:
	static const bool InterpolateZ = false;
	static const bool InterpolateW = false;
	static const int VarCount = 3;
	
	static SDL_Surface* surface;

	static int float2int(float v)
	{
		return _mm_cvtt_ss2si(_mm_load_ss(&v));
	}

	static void drawPixel(const PixelData &p)
	{
		int rint = float2int(p.var[0] * 255);
		int gint = float2int(p.var[1] * 255);
		int bint = float2int(p.var[2] * 255);
		
		Uint32 color = rint << 16 | gint << 8 | bint;

		Uint32 *buffer = (Uint32*)((Uint8 *)surface->pixels + (int)p.y * surface->pitch + (int)p.x * 4);
		*buffer = color;
	}
};

SDL_Surface* PixelShader::surface;

Vertex randomVertex()
{
	Vertex v;
	
	v.x = (float)(rand() % 640);
	v.y = (float)(rand() % 480);

	v.var[0] = (float)rand() / RAND_MAX;
	v.var[1] = (float)rand() / RAND_MAX;
	v.var[2] = (float)rand() / RAND_MAX;

	return v;
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

	Rasterizer r;
	r.setScissorRect(0, 0, 640, 480);
	r.setPixelShader<PixelShader>();
	PixelShader::surface = screen;

	Uint32 start = SDL_GetTicks();
	
Vertex v0, v1, v2;

	v0.x = 320;
	v0.y = 100;
	v0.var[0] = 1.0;
	v0.var[1] = 0.0;
	v0.var[2] = 0.0;

	v2.x = 180;
	v2.y = 200;
	v2.var[0] = 0.0;
	v2.var[1] = 1.0;
	v2.var[2] = 0.0;

	v1.x = 480;
	v1.y = 300;
	v1.var[0] = 0.0;
	v1.var[1] = 0.0;
	v1.var[2] = 1.0;

	for (int i = 0; i < 500000; ++i)
	{
		Vertex v0 = randomVertex();
		Vertex v1 = randomVertex();
		Vertex v2 = randomVertex();

		r.drawTriangle(v0, v1, v2);
	}

	r.drawTriangle(v0, v1, v2);

	/*Vertex v0, v1;

	v0.x = 150;
	v0.y = 100;
	v0.var[0] = 1.0;
	v0.var[1] = 0.0;
	v0.var[2] = 0.0;

	v1.x = 200;
	v1.y = 300;
	v1.var[0] = 0.0;
	v1.var[1] = 0.0;
	v1.var[2] = 1.0;

	r.drawLine(v0, v1);
	*/
	Uint32 end = SDL_GetTicks();
	printf("%i\n", end - start);

	SDL_UpdateWindowSurface(window);

	SDL_Delay(3000);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
