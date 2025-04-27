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

#include "SDL.h"
#include "SDL_image.h"
#include "Renderer.h"
#include "ObjData.h"
#include "vector_math.h"
#include "Texture.h"
#include <memory>

typedef vmath::vec3<float> vec3f;
typedef vmath::vec4<float> vec4f;
typedef vmath::mat4<float> mat4f;

using namespace swr;

class PixelShader : public PixelShaderBase<PixelShader> {
public:
    static const bool InterpolateZ = false;
    static const bool InterpolateW = true;  // Required for perspective correct texturing
    static const int AVarCount = 0;
    static const int PVarCount = 2;  // UV coordinates

    static SDL_Surface* surface;
    static std::shared_ptr<Texture> texture;

    static void drawPixel(const PixelData &p)
    {
        // Compute texture coordinate derivatives
        float dudx, dudy, dvdx, dvdy;
        p.computePerspectiveDerivatives(*p.equations, 0, dudx, dudy); // U derivatives
        p.computePerspectiveDerivatives(*p.equations, 1, dvdx, dvdy); // V derivatives

        Uint32 sampledColor;
        texture->sample(p.pvar[0], p.pvar[1], dudx, dvdx, dudy, dvdy, sampledColor);

        Uint32 *screenBuffer = (Uint32*)((Uint8 *)surface->pixels + (size_t)p.y * (size_t)surface->pitch + (size_t)p.x * 4);
        *screenBuffer = sampledColor;
    }
};

SDL_Surface* PixelShader::surface;
std::shared_ptr<Texture> PixelShader::texture;

class VertexShader : public VertexShaderBase<VertexShader> {
public:
    static const int AttribCount = 1;
    static const int AVarCount = 0;
    static const int PVarCount = 2;

    static mat4f modelViewProjectionMatrix;

    static void processVertex(VertexShaderInput in, VertexShaderOutput *out)
    {
        const ObjData::VertexArrayData *data = static_cast<const ObjData::VertexArrayData*>(in[0]);

        vec4f position = modelViewProjectionMatrix * vec4f(data->vertex, 1.0f);

        out->x = position.x;
        out->y = position.y;
        out->z = position.z;
        out->w = position.w;
        out->pvar[0] = data->texcoord.x;
        out->pvar[1] = data->texcoord.y;
    }
};

mat4f VertexShader::modelViewProjectionMatrix;

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        fprintf(stderr, "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Box",
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        640, 
        480,
        0
    );
    
    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Surface *screen = SDL_GetWindowSurface(window);
    if (!screen) {
        fprintf(stderr, "Could not get window surface! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    try {
        SDL_Surface *tmp = IMG_Load("data/box.png");
        if (!tmp) {
            throw std::runtime_error(std::string("Could not load texture! SDL_image Error: ") + IMG_GetError());
        }

        SDL_Surface *baseTex = SDL_ConvertSurface(tmp, screen->format, 0);
        SDL_FreeSurface(tmp);

        if (!baseTex) {
            throw std::runtime_error(std::string("Could not convert texture surface! SDL Error: ") + SDL_GetError());
        }

        // Create texture with mipmaps
        PixelShader::texture = std::make_shared<Texture>(baseTex);
        PixelShader::surface = screen;

        std::vector<ObjData::VertexArrayData> vdata;
        std::vector<int> idata;
        ObjData::loadFromFile("data/box.obj").toVertexArray(vdata, idata);

        Rasterizer r;
        VertexProcessor v(&r);
        
        r.setRasterMode(RasterMode::Span);
        r.setScissorRect(0, 0, 640, 480);
        r.setPixelShader<PixelShader>();

        v.setViewport(0, 0, 640, 480);
        v.setCullMode(CullMode::CW);
        v.setVertexShader<VertexShader>();

        // Animation and timing variables
        Uint32 lastFrameTime = SDL_GetTicks();
        Uint32 lastFPSUpdate = lastFrameTime;
        int frameCount = 0;
        float angle = 0.0f;
        const float angularSpeed = 0.5f; // Radians per second
        
        bool running = true;
        SDL_Event e;
        
        while (running) {
            // Handle events
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    running = false;
                    break;
                }
            }

            // Calculate frame timing
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastFrameTime) / 1000.0f;
            lastFrameTime = currentTime;
            
            // Update camera position
            angle += angularSpeed * deltaTime;
            float camX = 5.0f * cos(angle);
            float camZ = 5.0f * sin(angle);
            
            mat4f lookAtMatrix = vmath::lookat_matrix(vec3f(camX, 2.0f, camZ), vec3f(0.0f), vec3f(0.0f, 1.0f, 0.0f));
            mat4f perspectiveMatrix = vmath::perspective_matrix(60.0f, 4.0f / 3.0f, 0.1f, 10.0f);
            VertexShader::modelViewProjectionMatrix = perspectiveMatrix * lookAtMatrix;

            // Clear screen (set to black)
            SDL_FillRect(screen, NULL, 0);

            // Draw the box
            v.setVertexAttribPointer(0, sizeof(ObjData::VertexArrayData), &vdata[0]);
            v.drawElements(DrawMode::Triangle, idata.size(), &idata[0]);

            SDL_UpdateWindowSurface(window);
            
            // Update FPS counter every second
            frameCount++;
            if (currentTime - lastFPSUpdate >= 1000) {
                float fps = frameCount * 1000.0f / (currentTime - lastFPSUpdate);
                char title[64];
                snprintf(title, sizeof(title), "Box - FPS: %.1f", fps);
                SDL_SetWindowTitle(window, title);
                
                frameCount = 0;
                lastFPSUpdate = currentTime;
            }
        }

    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Cleanup
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
