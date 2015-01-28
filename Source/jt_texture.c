#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "jt_texture.h"

static jt_rts_textures _rts_textures;

SDL_Texture *jt_load_texture (SDL_Renderer *renderer, char *filename)
{
    SDL_Surface *surface = IMG_Load(filename);

    if (surface == NULL) {
        fprintf (stderr, "Error: IMG_Load failed (%s).\n", filename);
        exit (EXIT_FAILURE);
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface (renderer, surface);
    SDL_FreeSurface (surface);

    if (texture == NULL)
    {
        fprintf (stderr, "Error: SDL_CreateTextureFromSurface failed (%s).\n", filename);
    }

    return texture;
}

int jt_rts_textures_load (SDL_Renderer *renderer)
{
    _rts_textures.sidebar            = jt_load_texture (renderer, "./Media/Sidebar.png");
    _rts_textures.grass              = jt_load_texture (renderer, "./Media/Grass.png");
    _rts_textures.unit               = jt_load_texture (renderer, "./Media/Unit.png");
    _rts_textures.wall               = jt_load_texture (renderer, "./Media/Wall.png");
    _rts_textures.icons              = jt_load_texture (renderer, "./Media/Icons.png");
    _rts_textures.placement          = jt_load_texture (renderer, "./Media/Placement.png");
    return 0;
}

int jt_rts_textures_free ()
{
    SDL_DestroyTexture (_rts_textures.sidebar);
    SDL_DestroyTexture (_rts_textures.grass);
    SDL_DestroyTexture (_rts_textures.unit);
    SDL_DestroyTexture (_rts_textures.wall);
    SDL_DestroyTexture (_rts_textures.icons);
    SDL_DestroyTexture (_rts_textures.placement);
    return 0;
}

jt_rts_textures *jt_rts_textures_get ()
{
    return &_rts_textures;
}
