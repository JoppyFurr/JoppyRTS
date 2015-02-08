#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Libraries/AStar/AStar.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "jt_texture.h"
#include "jt_path.h"
#include "jt_sidebar.h"
#include "jt_mouse.h"
#include "jt_world.h"

#define FRAME_LIMIT 60

/* TODO: Use this */
int is_in_sidebar (int screen_width, int x)
{
    return x >= (screen_width - 256);
}

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

/* TODO: A better unit data structure */
jt_unit units[1000];
int global_unit_count = 0;

typedef struct jt_cell_struct
{
    int x;
    int y;
} jt_cell;

void jt_select_units (double x, double y, jt_unit *units)
{
    for (int i = 0; i < global_unit_count; i++) /* Currently, five test-units */
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

void jt_select_units_box (double x1, double y1,
                          double x2, double y2, jt_unit *units)
{
    for (int i = 0; i < global_unit_count; i++) /* Currently, five test-units */
    {
        if ( units[i].x_position > fmin (x1, x2) && units[i].x_position < fmax (x1, x2) &&
             units[i].y_position > fmin (y1, y2) && units[i].y_position < fmax (y1, y2))
        {
            units[i].selected = 1;
        }
        else
        {
            units[i].selected = 0;
        }

    }
}

void jt_move_unit (double x, double y, jt_unit *unit)
{
    unit->x_goal = x;
    unit->y_goal = y;

    if (unit->path)
    {
        jt_path_free (unit->path);
    }

    /*  -- TEMP: Use A* -- */
    /* Set up callbacks */
    ASPathNodeSource jt_path_node_source = { sizeof (jt_path_node),
                                             &jt_path_node_neighbours,
                                             &jt_path_node_heuristic,
                                             NULL, NULL };

    /* Run the algorithm */
    jt_path_node start = { unit->x_position, unit->y_position };
    jt_path_node goal =  { unit->x_goal,     unit->y_goal     };
    ASPath path = ASPathCreate (&jt_path_node_source, NULL, &start, &goal);

    /* Does a path exist? */
    if (ASPathGetCount (path) > 0)
    {
        /* Simplify the path */
        unit->path = jt_path_simplify (path);
        unit->path_progress = 0;
    }

    ASPathDestroy (path);
}

void jt_move_units (double x, double y, jt_unit *units)
{
    for (int i = 0; i < global_unit_count; i++) /* Currently, five test-units */
    {
        if ( units[i].selected )
        {
            jt_move_unit (x, y, &units[i]);
        }
    }
}

/* TODO: Refactor to remove globals */
jt_world_cell world [100][100];
SDL_Renderer    *global_renderer;
double          *global_camera_left;
double          *global_camera_top;
int             *global_screen_width;

/* Find the closest non-populated cell in the world */
jt_cell jt_closest (int x, int y)
{
    jt_cell guess;
    for (int distance = 1; distance < 10 ; distance++)
    {
        for (guess.x = x - distance; guess.x <= x + distance; guess.x++)
        {
            for (guess.y = y - distance; guess.y <= y + distance; guess.y++)
            {
                if (guess.x < 0 || guess.x >= 100 ||
                    guess.y < 0 || guess.x >= 100)
                {
                    continue;
                }
                if (!world[guess.y][guess.x].contains_unit &&
                    !world[guess.y][guess.x].contains_building)
                {
                    return guess;
                }
            }
        }
    }
}

void jt_render_placement_selector (SDL_Renderer *renderer, double mouse_x, double mouse_y)
{
    jt_rts_textures *rts_textures = jt_rts_textures_get ();
    SDL_Rect src_rect;
    SDL_Rect dest_rect = { ((int) mouse_x - *global_camera_left) * 32,
                           ((int) mouse_y - *global_camera_top) * 32,
                           32, 32 };

    if (world[(int) mouse_y][(int) mouse_x].contains_unit ||
        world[(int) mouse_y][(int) mouse_x].contains_building)
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

/* TODO: A better data structure for buildings */
jt_world_cell *primary_tent = NULL;
int primary_tent_x = 0;
int primary_tent_y = 0;

void jt_new_unit ()
{
    if (!primary_tent)
    {
        return;
    }

    units[global_unit_count++] = (jt_unit) { 0,
                                             primary_tent_x + 0.5,
                                             primary_tent_y + 2.5,
                                             primary_tent_x + 0.5,
                                             primary_tent_y + 2.5,
                                             NULL,
                                             0 };
}

#define JT_PLACING_NONE 0
#define JT_PLACING_WALL 1
#define JT_PLACING_TENT 2

#define JT_ACTION_NONE 0
#define JT_ACTION_BUILD_UNIT 1

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

    /* Are we placing a wall at the moment? */
    int placing = JT_PLACING_NONE;
    int action = JT_ACTION_NONE;

    /* Camera calculations */
    double camera_width;
    double camera_height;
    double camera_top;
    double camera_bottom;
    double camera_left;
    double camera_right;

    /* TODO: Remove globals */
    global_renderer = renderer;
    global_camera_left = &camera_left;
    global_camera_top = &camera_top;
    global_screen_width = &screen_width;

    /* Clear the map */
    memset (world, 0, sizeof (jt_world_cell) * 100 * 100);

    /* Load textures */
    jt_rts_textures_load (renderer);
    jt_rts_textures *rts_textures = jt_rts_textures_get ();

    /* Set cursor */
    SDL_Cursor* cursor;
    cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    SDL_SetCursor(cursor);

    jt_mouse_state *mouse = jt_mouse_state_get();

    for (;;)
    {
        frame_start = SDL_GetTicks ();

        /* Retrieve input from SDL */
        SDL_GetRendererOutputSize(renderer, &screen_width, &screen_height);
        while (SDL_PollEvent (&event))
        {
            if (event.type == SDL_QUIT)
            {
                return 0;
            }

            if (event.type == SDL_KEYDOWN)
            {
                /* TODO: Why is this not working? */
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        return 0;
                        break;
                    default:
                        break;
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN ||
                event.type == SDL_MOUSEBUTTONUP   ||
                event.type == SDL_MOUSEMOTION)
            {
                jt_mouse_input (&event);
            }


        }

        switch (mouse->state)
        {
            case JT_MOUSE_DEFAULT:
                break;

            case JT_MOUSE_CLICK_LEFT:
                if (placing == JT_PLACING_WALL)
                {
                    if ((int) mouse->x >= 0 && (int) mouse->x < 100 &&
                        (int) mouse->y >= 0 && (int) mouse->y < 100 &&
                        !world[(int) mouse->y][(int) mouse->x].contains_unit &&
                        !world[(int) mouse->y][(int) mouse->x].contains_building)
                    {
                        world[(int) mouse->y][(int) mouse->x].contains_building = 1;
                        world[(int) mouse->y][(int) mouse->x].render_as = JT_RENDER_WALL;
                        placing = JT_PLACING_NONE;
                    }
                }
                else if (placing == JT_PLACING_TENT)
                {
                    /* TODO: This is a mess! */
                    if ((int) mouse->x >= 0 && (int) mouse->x < 100 &&
                        (int) mouse->y >= 0 && (int) mouse->y < 100 &&
                        !world[(int) mouse->y    ][(int) mouse->x    ].contains_unit &&
                        !world[(int) mouse->y    ][(int) mouse->x    ].contains_building &&
                        !world[(int) mouse->y + 1][(int) mouse->x    ].contains_unit &&
                        !world[(int) mouse->y + 1][(int) mouse->x    ].contains_building &&
                        !world[(int) mouse->y    ][(int) mouse->x + 1].contains_unit &&
                        !world[(int) mouse->y    ][(int) mouse->x + 1].contains_building &&
                        !world[(int) mouse->y + 1][(int) mouse->x + 1].contains_unit &&
                        !world[(int) mouse->y + 1][(int) mouse->x + 1].contains_building)
                    {
                        world[(int) mouse->y    ][(int) mouse->x    ].contains_building = 1;
                        world[(int) mouse->y + 1][(int) mouse->x    ].contains_building = 1;
                        world[(int) mouse->y    ][(int) mouse->x + 1].contains_building = 1;
                        world[(int) mouse->y + 1][(int) mouse->x + 1].contains_building = 1;
                        world[(int) mouse->y    ][(int) mouse->x].render_as = JT_RENDER_TENT;
                        placing = JT_PLACING_NONE;
                        primary_tent = &world[(int) mouse->y][(int) mouse->x];
                        primary_tent_x = (int) mouse->x;
                        primary_tent_y = (int) mouse->y;
                    }
                }
                else
                {
                    jt_select_units (mouse->x, mouse->y, units);
                }

                mouse->state = JT_MOUSE_DEFAULT;
                break;

            case JT_MOUSE_CLICK_RIGHT:
                if (placing != JT_PLACING_NONE)
                {
                    placing = JT_PLACING_NONE;
                }
                else
                {
                    jt_move_units (mouse->x, mouse->y, units);
                }

                mouse->state = JT_MOUSE_DEFAULT;
                break;

            case JT_MOUSE_RELEASE_LEFT:
                if (placing == JT_PLACING_NONE)
                {
                    jt_select_units_box (mouse->down_x, mouse->down_y,
                                         mouse->x, mouse->y, units);
                }

                mouse->state = JT_MOUSE_DEFAULT;
                break;

            case JT_MOUSE_RELEASE_RIGHT:

                mouse->state = JT_MOUSE_DEFAULT;
                break;

            case JT_MOUSE_CLICK_SIDEBAR_LEFT:
                /* TODO: Click event should return a button ID so that
                 *       we don't need to do any pixelmath here */
                if (mouse->sidebar_x >= 6 &&
                    mouse->sidebar_x <  (6 + 122) &&
                    mouse->sidebar_y >= 325 &&
                    mouse->sidebar_y <  (325 + 64))
                {
                    placing = JT_PLACING_WALL;
                }
                else if (mouse->sidebar_x >= 6 &&
                    mouse->sidebar_x <  (6 + 122) &&
                    mouse->sidebar_y >= 325 + 66 &&
                    mouse->sidebar_y <  (325 + 66 + 64))
                {
                    placing = JT_PLACING_TENT;
                }
                else if (mouse->sidebar_x >= 6 + 124 &&
                    mouse->sidebar_x <  (6 + 122 + 124) &&
                    mouse->sidebar_y >= 325 &&
                    mouse->sidebar_y <  (325 + 64))
                {
                    action = JT_ACTION_BUILD_UNIT;
                }

                mouse->state = JT_MOUSE_DEFAULT;
                break;

            case JT_MOUSE_CLICK_SIDEBAR_RIGHT:
                if (placing != JT_PLACING_NONE)
                {
                    placing = JT_PLACING_NONE;
                }

                mouse->state = JT_MOUSE_DEFAULT;
                break;

            default:
                break;
        }

        /* Update camera position */
        if (mouse->state == JT_MOUSE_DRAG_RIGHT)
        {
            /* TODO: Fix 60 FPS assumption */
            camera_x += 4.0 * mouse->distance_x / 60.0;
            camera_y += 4.0 * mouse->distance_y / 60.0;
        }

        /* Camera calculations */
        camera_width = (screen_width - 256 ) / 32.0; /* Subtract sidebar width */
        camera_height  = screen_height / 32.0;
        camera_top = camera_y - camera_height / 2;
        camera_bottom = camera_y + camera_height / 2;
        camera_left = camera_x - camera_width / 2;
        camera_right = camera_x + camera_width / 2;

        /* World state update */

        if (action == JT_ACTION_BUILD_UNIT)
        {
            jt_new_unit();
            action = JT_ACTION_NONE;
        }

        /* Clear previous unit positions */
        for (int y = 0; y < 100; y++)
        {
            for (int x = 0; x < 100; x++)
            {
                if (world[y][x].contains_unit)
                {
                    world[y][x].contains_unit = 0;
                }
            }
        }
        for (int i = 0; i < global_unit_count; i++)
        {
            /* If there is already a unit here, we should probably move */
            if (world[(int) units[i].y_position][(int) units[i].x_position].contains_unit &&
                units[i].path == NULL)
            {
                /* Try move to the nearest empty space */
                jt_cell new = jt_closest ((int) units[i].x_position, (int) units[i].y_position);
                jt_move_unit (new.x + 0.5, new.y + 0.5, &units[i]);
            }
            else
            {
                world[(int) units[i].y_position][(int) units[i].x_position].contains_unit = 1;
            }
        }

        /* Each unit moves towards its goal */
        for (int i = 0; i < global_unit_count; i++) /* Currently, five test-units */
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
                    /* Otherwise, continue to move towards the goal */
                    else
                    {
                        /* TODO: Doing this every frame is overkill… */
                        /* We need to check that the path is still clear */
                        jt_path_node current_position = { (int) unit_x, (int) unit_y };
                        if (!jt_path_is_clear (&current_position, path_node))
                        {
                            /* We must recalculate */
                            jt_move_unit (units[i].x_goal, units[i].y_goal, &units[i]);

                        }
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
                if (world[y][x].render_as == JT_RENDER_WALL)
                {
                    /* Select which all sprite to use based on the
                     * surrounding walls */
                    int sprite_index = 0;
                    if (y != 0 && world[y-1][x].render_as == JT_RENDER_WALL)
                        sprite_index |= 1;
                    if (x != 99 && world[y][x+1].render_as == JT_RENDER_WALL)
                        sprite_index |= 2;
                    if (y != 99 && world[y+1][x].render_as == JT_RENDER_WALL)
                        sprite_index |= 4;
                    if (x != 0 && world[y][x-1].render_as == JT_RENDER_WALL)
                        sprite_index |= 8;

                    SDL_Rect src_rect = { 32 * sprite_index, 0, 32, 32};
                    SDL_Rect dest_rect = { 32.0 * (x - camera_left),
                                           32.0 * (y - camera_top),
                                           32, 32 };
                    SDL_RenderCopy (renderer,
                                    rts_textures->wall,
                                    &src_rect,
                                    &dest_rect);
                }

                if (world[y][x].render_as == JT_RENDER_TENT)
                {
                    SDL_Rect src_rect = { 0, 0, 64, 64};
                    SDL_Rect dest_rect = { 32.0 * (x - camera_left),
                                           32.0 * (y - camera_top),
                                           64, 64 };
                    SDL_RenderCopy (renderer,
                                    rts_textures->tent,
                                    &src_rect,
                                    &dest_rect);
                }
            }
        }

        /* Placement selector */
        if ((int) mouse->x >= 0 && (int) mouse->x < 100 &&
            (int) mouse->y >= 0 && (int) mouse->y < 100)
        {
            if (placing == JT_PLACING_WALL)
            {
                jt_render_placement_selector (renderer, mouse->x, mouse->y);
            }
            if (placing == JT_PLACING_TENT)
            {
                jt_render_placement_selector (renderer, mouse->x,       mouse->y);
                jt_render_placement_selector (renderer, mouse->x + 1.0, mouse->y);
                jt_render_placement_selector (renderer, mouse->x,       mouse->y + 1.0);
                jt_render_placement_selector (renderer, mouse->x + 1.0, mouse->y + 1.0);
            }
        }


        /* Units */
        for (int i = 0; i < global_unit_count; i++) /* Currently, five test-units */
        {
            double x_position = units[i].x_position;
            double y_position = units[i].y_position;

            /* TODO: Currently we just centre the texture. But the tallness
             *       of a unit should allow rendering into the above cell */
            SDL_Rect dest_rect = { 32.0 * (x_position - 0.25 - camera_left),
                                   32.0 * (y_position - 0.375 - camera_top),
                                   16, 24 };
            SDL_RenderCopy (renderer,
                            rts_textures->unit,
                            NULL,
                            &dest_rect);
            if(units[i].selected)
            {
                SDL_Rect src_rect;
                SDL_Rect dest_rect;

                /* Top left */
                src_rect  = (SDL_Rect) { 0, 0, 4, 4 };
                dest_rect = (SDL_Rect) { 32.0 * (x_position - 0.25 - camera_left) - 2,
                                        32.0 * (y_position - 0.375 - camera_top) - 2,
                                        4, 4 };
                SDL_RenderCopy (renderer, rts_textures->selector, &src_rect, &dest_rect);

                /* Top right */
                src_rect  = (SDL_Rect) { 4, 0, 4, 4 };
                dest_rect = (SDL_Rect) { 32.0 * (x_position + 0.25 - camera_left) - 2,
                                        32.0 * (y_position - 0.375 - camera_top) - 2,
                                        4, 4 };
                SDL_RenderCopy (renderer, rts_textures->selector, &src_rect, &dest_rect);

                /* Bottom left */
                src_rect  = (SDL_Rect) { 0, 4, 4, 4 };
                dest_rect = (SDL_Rect) { 32.0 * (x_position - 0.25 - camera_left) - 2,
                                        32.0 * (y_position + 0.375 - camera_top) - 2,
                                        4, 4 };
                SDL_RenderCopy (renderer, rts_textures->selector, &src_rect, &dest_rect);

                /* Bottom right */
                src_rect  = (SDL_Rect) { 4, 4, 4, 4 };
                dest_rect = (SDL_Rect) { 32.0 * (x_position + 0.25 - camera_left) - 2,
                                        32.0 * (y_position + 0.375 - camera_top) - 2,
                                        4, 4 };
                SDL_RenderCopy (renderer, rts_textures->selector, &src_rect, &dest_rect);
            }
        }

        /* Selection box */
        if (placing == JT_PLACING_NONE && mouse->state == JT_MOUSE_DRAG_LEFT)
        {
            /* TODO: Functions to map between world-space and screen-space */
            SDL_Rect selection_rectangle = { 32.0 * (fmin (mouse->x, mouse->down_x) - camera_left),
                                             32.0 * (fmin (mouse->y, mouse->down_y) - camera_top),
                                             32.0 * fabs (mouse->x - mouse->down_x),
                                             32.0 * fabs (mouse->y - mouse->down_y) };
            SDL_SetRenderDrawColor (renderer, 0xF0, 0xF0, 0xF0, 0xFF);
            SDL_RenderDrawRect (renderer, &selection_rectangle);
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

