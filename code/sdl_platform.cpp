#include "rt_weekend.h"

#include <SDL.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <dlfcn.h>

#include "sdl_platform.h"

const char *WindowTitle = "Ray Tracing in a Weekend";

const uint32_t TARGET_FRAME_RATE = 10;
const uint32_t TICKS_PER_FRAME = 1000 / TARGET_FRAME_RATE;
const uint32_t WINDOW_WIDTH = 1000;
const uint32_t WINDOW_HEIGHT = 500;

global_variable bool Running = true;
global_variable sdl_offscreen_buffer GlobalBackbuffer;

void PluginUpdateAndRenderStub(plugin_offscreen_buffer *Buffer, plugin_input *Input)
{
}

internal sdl_window_dimension
SDLGetWindowDimension(SDL_Window *Window)
{
    sdl_window_dimension Result;
    SDL_GetWindowSize(Window, &Result.Width, &Result.Height);

    return(Result);
}

internal void
SDLResizeTexture(sdl_offscreen_buffer *Buffer,
                 SDL_Renderer *Renderer,
                 int Width, int Height)
{
    const int BytesPerPixel = 4;

    if(Buffer->Memory)
    {
        munmap(Buffer->Memory,
               Buffer->Height * Buffer->Pitch);
    }

    if(Buffer->Texture)
    {
        SDL_DestroyTexture(Buffer->Texture);
    }

    Buffer->Texture = SDL_CreateTexture(Renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      Width, Height);
    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->Pitch = Width * BytesPerPixel;
    Buffer->Memory = mmap(0, Width * Height * BytesPerPixel,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1, 0);
}

internal void
SDLUpdateWindow(SDL_Renderer *Renderer,
                sdl_offscreen_buffer Buffer)
{
    SDL_UpdateTexture(Buffer.Texture,
                      0,
                      Buffer.Memory,
                      Buffer.Pitch);

    SDL_RenderCopy(Renderer, Buffer.Texture, 0, 0);

    SDL_RenderPresent(Renderer);
}

internal void
HandleEvent(SDL_Event *Event, plugin_input *Input)
{
    switch(Event->type)
    {
        case SDL_QUIT:
        {
            Running = false;
        } break;

        case SDL_WINDOWEVENT:
        {
            switch (Event->window.event)
            {
                case SDL_WINDOWEVENT_EXPOSED:
                {
                    SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
                    SDL_Renderer *Renderer = SDL_GetRenderer(Window);
                    SDLUpdateWindow(Renderer, GlobalBackbuffer);
                } break;
            }
        }

        case SDL_MOUSEMOTION:
        {
            // printf("We got a motion event.\n");
            // printf("Current mouse position is: (%d, %d)\n",
            //        Event->motion.x,
            //        Event->motion.y);
            Input->MouseX = Event->motion.x;
            Input->MouseY = Event->motion.y;

        } break;

        case SDL_MOUSEBUTTONDOWN:
        {
            Input->MouseDown = true;
        } break;

        case SDL_MOUSEBUTTONUP:
        {
            Input->MouseDown = false;
        } break;

        default:
        {
            // printf("Unhandled event!\n");
        } break;
    }
}

inline time_t
SDLGetLastModificationTime(const char *Filename)
{
    time_t LastModificationTime = 0;

    // Find out plugin library's last modification time
    struct stat LibraryFileStat;
    if (stat(Filename, &LibraryFileStat) == 0)
    {
        LastModificationTime = LibraryFileStat.st_ctime;
    }
    else
    {
        printf("Could not read plugin library: %s\n", strerror(errno));
    }

    return(LastModificationTime);
}

internal sdl_plugin_code
SDLLoadPluginCode(const char *Filename)
{
    sdl_plugin_code Result = {};
    Result.UpdateAndRender = PluginUpdateAndRenderStub;

    Result.LastModificationTime = SDLGetLastModificationTime(Filename);

    const char *error;

    /* Load library */
    Result.CodeLibrary = dlopen(Filename, RTLD_LAZY);
    if (Result.CodeLibrary)
    {
        dlerror();
        Result.UpdateAndRender =
          (plugin_update_and_render *)dlsym(Result.CodeLibrary,
                                          "PluginUpdateAndRender");
        if ((error = dlerror()))
        {
            fprintf(stderr, "Couldn't find PluginUpdateAndRender: %s\n", error);
        }
    }
    else
    {
        fprintf(stderr, "Couldn't open plugin library: %s\n",
                dlerror());
    }

    return(Result);
}

