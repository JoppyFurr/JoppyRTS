#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Libraries/AStar/AStar.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "jt_texture.h"
#include "jt_path.h"
#include "jt_sidebar.h"

#define FRAME_LIMIT 60

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

void jt_select_units (double x, double y, jt_unit *units)
{
    for (int i = 0; i < 5; i++) /* Currently, five test-units */
    {
        if ( x > (units[i].x_position - 0.5) && x < (units[i].x_position + 0.5) &&
             y > (units[i].y_position - 0.5) && y < (units[i].y_position + 0.5))
        {
            units[i].selected = 1;
        }
        else
        {
            units[i].selected = 0;
        }

    }
}

void jt_move_units (double x, double y, jt_unit *units)
{
    for (int i = 0; i < 5; i++) /* Currently, five test-units */
    {
        if ( units[i].selected )
        {
            units[i].x_goal = x;
            units[i].y_goal = y;

            if (units[i].path)
            {
                jt_path_free (units[i].path);
            }

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

            /* Does a path exist? */
            if (ASPathGetCount (path) > 0)
            {
                /* Simplify the path */
                units[i].path = jt_path_simplify (path);
                units[i].path_progress = 0;
            }

            ASPathDestroy (path);
        }
    }
}

/* TODO: Globals are bad, mkay. */
int world [100][100];

#define LEFT_BUTTON_DOWN    0x01
#define MIDDLE_BUTTON_DOWN  0x02
#define RIGHT_BUTTON_DOWN   0x04
#define MOUSE_HAS_MOVED     0x08

