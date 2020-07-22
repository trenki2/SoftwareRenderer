#include "Renderer.h"
#include "Random.h"
#include <vector>
#include <chrono>
#include <iostream>
using namespace swr;

struct VertexData
{
    float x, y, z;
    float r, g, b;
};

struct PixelShader : public PixelShaderBase<PixelShader>
{
    static const int AVarCount = 3;

    static std::vector<int> buffer;
    static int width;
    static int height;

    static void drawPixel(const PixelData& p)
    {
        buffer[p.x + width * p.y] = 1;
    }
};

std::vector<int> PixelShader::buffer;
int PixelShader::width;
int PixelShader::height;

struct VertexShader : public VertexShaderBase<VertexShader>
{
    static const int AttribCount = 1;
    static const int AVarCount = 3;
    static const int PVarCount = 0;

    static void processVertex(VertexShaderInput in, VertexShaderOutput* out)
    {
        const VertexData* data = static_cast<const VertexData*>(in[0]);
        out->x = data->x;
        out->y = data->y;
        out->z = data->z;
        out->w = 1.0f;
        out->avar[0] = data->r;
        out->avar[1] = data->g;
        out->avar[2] = data->b;
    }
};

class Benchmark {
private: 
    VertexData CreateVertex(Random& random)
    {
        VertexData vertex;
        vertex.x = (float)random.NextDouble();
        vertex.y = (float)random.NextDouble();
        vertex.z = (float)random.NextDouble();
        vertex.r = (float)random.NextDouble();
        vertex.g = (float)random.NextDouble();
        vertex.b = (float)random.NextDouble();
        return vertex;
    }

public:
    void Run()
    {
        PixelShader::buffer.resize(640 * 480);
        PixelShader::width = 640;
        PixelShader::height = 480;

        Rasterizer r;
        VertexProcessor v(&r);

        r.setScissorRect(0, 0, 640, 480);
        v.setViewport(0, 0, 640, 480);
        v.setCullMode(CullMode::None);

        std::vector<int> indices;
        std::vector<VertexData> vertices;

        Random random(0);

        for (int i = 0; i < 4096 * 10; i++)
        {
            auto offset = vertices.size();

            vertices.push_back(CreateVertex(random));
            vertices.push_back(CreateVertex(random));
            vertices.push_back(CreateVertex(random));

            indices.push_back(offset + 0);
            indices.push_back(offset + 1);
            indices.push_back(offset + 2);
        }

        r.setPixelShader<PixelShader>();
        v.setVertexShader<VertexShader>();
        v.setVertexAttribPointer(0, sizeof(VertexData), &vertices[0]);

        auto start = std::chrono::steady_clock::now();
        v.drawElements(DrawMode::Triangle, indices.size(), &indices[0]);
        auto end = std::chrono::steady_clock::now();
        std::cout << "Elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    }
};

int main(int argc, char* argv[])
{
    Benchmark b;
    b.Run();
}