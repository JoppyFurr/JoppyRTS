typedef struct jt_rts_textures_struct
{
    SDL_Texture *sidebar;
    SDL_Texture *grass;
    SDL_Texture *unit;
    SDL_Texture *selector;
    SDL_Texture *wall;
    SDL_Texture *icons;
    SDL_Texture *placement;
    SDL_Texture *tent;
} jt_rts_textures;

SDL_Texture *loadTexture(SDL_Renderer *renderer, char *filename);

int jt_rts_textures_load (SDL_Renderer *renderer);
int jt_rts_textures_free ();
jt_rts_textures *jt_rts_textures_get ();
