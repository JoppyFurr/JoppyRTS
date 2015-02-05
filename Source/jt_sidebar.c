
#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

#include "jt_texture.h"

void jt_sidebar_render (SDL_Renderer *renderer)
{
    int screen_width;
    int screen_height;

    SDL_Rect src_rect;
    SDL_Rect dest_rect;

    SDL_GetRendererOutputSize(renderer, &screen_width, &screen_height);
    jt_rts_textures *rts_textures = jt_rts_textures_get ();

    /* Clear */
    for (int y = 0; y < screen_height ; y += 66)
    {
        src_rect = (SDL_Rect) { 0, 391, 256, 66 };
        dest_rect = (SDL_Rect) { screen_width - 256, y, 256, 66 };
        SDL_RenderCopy (renderer,
                        rts_textures->sidebar,
                        &src_rect,
                        &dest_rect);
    }

    /* Map and buttons */
    src_rect = (SDL_Rect) { 0, 0, 256, 324 };
    dest_rect = (SDL_Rect) { screen_width - 256, 0, 256, 324 };
    SDL_RenderCopy (renderer,
                    rts_textures->sidebar,
                    &src_rect,
                    &dest_rect);

    /* Buy buttons */
    for (int i = 0; i < ((screen_height - 324) / 66) ; i++)
    {
        src_rect = (SDL_Rect) { 0, 325, 256, 66 };
        dest_rect = (SDL_Rect) { screen_width - 256, 324 + 66 * i, 256, 66 };
        SDL_RenderCopy (renderer,
                        rts_textures->sidebar,
                        &src_rect,
                        &dest_rect);

        if (i == 0)
        {
            /* Wall button */
            src_rect = (SDL_Rect) { 0, 0, 122, 64 };
            dest_rect = (SDL_Rect) { screen_width - 256 + 6, 325 + 66 * i, 122, 64 };
            SDL_RenderCopy (renderer,
                            rts_textures->icons,
                            &src_rect,
                            &dest_rect);
        }

        if (i == 1)
        {
            /* Tent button */
            src_rect = (SDL_Rect) { 122, 0, 122, 64 };
            dest_rect = (SDL_Rect) { screen_width - 256 + 6, 325 + 66 * i, 122, 64 };
            SDL_RenderCopy (renderer,
                            rts_textures->icons,
                            &src_rect,
                            &dest_rect);
        }
    }
}