int jt_run_game (SDL_Renderer *renderer)
{
    SDL_Event event;
    uint32_t frame_start;
    uint32_t frame_stop;

    int screen_width;
    int screen_height;

    /* Where in world-space are we looking? */
    double camera_x = 50.0;
    double camera_y = 50.0;

    /* Position at which the mouse button was held down */
    int mouse_state = 0;
    int mouse_down_x = 0;
    int mouse_down_y = 0;
    int mouse_position_x = 0;
    int mouse_position_y = 0;

    /* Cell in world-space that the mouse hovers over */
    int mouse_cell_x = 0;
    int mouse_cell_y = 0;
    double mouse_world_x = 0;
    double mouse_world_y = 0;

    /* Are we placing a wall at the moment? */
    int placing_wall = 0;

    /* Camera calculations */
    double camera_width;
    double camera_height;
    double camera_top;
    double camera_bottom;
    double camera_left;
    double camera_right;

    /* Test Data */
    jt_unit units[] = { { 0,  5.5,  5.5,  5.5,  5.5, NULL, 0 },
                        { 0,  5.5, 10.5,  5.5, 10.5, NULL, 0 },
                        { 0, 10.5,  5.5, 10.5,  5.5, NULL, 0 },
                        { 0, 10.5, 10.5, 10.5, 10.5, NULL, 0 },
                        { 0, 10.5, 15.5, 10.5, 15.5, NULL, 0 } };

    /* Walls:           */
    /*   0 -> no wall   */
    /*   1 -> wall      */
    memset (world, 0, sizeof (int) * 100 * 100);

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
    jt_rts_textures_load (renderer);
    jt_rts_textures *rts_textures = jt_rts_textures_get ();

    /* Set cursor */
    SDL_Cursor* cursor;
    cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    SDL_SetCursor(cursor);

    for (;;)
    {
        frame_start = SDL_GetTicks ();
        SDL_GetRendererOutputSize(renderer, &screen_width, &screen_height);

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
                mouse_down_x = event.button.x;
                mouse_down_y = event.button.y;
                mouse_state &= ~MOUSE_HAS_MOVED;

                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        mouse_state |= LEFT_BUTTON_DOWN;
                        break;

                    case SDL_BUTTON_RIGHT:
                        mouse_state |= RIGHT_BUTTON_DOWN;

                    default:
                        break;
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP)
            {

                /* Restore the default arrow */
                {
                    SDL_Cursor* cursor;
                    cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
                    SDL_SetCursor(cursor);
                }

                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        mouse_state &= ~LEFT_BUTTON_DOWN;

                        /* if (mouse_state & MOUSE_HAS_MOVED)
                        {
                            TODO: Selection-box code
                        }
                        else */
                        {
                            if (event.button.x >= (screen_width - 256 + 6) &&
                                event.button.x <  (screen_width - 256 + 6 + 122) &&
                                event.button.y >= (325) &&
                                event.button.y <  (325 + 64))
                            {
                                placing_wall = !placing_wall;
                            }
                            else if (placing_wall)
                            {
                                if (world[mouse_cell_y][mouse_cell_x] == 0)
                                {
                                    world[mouse_cell_y][mouse_cell_x] = 1;
                                    placing_wall = 0;
                                }
                            }
                            else
                            {
                                /* TODO: Perhaps move all actions based on mouse coordinates
                                 *       to after where we have just updated the mouse in
                                 *       world-space */
                                jt_select_units (mouse_world_x, mouse_world_y, units);
                            }
                        }
                        break;
                    case SDL_BUTTON_RIGHT:
                        mouse_state &= ~RIGHT_BUTTON_DOWN;

                        if (mouse_state & MOUSE_HAS_MOVED)
                        {
                            /* Move around the map */
                        }
                        else
                        {
                            if (placing_wall)
                            {
                                placing_wall = 0;
                            }
                            else
                            {
                                /* TODO: Perhaps move all actions based on mouse coordinates
                                 *       to after where we have just updated the mouse in
                                 *       world-space */
                                jt_move_units (mouse_world_x, mouse_world_y, units);
                            }
                        }
                        break;
                    default:
                        break;
                }
                mouse_state &= ~MOUSE_HAS_MOVED;
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                if (mouse_state & (LEFT_BUTTON_DOWN | RIGHT_BUTTON_DOWN))
                {
                    /* TODO: Implement a dead-zone */
                    mouse_state |= MOUSE_HAS_MOVED;
                    if (mouse_state & RIGHT_BUTTON_DOWN)
                    {
                        /* TODO: We probably need to free these things */
                        SDL_Cursor* cursor;
                        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
                        SDL_SetCursor(cursor);
                    }
                }
                mouse_position_x = event.motion.x;
                mouse_position_y = event.motion.y;

            }
        }

        /* -- Game Logic -- */

        /* Update camera position */
        if ((mouse_state & RIGHT_BUTTON_DOWN) &&
            (mouse_state & MOUSE_HAS_MOVED))
        {
            /* TODO: Fix 60 FPS assumption */
            camera_x += 4.0 * (mouse_position_x - mouse_down_x) / 32.0 / 60.0;
            camera_y += 4.0 * (mouse_position_y - mouse_down_y) / 32.0 / 60.0;
        }

        /* Camera calculations */
        camera_width = (screen_width - 256 ) / 32.0; /* Subtract sidebar width */
        camera_height  = screen_height / 32.0;
        camera_top = camera_y - camera_height / 2;
        camera_bottom = camera_y + camera_height / 2;
        camera_left = camera_x - camera_width / 2;
        camera_right = camera_x + camera_width / 2;

        /* Mouse position in world-space */
        mouse_cell_x = camera_left + (mouse_position_x / 32.0);
        mouse_cell_y = camera_top + (mouse_position_y / 32.0);
        mouse_world_x = camera_left + (mouse_position_x / 32.0);
        mouse_world_y = camera_top + (mouse_position_y / 32.0);

        /* Each unit moves towards its goal */
        for (int i = 0; i < 5; i++) /* Currently, five test-units */
        {
            /* For now, we will assume we always get 60 FPS */
            double distance_per_frame = 5.0 / 60.0; /* Five spaces per second */
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

        /* Silly and trivial for now */
        for (int y = 0 ; y < 100; y += 4)
        {
            for (int x = 0; x < 100; x += 4)
            {
                SDL_Rect dest_rect = { (x - camera_left) * 32,
                                       (y - camera_top) * 32,
                                       128, 128 };
                SDL_RenderCopy (renderer,
                                rts_textures->grass,
                                NULL,
                                &dest_rect);
            }
        }

        /* Structures */
        for (int y = 0; y < 100; y++)
        {
            for (int x = 0; x < 100; x++)
            {
                if (world[y][x] == 1)
                {
                    /* Select which all sprite to use based on the
                     * surrounding walls */
                    int sprite_index = 0;
                    if (y != 0 && world[y-1][x] == 1)
                        sprite_index |= 1;
                    if (x != 60 && world[y][x+1] == 1)
                        sprite_index |= 2;
                    if (y != 32 && world[y+1][x] == 1)
                        sprite_index |= 4;
                    if (x != 0 && world[y][x-1] == 1)
                        sprite_index |= 8;

                    SDL_Rect src_rect = {32 * sprite_index, 0, 32, 32};
                    SDL_Rect dest_rect = { 32.0 * (x - camera_left),
                                           32.0 * (y - camera_top),
                                           32, 32 };
                    SDL_RenderCopy (renderer,
                                    rts_textures->wall,
                                    &src_rect,
                                    &dest_rect);
                }
            }
        }

        /* Placement selector */
        if (placing_wall)
        {
            SDL_Rect src_rect;
            SDL_Rect dest_rect = { (mouse_cell_x - camera_left) * 32,
                                   (mouse_cell_y - camera_top) * 32,
                                   32, 32 };

            if (world[mouse_cell_y][mouse_cell_x])
            {
                src_rect = (SDL_Rect) { 32, 0, 32, 32 };
            }
            else
            {
                src_rect = (SDL_Rect) { 0, 0, 32, 32 };
            }

            SDL_RenderCopy (renderer,
                            rts_textures->placement,
                            &src_rect,
                            &dest_rect);
        }


        /* Units */
        for (int i = 0; i < 5; i++) /* Currently, five test-units */
        {
            SDL_Rect dest_rect = { 32.0 * (units[i].x_position - 0.5 - camera_left),
                                   32.0 * (units[i].y_position - 0.5 - camera_top),
                                   16, 24 };
            SDL_RenderCopy (renderer,
                            rts_textures->unit,
                            NULL,
                            &dest_rect);
        }

        jt_sidebar_render (renderer);

        SDL_RenderPresent (renderer);

        /* Frame limiting */
        frame_stop = SDL_GetTicks ();
        if ((frame_stop - frame_start) < (1000 / FRAME_LIMIT))
        {
            SDL_Delay (1000 / FRAME_LIMIT - (frame_stop - frame_start));
        }
    }

    jt_rts_textures_free ();

    return 0;
}

