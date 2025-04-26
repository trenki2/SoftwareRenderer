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

#pragma once

/** @file */

#include "IRasterizer.h"
#include "TriangleEquations.h"

namespace swr {

#pragma warning(push)
#pragma warning(disable: 26495) // variable in uninitialized

/// PixelData passed to the pixel shader for display.
struct PixelData {
    int x; ///< The x coordinate.
    int y; ///< The y coordinate.

    float z; ///< The interpolated z value.
    float w; ///< The interpolated w value.
    float invw; ///< The interpolated 1 / w value.
    
    /// Affine variables.
    float avar[MaxAVars];
    
    /// Perspective variables.
    float pvar[MaxPVars];
    
    // Used internally.
    float pvarTemp[MaxPVars];

    // Triangle equations needed for derivative computation
    const TriangleEquations* equations;

    PixelData() : equations(nullptr) {}

    // Initialize pixel data for the given pixel coordinates.
    void init(const TriangleEquations &eqn, float x, float y, int aVarCount, int pVarCount, bool interpolateZ, bool interpolateW)
    {
        equations = &eqn;
        if (interpolateZ)
            z = eqn.z.evaluate(x, y);
        
        if (interpolateW || pVarCount > 0) 
        {
            invw = eqn.invw.evaluate(x, y);
            w = 1.0f / invw;
        }

        for (int i = 0; i < aVarCount; ++i)
            avar[i] = eqn.avar[i].evaluate(x, y);
        
        for (int i = 0; i < pVarCount; ++i)
        {
            pvarTemp[i] = eqn.pvar[i].evaluate(x, y);
            pvar[i] = pvarTemp[i] * w;
        }
    }

    // Step all the pixel data in the x direction.
    void stepX(const TriangleEquations &eqn, int aVarCount, int pVarCount, bool interpolateZ, bool interpolateW)
    {
        if (interpolateZ)
            z = eqn.z.stepX(z);
        
        if (interpolateW || pVarCount > 0) 
        {
            invw = eqn.invw.stepX(invw);
            w = 1.0f / invw;
        }

        for (int i = 0; i < aVarCount; ++i)
            avar[i] = eqn.avar[i].stepX(avar[i]);

        for (int i = 0; i < pVarCount; ++i)
        {
            pvarTemp[i] = eqn.pvar[i].stepX(pvarTemp[i]);            
            pvar[i] = pvarTemp[i] * w;
        }
    }

    // Step all the pixel data in the y direction.
    void stepY(const TriangleEquations &eqn, int aVarCount, int pVarCount, bool interpolateZ, bool interpolateW)
    {
        if (interpolateZ)
            z = eqn.z.stepY(z);
        
        if (interpolateW || pVarCount > 0) 
        {
            invw = eqn.invw.stepY(invw);
            w = 1.0f / invw;
        }

        for (int i = 0; i < aVarCount; ++i)
            avar[i] = eqn.avar[i].stepY(avar[i]);

        for (int i = 0; i < pVarCount; ++i)
        {
            pvarTemp[i] = eqn.pvar[i].stepY(pvarTemp[i]);            
            pvar[i] = pvarTemp[i] * w;
        }
    }

    // Get derivatives for perspective-correct variables
    void computePerspectiveDerivatives(const TriangleEquations &eqn, int varIndex, float &ddx, float &ddy) const 
    {
        // Get current interpolated values
        float var = eqn.pvar[varIndex].evaluate(x + 0.5f, y + 0.5f);
        float curInvw = eqn.invw.evaluate(x + 0.5f, y + 0.5f);

        // Get partial derivatives of variable and 1/w
        float dvar_dx = eqn.pvar[varIndex].a;
        float dvar_dy = eqn.pvar[varIndex].b;
        float dinvw_dx = eqn.invw.a;
        float dinvw_dy = eqn.invw.b;

        // Use quotient rule to compute d(var/invw)/dx and d(var/invw)/dy
        // d(v/w)/dx = (w * dv/dx - v * dw/dx) / w^2
        float w = 1.0f / curInvw;
        ddx = (curInvw * dvar_dx - var * dinvw_dx) / (curInvw * curInvw);
        ddy = (curInvw * dvar_dy - var * dinvw_dy) / (curInvw * curInvw);
    }
};

#pragma warning(pop)

} // end namespace swr