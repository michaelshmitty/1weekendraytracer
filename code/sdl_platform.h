#ifndef PLATFORM_H
#define PLATFORM_H

typedef void plugin_update_and_render(plugin_offscreen_buffer *Buffer);

struct sdl_plugin_code
{
    time_t LastModificationTime;
    void *CodeLibrary;
    plugin_update_and_render *UpdateAndRender;
};

struct sdl_offscreen_buffer
{
    SDL_Texture *Texture;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct sdl_window_dimension
{
    int Width;
    int Height;
};

#endif
