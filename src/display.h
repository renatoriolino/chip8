#pragma once

#include <array>
#include <cstdint>

#include <iostream>

#include <SDL2/SDL.h>

class Display
{
protected:
    uint16_t width;
    uint16_t height;
    uint8_t scale;

    virtual void DrawPixel(uint16_t x, uint16_t y, uint32_t color) = 0;
    virtual void Present() = 0;

public:
    Display(uint16_t w, uint16_t h, uint8_t s) : width{w}, height{h}, scale{s} {};

    virtual uint16_t GetW() { return width; };
    virtual uint16_t GetH() { return height; };

    template<class InputIt>
    void Draw(InputIt first, InputIt last)
    {
        uint16_t wcnt = 0;
        uint16_t hcnt = 0;
        for (auto i = first; i != last && hcnt < height; ++i)
        {
            auto v = *i;
            for (int8_t b = 7; b>=0; --b)
            {
                DrawPixel(wcnt, hcnt, ((v>>b) & 0x1) ? 0xffffff : 0x0);
                if (++wcnt >= width)
                {
                    wcnt = 0;
                    hcnt += 1;
                }
            }
        }

        Present();
    };

    void Clear()
    {
        for (auto x=0; x<width; ++x)
            for (auto y=0; y<height; ++y)
                DrawPixel(x, y, 0x0);
        Present();
    }
};

class DisplaySDL : public Display
{
protected:
    SDL_Window * window;
    SDL_Surface * surface;

    virtual void DrawPixel(uint16_t x, uint16_t y, uint32_t color)
    {
        SDL_Rect rect{
            .x = x * scale,
            .y = y * scale,
            .w = scale,
            .h = scale
        };
        SDL_FillRect(surface, &rect, color);
    }

    virtual void Present()
    {
        SDL_UpdateWindowSurface(window);
    }

public:
    DisplaySDL(uint16_t w, uint16_t h, uint16_t s) : Display(w, h, s), window(NULL), surface(NULL)
    {
        window = SDL_CreateWindow("display", 0, 0, width * scale, height * scale, SDL_WINDOW_SHOWN | SDL_WINDOW_MOUSE_FOCUS);
        if (window != NULL)
            surface = SDL_GetWindowSurface(window);
    }

    virtual ~DisplaySDL()
    {
        surface = NULL;
        if (window != NULL)
            SDL_DestroyWindow(window);
        window = NULL;
    }
};