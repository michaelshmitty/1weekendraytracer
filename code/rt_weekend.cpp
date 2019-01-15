#include "rt_weekend.h"
#include "ray.h"

bool hit_sphere(const vec3& center, float radius, const ray& r)
{
    vec3 oc = r.origin() - center;
    float a = dot(r.direction(), r.direction());
    float b = 2.0 * dot(oc, r.direction());
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    return (discriminant > 0);
}

vec3 color(const ray& r)
{
    if (hit_sphere(vec3(0, 0, -1), 0.5, r))
        return vec3(1, 0, 0);
    vec3 unit_direction = unit_vector(r.direction());
    float t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
}

extern "C"
void
PluginUpdateAndRender(plugin_offscreen_buffer *Buffer)
{
    int Pitch = Buffer->Pitch;
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0;
            X < Buffer->Width;
            ++X)
        {
            float r = float(X) / float(Buffer->Width);
            float g = float(Y) / float(Buffer->Height);
            float b = 0.2;
            int ig = int(255.99 * r);
            int ir = int(255.99 * g);
            int ib = int(255.99 * b);

            *Pixel++ = (ir << 24) + (ig << 16) + (ib << 8) + 0xFF;
        }

        Row += Pitch;
    }
}
