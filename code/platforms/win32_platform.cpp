#define _CRT_SECURE_NO_WARNINGS 1
#define SDL_MAIN_HANDLED
#include "../rt_weekend.h"

#include <windows.h>
#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "win32_platform.h"

const char *WindowTitle = "Ray Tracing in a Weekend";

const uint32_t TARGET_FRAME_RATE = 10;
const uint32_t TICKS_PER_FRAME = 1000 / TARGET_FRAME_RATE;
const uint32_t WINDOW_WIDTH = 500;
const uint32_t WINDOW_HEIGHT = 250;

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
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
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
    Buffer->Memory = VirtualAlloc(0, Width * Height * BytesPerPixel,
                                  MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
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
HandleEvent(SDL_Event *Event)
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
    }
}

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return LastWriteTime;
}

internal sdl_plugin_code
SDLLoadPluginCode(char *SourceDLLName, char *TempDLLName)
{
    sdl_plugin_code Result = {};

    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    CopyFile(SourceDLLName, TempDLLName, FALSE);

    Result.PluginCodeDLL = LoadLibraryA(TempDLLName);
    if (Result.PluginCodeDLL)
    {
        Result.UpdateAndRender = (plugin_update_and_render *)
            GetProcAddress(Result.PluginCodeDLL, "PluginUpdateAndRender");
        Result.IsValid = Result.UpdateAndRender;
    }
    Assert(Result.IsValid);


    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
    }

    return Result;
}

internal void
SDLUnloadPluginCode(sdl_plugin_code *PluginCode)
{
    if (PluginCode->PluginCodeDLL)
    {
        FreeLibrary(PluginCode->PluginCodeDLL);
        PluginCode->PluginCodeDLL = 0;
    }

    PluginCode->IsValid = false;
    PluginCode->UpdateAndRender = 0;
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
    sdl_plugin_code Plugin = SDLLoadPluginCode("rt_weekend.dll", "rt_weekend_temp.dll");

    // Main loop
    while (Running)
    {
        uint32_t FrameStartTime = SDL_GetTicks();

        // Live reloading
        FILETIME NewDLLWriteTime = Win32GetLastWriteTime("rt_weekend.dll");
        if (CompareFileTime(&NewDLLWriteTime, &Plugin.DLLLastWriteTime) != 0)
        {
            SDLUnloadPluginCode(&Plugin);
            Plugin = SDLLoadPluginCode("rt_weekend.dll", "rt_weekend_temp.dll");
        }

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            HandleEvent(&Event);
        }

        // Display rendering information in Window title
        char WindowTitleBuffer[512];
        sprintf(WindowTitleBuffer,
                "%s - %d x %d - %u fps - %d frames missed",
                WindowTitle,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                CurrentFPS,
                FramesMissed);

        SDL_SetWindowTitle(Window, WindowTitleBuffer);

        plugin_offscreen_buffer Buffer = {};
        Buffer.Memory = GlobalBackbuffer.Memory;
        Buffer.Width = GlobalBackbuffer.Width;
        Buffer.Height = GlobalBackbuffer.Height;
        Buffer.Pitch = GlobalBackbuffer.Pitch;
        Plugin.UpdateAndRender(&Buffer);

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
