#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
SDL_Surface *loadImage(char *filename)
{
    SDL_Surface *initialImage;

    initialImage = IMG_Load(filename);

    if(initialImage == NULL) {
        fprintf(stderr, "Error: IMG_Load failed (%s).\n", filename);
        exit(EXIT_FAILURE);
    }

    return initialImage;
}

SDL_Texture *loadTexture (SDL_Renderer *renderer, char *filename)
{
    SDL_Surface *surface = loadImage (filename);
    SDL_Texture *texture = SDL_CreateTextureFromSurface (renderer, surface);
    SDL_FreeSurface (surface);

    if (texture == NULL)
    {
        fprintf (stderr, "Error: SDL_CreateTextureFromSurface failed (%s).\n", filename);
    }

    return texture;
}
