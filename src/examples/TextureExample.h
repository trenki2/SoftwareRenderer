#pragma once

#include "SDL.h"
#include "Texture.h"
#include <memory>

class TexturedPixelShader : public PixelShaderBase<TexturedPixelShader> {
public:
    static const bool InterpolateZ = false;
    static const bool InterpolateW = true;  // Required for perspective correct texturing
    static const int AVarCount = 0;
    static const int PVarCount = 2;  // UV coordinates

    static SDL_Surface* surface;
    static std::shared_ptr<swr::Texture> texture;

    static void drawPixel(const PixelData &p)
    {
        // Compute texture coordinate derivatives
        float dudx, dudy, dvdx, dvdy;
        p.computePerspectiveDerivatives(*p.equations, 0, dudx, dudy); // U derivatives
        p.computePerspectiveDerivatives(*p.equations, 1, dvdx, dvdy); // V derivatives

        Uint32 sampledColor;
        texture->sample(p.pvar[0], p.pvar[1], dudx, dvdx, dudy, dvdy, sampledColor);

        Uint32 *screenBuffer = (Uint32*)((Uint8 *)surface->pixels + p.y * surface->pitch + p.x * 4);
        *screenBuffer = sampledColor;
    }
};

// Static member initialization
SDL_Surface* TexturedPixelShader::surface;
std::shared_ptr<swr::Texture> TexturedPixelShader::texture;