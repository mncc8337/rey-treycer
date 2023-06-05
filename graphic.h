// this file contain method to create window and draw pixel to it

#pragma once
#include <SDL2/SDL.h>
#include "vec3.h"
#include "constant.h"

class SDL {
public:
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Texture* texture;

    unsigned char* pixels;
    int pitch;

    SDL(char* title) {   
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    }
    // graphics
    void enable_drawing() {
        SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
    }
    void draw_pixel(int x, int y, Vec3 color) {
        pixels[y * pitch + x * 4 + 0] = color.z * 255;
        pixels[y * pitch + x * 4 + 1] = color.y * 255;
        pixels[y * pitch + x * 4 + 2] = color.x * 255;
        pixels[y * pitch + x * 4 + 3] = 255;
    }
    void disable_drawing() {
        SDL_UnlockTexture(texture);
        SDL_Rect rect;
        rect.x = 0; rect.y = 0;
        rect.w = WIDTH; rect.h = HEIGHT;
        SDL_RenderCopy(renderer, texture, NULL, &rect);

    }
    void swap_buffer() {
        SDL_RenderPresent(renderer);
    }
};
