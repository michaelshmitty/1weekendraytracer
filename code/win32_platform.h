#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

typedef void plugin_update_and_render(plugin_offscreen_buffer *Buffer);

struct sdl_plugin_code
{
    HMODULE PluginCodeDLL;
    FILETIME DLLLastWriteTime;
    plugin_update_and_render *UpdateAndRender;
    bool IsValid;
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
