#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Libraries/AStar/AStar.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "jt_helpers.h"
#include "jt_path.h"

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
    jt_path *path;
    int path_progress;
} jt_unit;

void jt_select_units (int x, int y, jt_unit *units)
{
    for (int i = 0; i < 5; i++) /* Currently, five test-units */
    {
        if ( x > ((units[i].x_position - 0.5) * 32.0) && x < ((units[i].x_position - 0.5) * 32.0 + 32.0) &&
             y > ((units[i].y_position - 0.5) * 32.0) && y < ((units[i].y_position - 0.5) * 32.0 + 32.0))
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

            /* TODO - If we already have a path, destroy that before
             *        making a new one. */

            /*  -- TEMP: Use A* -- */
            /* Set up callbacks */
            ASPathNodeSource jt_path_node_source = { sizeof (jt_path_node),
                                                     &jt_path_node_neighbours,
                                                     &jt_path_node_heuristic,
                                                     NULL, NULL };

            /* Run the algorithm */
            jt_path_node start = { units[i].x_position, units[i].y_position };
            jt_path_node goal =  { units[i].x_goal,     units[i].y_goal     };
            ASPath path = ASPathCreate (&jt_path_node_source, NULL, &start, &goal);

            /* Simplify the path */
            units[i].path = jt_path_simplify (path);
            units[i].path_progress = 0;

            ASPathDestroy (path);
        }
    }
}

/* TODO: Globals are bad, mkay. */
int world [33][60];

int jt_run_game (SDL_Renderer *renderer)
{
    SDL_Event event;
    uint32_t frame_start;
    uint32_t frame_stop;

    /* Note: Pre-scrolling size is 60x33 */

    /* Test Data */
    jt_unit units[] = { { 0,  5.5,  5.5,  5.5,  5.5, NULL, 0 },
                        { 0,  5.5, 10.5,  5.5, 10.5, NULL, 0 },
                        { 0, 10.5,  5.5, 10.5,  5.5, NULL, 0 },
                        { 0, 10.5, 10.5, 10.5, 10.5, NULL, 0 },
                        { 0, 10.5, 15.5, 10.5, 15.5, NULL, 0 } };

    /* Walls:           */
    /*   0 -> no wall   */
    /*   1 -> wall      */
    memset (world, 0, sizeof (int) * 33 * 60);

    /* horseshoe */
    world [10][29] = 1; world [10][28] = 1; world [10][27] = 1;
    world [10][30] = 1; world [11][30] = 1; world [12][30] = 1;
    world [13][30] = 1; world [14][30] = 1; world [14][29] = 1;
    world [14][28] = 1; world [14][27] = 1;

    /* One block gap */
    world [10][45] = 1; world [11][45] = 1; world [12][45] = 1;
    world [13][45] = 1; world [14][45] = 1; world [16][45] = 1;
    world [17][45] = 1; world [18][45] = 1; world [19][45] = 1;
    world [20][45] = 1;


    /* Load textures */
    SDL_Texture *background_texture     = loadTexture (renderer, "./Media/Greenstuff.png");
    SDL_Texture *selected_unit          = loadTexture (renderer, "./Media/Selected32.png");
    SDL_Texture *unselected_unit        = loadTexture (renderer, "./Media/Unselected32.png");
    SDL_Texture *wall_texture           = loadTexture (renderer, "./Media/Wall32.png");

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

            if (units[i].path)
            {
                if (units[i].path_progress == units[i].path->count)
                {
                    jt_path_free (units[i].path);
                    units[i].path = NULL;
                }
                else
                {
                    jt_path_node *path_node = &units[i].path->nodes[units[i].path_progress];
                    double goal_x = path_node->x + 0.5;
                    double goal_y = path_node->y + 0.5;
                    double distance = sqrt ((unit_x - goal_x) * (unit_x - goal_x) +
                                            (unit_y - goal_y) * (unit_y - goal_y));

                    /* Just take the goal if we are less than a frames distance from it. */
                    if (distance < distance_per_frame)
                    {
                        units[i].x_position = goal_x;
                        units[i].y_position = goal_y;
                        units[i].path_progress++;
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
            }
        }

        /* -- Rendering -- */

        /* Background */
        SDL_SetRenderDrawColor (renderer, 0x00, 0x80, 0x00, 0xFF);
        SDL_RenderClear (renderer);
        SDL_RenderCopy (renderer, background_texture, NULL, NULL);

        /* Structures */
        for (int y = 0; y < 33; y++)
        {
            for (int x = 0; x < 60; x++)
            {
                if (world[y][x] == 1)
                {
                    SDL_Rect dest_rect = { 32.0 * x,
                                           32.0 * y,
                                           32, 32 };
                    SDL_RenderCopy (renderer,
                                    wall_texture,
                                    NULL,
                                    &dest_rect);
                }
            }
        }


        /* Units */
        for (int i = 0; i < 5; i++) /* Currently, five test-units */
        {
            SDL_Rect dest_rect = { 32.0 * (units[i].x_position - 0.5),
                                   32.0 * (units[i].y_position - 0.5),
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

