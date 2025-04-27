#pragma once

#include "SDL.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace swr {

class Texture {
public:
    static constexpr int MAX_ANISOTROPY = 16;
    
    Texture(SDL_Surface* baseSurface, int maxAnisotropy = 8) 
        : m_maxAnisotropy(std::clamp(maxAnisotropy, 1, MAX_ANISOTROPY)) 
    {
        if (!baseSurface) {
            throw std::runtime_error("Null base surface provided to Texture constructor");
        }
        if (!baseSurface->pixels) {
            throw std::runtime_error("Base surface has no pixel data");
        }
        generateMipmaps(baseSurface);
        std::cout << "Texture initialized with " << m_mipmaps.size() << " mip levels" << std::endl;
    }

    ~Texture() {
        cleanup();
    }

    // Prevent copying since we manage SDL resources
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void sample(float u, float v, float dudx, float dvdx, float dudy, float dvdy, Uint32& outColor) const {
        if (m_mipmaps.empty()) {
            outColor = 0;
            return;
        }

        // Safely wrap texture coordinates
        u = std::fmod(u, 1.0f);
        v = std::fmod(v, 1.0f);
        if (u < 0) u += 1.0f;
        if (v < 0) v += 1.0f;

        SDL_Surface* baseLevel = m_mipmaps[0];
        if (!baseLevel || !baseLevel->pixels) {
            std::cerr << "Invalid base level texture in sample()" << std::endl;
            outColor = 0;
            return;
        }
        
        // Compute ellipse axes
        float dudx_scaled = dudx * baseLevel->w;
        float dvdx_scaled = dvdx * baseLevel->h;
        float dudy_scaled = dudy * baseLevel->w;
        float dvdy_scaled = dvdy * baseLevel->h;
        
        float dx_len = std::sqrt(dudx_scaled * dudx_scaled + dvdx_scaled * dvdx_scaled);
        float dy_len = std::sqrt(dudy_scaled * dudy_scaled + dvdy_scaled * dvdy_scaled);
        
        dx_len = std::max(dx_len, 1e-6f);
        dy_len = std::max(dy_len, 1e-6f);

        float major_len = std::max(dx_len, dy_len);
        float minor_len = std::min(dx_len, dy_len);
        
        float ratio = std::min(major_len / minor_len, static_cast<float>(m_maxAnisotropy));
        int num_samples = std::max(1, static_cast<int>(std::ceil(ratio)));

        try {
            if (num_samples <= 1) {
                // Use regular trilinear filtering for low anisotropy
                sampleTrilinear(u, v, major_len, outColor);
                return;
            }

            // Determine major axis direction
            float major_du, major_dv;
            if (dx_len > dy_len) {
                major_du = dudx / dx_len;
                major_dv = dvdx / dx_len;
            } else {
                major_du = dudy / dy_len;
                major_dv = dvdy / dy_len;
            }

            // Accumulate color components
            float r = 0, g = 0, b = 0;
            float step = 1.0f / num_samples;
            
            for (int i = 0; i < num_samples; ++i) {
                float t = (i + 0.5f) * step - 0.5f;
                float sample_u = u + major_du * major_len * t;
                float sample_v = v + major_dv * major_len * t;
                
                sample_u = std::fmod(sample_u, 1.0f);
                sample_v = std::fmod(sample_v, 1.0f);
                if (sample_u < 0) sample_u += 1.0f;
                if (sample_v < 0) sample_v += 1.0f;

                Uint32 sample_color;
                sampleTrilinear(sample_u, sample_v, minor_len, sample_color);
                
                r += getR(sample_color);
                g += getG(sample_color);
                b += getB(sample_color);
            }
            
            r = std::min(255.0f, r / num_samples);
            g = std::min(255.0f, g / num_samples);
            b = std::min(255.0f, b / num_samples);
            
            outColor = packRGB(static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b));
        } catch (const std::exception& e) {
            std::cerr << "Exception in sample(): " << e.what() << std::endl;
            outColor = 0;
        }
    }

