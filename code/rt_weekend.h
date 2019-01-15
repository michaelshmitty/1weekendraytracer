#ifndef RT_WEEKEND_H
#define RT_WEEKEND_H

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }

// Services that are being provided to the platform layer

struct plugin_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

#endif
