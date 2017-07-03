# C++ Software Renderer/Rasterizer

This project implements a C++ software renderer/rasterizer with vertex- and
pixel shader support.

## Features
* Generic vertex arrays for arbitrary data in the vertex processing stage.
* Internal vertex cache for better vertex processing.
* Affine and perspective correct per vertex parameter interpolation.
* Vertex and pixel shaders written in C++ using some C++ template magic.

## Example

```c++

#include "Renderer.h"

// Define a pixel shader
class PixelShader : public PixelShaderBase<PixelShader> {
public:
	static const bool InterpolateZ = true;
	static const bool InterpolateW = true;

    // Number of affine/linear variables used.
	static const int AVarCount = 3;

    // Number of perspective correct variables used.
	static const int PVarCount = 2;

	static void drawPixel(const PixelData &p)
	{
		...
	}
};

// Define a vertex shader
class VertexShader : public VertexShaderBase<VertexShader> {
public:
    // Number of input vertex attributes used.
	static const int AttribCount = 1;

	static void processVertex(VertexShaderInput in, VertexShaderOutput *out)
	{
        // Write result to out.
		...
	}
};

// Use the renderer
Rasterizer r;
VertexProcessor v(&r);

r.setRasterMode(RasterMode::Span);
r.setScissorRect(0, 0, 640, 480);
r.setPixelShader<PixelShader>();

v.setViewport(0, 0, 640, 480);
v.setCullMode(CullMode::CW);
v.setVertexShader<VertexShader>();

// Draw
v.setVertexAttribPointer(0, sizeof(VertexData), vertexData);
v.drawElements(DrawMode::Triangle, indexData.size(), indexData);

```

## License

This code is licensed under the MIT License (see [LICENSE](LICENSE)).