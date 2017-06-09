#include "SDL.h"
#include "Rasterizer.h"

Vertex randomVertex()
{
	Vertex v;
	
	v.x = (float)(rand() % 640);
	v.y = (float)(rand() % 480);

	v.r = (float)rand() / RAND_MAX;
	v.g = (float)rand() / RAND_MAX;
	v.b = (float)rand() / RAND_MAX;

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
    r.setSurface(screen);
    r.setScissorRect(0, 0, 640, 480);

	Uint32 start = SDL_GetTicks();

	for (int i = 0; i < 10000; ++i)
	{
		Vertex v0 = randomVertex();
		Vertex v1 = randomVertex();
		Vertex v2 = randomVertex();

		r.drawTriangle(v0, v1, v2);
	}

	Vertex v0, v1, v2;

	v0.x = 320;
	v0.y = 100;
	v0.r = 1.0;
	v0.g = 0.0;
	v0.b = 0.0;

	v2.x = 180;
	v2.y = 200;
	v2.r = 0.0;
	v2.g = 1.0;
	v2.b = 0.0;

	v1.x = 480;
	v1.y = 300;
	v1.r = 0.0;
	v1.g = 0.0;
	v1.b = 1.0;

	r.drawTriangle(v0, v1, v2);

	Uint32 end = SDL_GetTicks();
	printf("%i\n", end - start);

    SDL_UpdateWindowSurface(window);

	SDL_Delay(3000);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
