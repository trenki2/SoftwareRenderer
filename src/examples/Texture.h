#pragma once

#include "SDL.h"
#include <vector>
#include <cmath>
#include <iostream>

namespace swr {

class Texture {
public:
    Texture(SDL_Surface* baseSurface) {
        generateMipmaps(baseSurface);
    }

    ~Texture() {
        cleanup();
    }

    // Prevent copying since we manage SDL resources
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void sample(float u, float v, float dudx, float dvdx, float dudy, float dvdy, Uint32& outColor) const {
        // Compute level of detail for mip-mapping
        float maxDeriv = std::max(
            std::sqrt(dudx * dudx + dvdx * dvdx),  // Length of du/dx vector
            std::sqrt(dudy * dudy + dvdy * dvdy)   // Length of du/dy vector
        );
        float lod = std::log2(maxDeriv);
        
        // Get the two mip levels to interpolate between
        int lodBase = std::max(0, int(std::floor(lod)));
        int lodNext = std::min(lodBase + 1, (int)m_mipmaps.size() - 1);
        float lodFrac = std::clamp(lod - lodBase, 0.0f, 1.0f);

        // Sample from both mip levels using bilinear filtering
        Uint32 colorBase, colorNext;
        sampleBilinear(lodBase, u, v, colorBase);
        sampleBilinear(lodNext, u, v, colorNext);

        // Linearly interpolate between the two mip levels
        outColor = lerpColors(colorBase, colorNext, lodFrac);
    }

private:
    void sampleBilinear(int mipLevel, float u, float v, Uint32& outColor) const {
        SDL_Surface* mip = m_mipmaps[mipLevel];

        // Convert texture coordinates to pixel coordinates
        float px = u * (mip->w - 1);
        float py = v * (mip->h - 1);

        // Get the four texel coordinates
        int x0 = std::max(0, int(std::floor(px)));
        int y0 = std::max(0, int(std::floor(py)));
        int x1 = std::min(mip->w - 1, x0 + 1);
        int y1 = std::min(mip->h - 1, y0 + 1);

        // Calculate interpolation factors
        float fx = px - x0;
        float fy = py - y0;

        // Get the four texel colors
        Uint32* pixels = (Uint32*)mip->pixels;
        Uint32 c00 = pixels[y0 * mip->w + x0];
        Uint32 c10 = pixels[y0 * mip->w + x1];
        Uint32 c01 = pixels[y1 * mip->w + x0];
        Uint32 c11 = pixels[y1 * mip->w + x1];

        // Bilinear interpolate each color component directly
        outColor = bilinearLerpColors(c00, c10, c01, c11, fx, fy);
    }

    // Fast color component extraction
    static inline Uint8 getR(Uint32 color) { return (color >> 16) & 0xFF; }
    static inline Uint8 getG(Uint32 color) { return (color >> 8) & 0xFF; }
    static inline Uint8 getB(Uint32 color) { return color & 0xFF; }

    // Fast color packing (assumes RGBA8888 format)
    static inline Uint32 packRGB(Uint8 r, Uint8 g, Uint8 b) {
        return (r << 16) | (g << 8) | b;
    }

    // Linear interpolation between two colors
    static inline Uint32 lerpColors(Uint32 c1, Uint32 c2, float t) {
        Uint8 r = getR(c1) + (Uint8)((getR(c2) - getR(c1)) * t);
        Uint8 g = getG(c1) + (Uint8)((getG(c2) - getG(c1)) * t);
        Uint8 b = getB(c1) + (Uint8)((getB(c2) - getB(c1)) * t);
        return packRGB(r, g, b);
    }

    // Bilinear interpolation between four colors
    static inline Uint32 bilinearLerpColors(Uint32 c00, Uint32 c10, Uint32 c01, Uint32 c11, float fx, float fy) {
        Uint8 r = (Uint8)(
            getR(c00) * (1 - fx) * (1 - fy) +
            getR(c10) * fx * (1 - fy) +
            getR(c01) * (1 - fx) * fy +
            getR(c11) * fx * fy
        );
        Uint8 g = (Uint8)(
            getG(c00) * (1 - fx) * (1 - fy) +
            getG(c10) * fx * (1 - fy) +
            getG(c01) * (1 - fx) * fy +
            getG(c11) * fx * fy
        );
        Uint8 b = (Uint8)(
            getB(c00) * (1 - fx) * (1 - fy) +
            getB(c10) * fx * (1 - fy) +
            getB(c01) * (1 - fx) * fy +
            getB(c11) * fx * fy
        );
        return packRGB(r, g, b);
    }

    void generateMipmaps(SDL_Surface* baseTexture) {
        // Clear any existing mipmaps
        cleanup();

        // Add base level
        m_mipmaps.push_back(baseTexture);

        SDL_Surface* prevLevel = baseTexture;
        int width = baseTexture->w;
        int height = baseTexture->h;

        // Generate mip chain until 1x1
        while (width > 1 || height > 1) {
            int newWidth = std::max(1, width / 2);
            int newHeight = std::max(1, height / 2);

            // Create new surface for this mip level
            SDL_Surface* newMip = SDL_CreateRGBSurface(0, newWidth, newHeight, 32,
                0x00FF0000,  // R mask
                0x0000FF00,  // G mask
                0x000000FF,  // B mask
                0x00000000); // A mask (unused)

            // Lock surfaces
            SDL_LockSurface(prevLevel);
            SDL_LockSurface(newMip);

            // Average 2x2 blocks to create mip level
            Uint32* srcPixels = (Uint32*)prevLevel->pixels;
            Uint32* dstPixels = (Uint32*)newMip->pixels;

            for (int y = 0; y < newHeight; y++) {
                for (int x = 0; x < newWidth; x++) {
                    // Get the four source pixels
                    Uint32 p00 = srcPixels[(y*2) * prevLevel->w + (x*2)];
                    Uint32 p10 = x*2+1 < prevLevel->w ? srcPixels[(y*2) * prevLevel->w + (x*2+1)] : p00;
                    Uint32 p01 = y*2+1 < prevLevel->h ? srcPixels[(y*2+1) * prevLevel->w + (x*2)] : p00;
                    Uint32 p11 = (x*2+1 < prevLevel->w && y*2+1 < prevLevel->h) ? 
                                srcPixels[(y*2+1) * prevLevel->w + (x*2+1)] : p00;

                    // Average color components directly
                    Uint8 r = (getR(p00) + getR(p10) + getR(p01) + getR(p11)) >> 2;
                    Uint8 g = (getG(p00) + getG(p10) + getG(p01) + getG(p11)) >> 2;
                    Uint8 b = (getB(p00) + getB(p10) + getB(p01) + getB(p11)) >> 2;

                    dstPixels[y * newMip->w + x] = packRGB(r, g, b);
                }
            }

            // Unlock surfaces
            SDL_UnlockSurface(prevLevel);
            SDL_UnlockSurface(newMip);

            // Add to mipmap chain
            m_mipmaps.push_back(newMip);

            // Prepare for next iteration
            prevLevel = newMip;
            width = newWidth;
            height = newHeight;
        }
    }

    void cleanup() {
        // Free all mip levels except base level (which is managed elsewhere)
        for (size_t i = 1; i < m_mipmaps.size(); i++) {
            if (m_mipmaps[i]) {
                SDL_FreeSurface(m_mipmaps[i]);
            }
        }
        m_mipmaps.clear();
    }

private:
    std::vector<SDL_Surface*> m_mipmaps;

    // Helper function to clamp a float value between 0 and 1
    static float clamp(float value, float min, float max) {
        return value < min ? min : (value > max ? max : value);
    }
};
}