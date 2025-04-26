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
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "Box",
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        640, 
        480,
        0
    );

    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Surface *tmp = IMG_Load("data/box.png");
    SDL_Surface *baseTex = SDL_ConvertSurface(tmp, screen->format, 0);
    SDL_FreeSurface(tmp);

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

    mat4f lookAtMatrix = vmath::lookat_matrix(vec3f(3.0f, 2.0f, 5.0f), vec3f(0.0f), vec3f(0.0f, 1.0f, 0.0f));
    mat4f perspectiveMatrix = vmath::perspective_matrix(60.0f, 4.0f / 3.0f, 0.1f, 10.0f);
    VertexShader::modelViewProjectionMatrix = perspectiveMatrix * lookAtMatrix;

    v.setVertexAttribPointer(0, sizeof(ObjData::VertexArrayData), &vdata[0]);
    v.drawElements(DrawMode::Triangle, idata.size(), &idata[0]);

    SDL_UpdateWindowSurface(window);

    SDL_Event e;
    while (SDL_WaitEvent(&e) && e.type != SDL_QUIT);

    // Texture will be automatically cleaned up by shared_ptr
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