internal void
SDLUnloadPluginCode(sdl_plugin_code *Plugin)
{
    if(Plugin->CodeLibrary)
    {
        dlclose(Plugin->CodeLibrary);
    }

    Plugin->UpdateAndRender = PluginUpdateAndRenderStub;
}

int main()
{
    // Variables required to calculate framerate
    #define FPS_INTERVAL 1.0
    uint32_t LastFrameEndTime = SDL_GetTicks();
    uint32_t CurrentFPS = 0;
    uint32_t FramesElapsed = 0;
    uint32_t FramesMissed = 0;

    // Initialize SDL
    SDL_Window *Window = NULL;
    SDL_Renderer *Renderer = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create window
    Window = SDL_CreateWindow(WindowTitle,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_OPENGL);
    if (Window == NULL)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);

    if (Renderer == NULL)
    {
        printf("Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    // Create texture
    sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
    SDLResizeTexture(&GlobalBackbuffer, Renderer, Dimension.Width, Dimension.Height);

    // Load plugin code
    const char *PluginLibraryFilename = "librt_weekend.so";
    sdl_plugin_code Plugin = SDLLoadPluginCode(PluginLibraryFilename);

    // Keep track of input
    plugin_input Input = {};

    bool FirstFrame = true;
    uint32_t LastRenderedTime = 0;
    bool ForceRenderFrame = false;

    // Main loop
    while (Running)
    {
        uint32_t FrameStartTime = SDL_GetTicks();

        // Live reloading
        time_t NewLastModificationTime = SDLGetLastModificationTime(PluginLibraryFilename);
        if(NewLastModificationTime > Plugin.LastModificationTime)
        {
            SDLUnloadPluginCode(&Plugin);
            Plugin = SDLLoadPluginCode(PluginLibraryFilename);
            ForceRenderFrame = true;
        }

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            HandleEvent(&Event, &Input);
        }

        // Display rendering information in Window title
        char WindowTitleBuffer[512];
        sprintf(WindowTitleBuffer,
                "%s - %d x %d - %u fps - %d frames missed - Last rendered: %d",
                WindowTitle,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                CurrentFPS,
                FramesMissed,
                LastRenderedTime);

        SDL_SetWindowTitle(Window, WindowTitleBuffer);

        plugin_offscreen_buffer Buffer = {};
        Buffer.Memory = GlobalBackbuffer.Memory;
        Buffer.Width = GlobalBackbuffer.Width;
        Buffer.Height = GlobalBackbuffer.Height;
        Buffer.Pitch = GlobalBackbuffer.Pitch;

        if (FirstFrame || ForceRenderFrame || Input.MouseDown)
        {
            LastRenderedTime = SDL_GetTicks();
            Plugin.UpdateAndRender(&Buffer, &Input);
            ForceRenderFrame = false;
        }
        FirstFrame = false;

        SDLUpdateWindow(Renderer, GlobalBackbuffer);

        uint32_t FrameEndTime = SDL_GetTicks();
        // Calculate framerate
        if (LastFrameEndTime < FrameEndTime - FPS_INTERVAL * 1000)
        {
            LastFrameEndTime = FrameEndTime;
            CurrentFPS = FramesElapsed;
            FramesElapsed = 0;
        }
        FramesElapsed++;

        // Cap framerate
        uint32_t FrameTicks = FrameEndTime - FrameStartTime;

        if (FrameTicks < TICKS_PER_FRAME)
        {
            // Wait remaining time
            SDL_Delay(TICKS_PER_FRAME - FrameTicks);
        }
        else
        {
            FramesMissed++;
        }
    }

    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();
    return 0;
}