private:
    void sampleTrilinear(float u, float v, float rho, Uint32& outColor) const {
        if (m_mipmaps.empty()) {
            outColor = 0;
            return;
        }

        // Calculate mipmap level based on rho (pixel footprint)
        float lod = std::log2(std::max(rho, 1e-6f));
        lod = std::clamp(lod, 0.0f, static_cast<float>(m_mipmaps.size() - 1));
        
        int lodBase = static_cast<int>(std::floor(lod));
        int lodNext = std::min(lodBase + 1, static_cast<int>(m_mipmaps.size()) - 1);
        float lodFrac = std::clamp(lod - lodBase, 0.0f, 1.0f);

        Uint32 colorBase, colorNext;
        sampleBilinear(lodBase, u, v, colorBase);
        if (lodBase != lodNext) {
            sampleBilinear(lodNext, u, v, colorNext);
            outColor = lerpColors(colorBase, colorNext, lodFrac);
        } else {
            outColor = colorBase;
        }
    }

    void sampleBilinear(int mipLevel, float u, float v, Uint32& outColor) const {
        if (mipLevel < 0 || mipLevel >= m_mipmaps.size()) {
            std::cerr << "Invalid mip level " << mipLevel << " (max: " << m_mipmaps.size()-1 << ")" << std::endl;
            outColor = 0;
            return;
        }

        SDL_Surface* mip = m_mipmaps[mipLevel];
        if (!mip || !mip->pixels) {
            std::cerr << "Invalid mip level surface at level " << mipLevel << std::endl;
            outColor = 0;
            return;
        }

        float px = u * (mip->w - 1);
        float py = v * (mip->h - 1);

        int x0 = std::max(0, int(std::floor(px)));
        int y0 = std::max(0, int(std::floor(py)));
        int x1 = std::min(mip->w - 1, x0 + 1);
        int y1 = std::min(mip->h - 1, y0 + 1);

        float fx = px - x0;
        float fy = py - y0;

        Uint32* pixels = static_cast<Uint32*>(mip->pixels);
        if (!pixels) {
            std::cerr << "Null pixel data in mip level " << mipLevel << std::endl;
            outColor = 0;
            return;
        }

        Uint32 c00 = pixels[y0 * mip->w + x0];
        Uint32 c10 = pixels[y0 * mip->w + x1];
        Uint32 c01 = pixels[y1 * mip->w + x0];
        Uint32 c11 = pixels[y1 * mip->w + x1];

        outColor = bilinearLerpColors(c00, c10, c01, c11, fx, fy);
    }

    static inline Uint8 getR(Uint32 color) { return (color >> 16) & 0xFF; }
    static inline Uint8 getG(Uint32 color) { return (color >> 8) & 0xFF; }
    static inline Uint8 getB(Uint32 color) { return color & 0xFF; }

    static inline Uint32 packRGB(Uint8 r, Uint8 g, Uint8 b) {
        return (r << 16) | (g << 8) | b;
    }

    static inline Uint32 lerpColors(Uint32 c1, Uint32 c2, float t) {
        Uint8 r = getR(c1) + (Uint8)((getR(c2) - getR(c1)) * t);
        Uint8 g = getG(c1) + (Uint8)((getG(c2) - getG(c1)) * t);
        Uint8 b = getB(c1) + (Uint8)((getB(c2) - getB(c1)) * t);
        return packRGB(r, g, b);
    }

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
        cleanup();

        if (!baseTexture || !baseTexture->pixels) {
            throw std::runtime_error("Invalid base texture in generateMipmaps");
        }

        // Copy base texture
        SDL_Surface* baseCopy = SDL_CreateRGBSurface(0, baseTexture->w, baseTexture->h, 32,
            0x00FF0000,  // R mask
            0x0000FF00,  // G mask
            0x000000FF,  // B mask
            0x00000000); // A mask

        if (!baseCopy) {
            throw std::runtime_error(std::string("Failed to create base texture copy: ") + SDL_GetError());
        }

        SDL_BlitSurface(baseTexture, NULL, baseCopy, NULL);
        m_mipmaps.push_back(baseCopy);

        SDL_Surface* prevLevel = baseCopy;
        int width = baseTexture->w;
        int height = baseTexture->h;

        // Generate mip chain until 1x1
        while (width > 1 || height > 1) {
            int newWidth = std::max(1, width / 2);
            int newHeight = std::max(1, height / 2);

            SDL_Surface* newMip = SDL_CreateRGBSurface(0, newWidth, newHeight, 32,
                0x00FF0000,  // R mask
                0x0000FF00,  // G mask
                0x000000FF,  // B mask
                0x00000000); // A mask

            if (!newMip) {
                std::cerr << "Failed to create mip surface: " << SDL_GetError() << std::endl;
                break;
            }

            if (SDL_LockSurface(prevLevel) < 0 || SDL_LockSurface(newMip) < 0) {
                SDL_FreeSurface(newMip);
                throw std::runtime_error("Failed to lock surfaces for mipmap generation");
            }

            Uint32* srcPixels = static_cast<Uint32*>(prevLevel->pixels);
            Uint32* dstPixels = static_cast<Uint32*>(newMip->pixels);

            for (int y = 0; y < newHeight; y++) {
                for (int x = 0; x < newWidth; x++) {
                    Uint32 p00 = srcPixels[(y*2) * prevLevel->w + (x*2)];
                    Uint32 p10 = x*2+1 < prevLevel->w ? srcPixels[(y*2) * prevLevel->w + (x*2+1)] : p00;
                    Uint32 p01 = y*2+1 < prevLevel->h ? srcPixels[(y*2+1) * prevLevel->w + (x*2)] : p00;
                    Uint32 p11 = (x*2+1 < prevLevel->w && y*2+1 < prevLevel->h) ? 
                                srcPixels[(y*2+1) * prevLevel->w + (x*2+1)] : p00;

                    Uint8 r = (getR(p00) + getR(p10) + getR(p01) + getR(p11)) >> 2;
                    Uint8 g = (getG(p00) + getG(p10) + getG(p01) + getG(p11)) >> 2;
                    Uint8 b = (getB(p00) + getB(p10) + getB(p01) + getB(p11)) >> 2;

                    dstPixels[y * newMip->w + x] = packRGB(r, g, b);
                }
            }

            SDL_UnlockSurface(prevLevel);
            SDL_UnlockSurface(newMip);

            m_mipmaps.push_back(newMip);

            prevLevel = newMip;
            width = newWidth;
            height = newHeight;
        }

        std::cout << "Generated " << m_mipmaps.size() << " mipmap levels" << std::endl;
    }

    void cleanup() {
        for (SDL_Surface* mip : m_mipmaps) {
            if (mip) {
                SDL_FreeSurface(mip);
            }
        }
        m_mipmaps.clear();
    }

    std::vector<SDL_Surface*> m_mipmaps;
    int m_maxAnisotropy;
};

} // namespace swr