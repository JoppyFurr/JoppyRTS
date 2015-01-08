#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "jt_helpers.h"

#define FRAME_LIMIT 60

/* Notes:
 *  - The game is played on a grid. For now, lets just use 32x32 pixel blocks for the grid.
 */

typedef struct jt_unit_struct
{
    int selected;   /* Bool - Is this unit selected */
    double x_position; /* In grid-space */
    double y_position; /* In grid-space */
    double x_goal;
    double y_goal;
} jt_unit;

void jt_select_units (int x, int y, jt_unit *units)
{
    for (int i = 0; i < 5; i++) /* Currently, five test-units */
    {
        if ( x > (units[i].x_position * 32.0) && x < (units[i].x_position * 32.0 + 32.0) &&
             y > (units[i].y_position * 32.0) && y < (units[i].y_position * 32.0 + 32.0))
        {
            units[i].selected = 1;
        }
        else
        {
            units[i].selected = 0;
        }

    }
}

void jt_move_units (int x, int y, jt_unit *units)
{
    for (int i = 0; i < 5; i++) /* Currently, five test-units */
    {
        if ( units[i].selected )
        {
            units[i].x_goal = x / 32.0;
            units[i].y_goal = y / 32.0;
        }
    }
}

int jt_run_game (SDL_Renderer *renderer)
{
    SDL_Event event;
    uint32_t frame_start;
    uint32_t frame_stop;

    /* Test Data */
    jt_unit units[] = { { 0, 10, 10, 10, 10 },
                        { 0, 10, 20, 10, 20 },
                        { 0, 20, 10, 20, 10 },
                        { 0, 20, 20, 20, 20 },
                        { 0, 20, 30, 20, 30 } };

    /* Load textures */
    SDL_Texture *background_texture     = loadTexture (renderer, "./Media/Greenstuff.png");
    SDL_Texture *selected_unit          = loadTexture (renderer, "./Media/Selected32.png");
    SDL_Texture *unselected_unit        = loadTexture (renderer, "./Media/Unselected32.png");

    /* Set cursor */
    SDL_Cursor* cursor;
    cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    SDL_SetCursor(cursor);

    for (;;)
    {
        frame_start = SDL_GetTicks ();

        /* -- Process input -- */
        while (SDL_PollEvent (&event))
        {
            if (event.type == SDL_QUIT)
            {
                return 0;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        return 0;
                        break;
                    default:
                        break;
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        jt_select_units (event.button.x, event.button.y, units);
                        break;
                    case SDL_BUTTON_RIGHT:
                        jt_move_units (event.button.x, event.button.y, units);
                        break;
                    default:
                        break;
                }
            }
        }

        /* -- Game Logic -- */
        /* Each unit moves towards its goal */
        for (int i = 0; i < 5; i++) /* Currently, five test-units */
        {
            /* For now, we will assume we always get 60 FPS */
            double distance_per_frame = 4.0 / 60.0; /* Four spaces per second */

            double unit_x = units[i].x_position;
            double unit_y = units[i].y_position;
            double goal_x = units[i].x_goal;
            double goal_y = units[i].y_goal;

            double distance = sqrt ((unit_x - goal_x) * (unit_x - goal_x) +
                                    (unit_y - goal_y) * (unit_y - goal_y));

            /* If we are already at our goal, no need to do anything. */
            if (unit_x == goal_x && unit_y == goal_y)
            {
            }
            /* Just take the goal if we are less than a frames distance from it. */
            else if (distance < distance_per_frame)
            {
                units[i].x_position = goal_x;
                units[i].y_position = goal_y;
            }
            /* Otherwise, move at our constant velocity */
            else
            {
                double x_direction = (goal_x - unit_x) / distance;
                double y_direction = (goal_y - unit_y) / distance;
                units[i].x_position += x_direction * distance_per_frame;
                units[i].y_position += y_direction * distance_per_frame;
            }
        }

        /* -- Rendering -- */

        /* Background */
        SDL_SetRenderDrawColor (renderer, 0x00, 0x80, 0x00, 0xFF);
        SDL_RenderClear (renderer);
        SDL_RenderCopy (renderer, background_texture, NULL, NULL);

        /* Units */
        for (int i = 0; i < 5; i++) /* Currently, five test-units */
        {
            SDL_Rect dest_rect = { 32.0 * units[i].x_position,
                                   32.0 * units[i].y_position,
                                   32, 32 };
            SDL_RenderCopy (renderer,
                            units[i].selected ? selected_unit : unselected_unit,
                            NULL,
                            &dest_rect);
        }

        SDL_RenderPresent (renderer);

        /* Frame limiting */
        frame_stop = SDL_GetTicks ();
        if ((frame_stop - frame_start) < (1000 / FRAME_LIMIT))
        {
            SDL_Delay (1000 / FRAME_LIMIT - (frame_stop - frame_start));
        }
    }

    SDL_DestroyTexture (background_texture);

    return 0;
}

