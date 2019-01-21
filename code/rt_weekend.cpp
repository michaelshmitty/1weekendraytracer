#include <float.h>
#include "rt_weekend.h"
#include "sphere.h"
#include "target_list.h"
#include "camera.h"

vec3 color(const ray &r, target *world)
{
    hit_record rec;
    if (world->hit(r, 0.0, MAXFLOAT, rec))
    {
        return 0.5 * vec3(rec.normal.x() + 1, rec.normal.y() + 1, rec.normal.z() + 1);
    }
    else
    {
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
    }
}

extern "C"
void
PluginUpdateAndRender(plugin_offscreen_buffer *Buffer)
{
    int ns = 1;
    int Pitch = Buffer->Pitch;
    uint8_t *Row = (uint8_t *)Buffer->Memory;

    target *list[2];
    list[0] = new sphere(vec3(0, 0, -1), 0.5);
    list[1] = new sphere(vec3(0, -100.5, -1), 100);
    target *world = new target_list(list, 2);
    camera cam;

    for(int j = Buffer->Height - 1; j >= 0; j--)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int i = 0; i < Buffer->Width; i++)
        {
            vec3 col(0, 0, 0);
            for (int s = 0; s < ns; s++)
            {
                float u = float(i + drand48()) / float(Buffer->Width);
                float v = float(j + drand48()) / float(Buffer->Height);
                ray r = cam.get_ray(u, v);
                vec3 p = r.point_at_parameter(2.0);
                col += color(r, world);
            }
            col /= float(ns);
            int ir = int(255.99 * col[0]);
            int ig = int(255.99 * col[1]);
            int ib = int(255.99 * col[2]);

            *Pixel++ = (ir << 24) + (ig << 16) + (ib << 8) + 0xFF;
        }

        Row += Pitch;
    }
}
