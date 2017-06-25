#include "SDL.h"
#include "Rasterizer.h"
#include "VertexProcessor.h"
#include "ObjData.h"

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow(
		"Crate",
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
		640, 
		480,
		0
	);

	SDL_Surface *screen = SDL_GetWindowSurface(window);
	srand(1234);

	std::vector<ObjData::VertexArrayData> vdata;
	std::vector<int> idata;
	ObjData::loadFromFile("data/crate.obj").toVertexArray(vdata, idata);

	SDL_UpdateWindowSurface(window);

	SDL_Event e;
	while (SDL_WaitEvent(&e) && e.type != SDL_QUIT);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}