#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "SDL2/SDL.h"

#include "jt_mouse.h"

/* TODO: Remove globals */
extern SDL_Renderer *global_renderer;
extern double       *global_camera_left;
extern double       *global_camera_top;
extern int          *global_screen_width;

static jt_mouse_state mouse_state;

jt_mouse_state *jt_mouse_state_get()
{
    return &mouse_state;
}

/* Desired mouse behaviour:
 *
 * Left click -> Select a unit, build a building, place a building.
 * Right click -> Order a unit to move, cancel a building, cancel a placement.
 *
 * Left drag -> Select a group of units. 
 * Right drag -> Scroll around the map.
 */

/* Note:
 *
 *   This function will set the mouse state.
 *   
 *   When a click or release state is processed, the processor should restore
 *   the default state.
 *
 *   Click or release states should not be overriden by drag states.
 *
 *   This assumes that only a single click/release will happen per frame.
 */
void jt_mouse_input (SDL_Event *event)
{
    static int pixel_x;
    static int pixel_y;
    static int down_pixel_x;
    static int down_pixel_y;
    static int left_button_down = 0;
    static int right_button_down = 0;
    static int mouse_has_moved = 0;

    if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        down_pixel_x = event->button.x;
        down_pixel_y = event->button.y;
        mouse_state.down_x = *global_camera_left + down_pixel_x / 32.0;
        mouse_state.down_y = *global_camera_top + down_pixel_y / 32.0;
        mouse_has_moved = 0;

        switch (event->button.button)
        {
            case SDL_BUTTON_LEFT:
                left_button_down = 1;
                break;

            case SDL_BUTTON_RIGHT:
                right_button_down = 1;
                break;

            default:
                break;
        }
    }

    if (event->type == SDL_MOUSEMOTION)
    {
        pixel_x = event->motion.x;
        pixel_y = event->motion.y;

        if ((left_button_down || right_button_down) && !mouse_has_moved)
        {
            if ((pixel_x - down_pixel_x) * (pixel_x - down_pixel_x) +
                (pixel_y - down_pixel_y) * (pixel_y - down_pixel_y) > 5.0)
            {
                mouse_has_moved = 1;
            }
        }

        if (mouse_has_moved && left_button_down &&
            mouse_state.state == JT_MOUSE_DEFAULT)
        {
            mouse_state.state = JT_MOUSE_DRAG_LEFT;
        }

        if (mouse_has_moved && right_button_down &&
            mouse_state.state == JT_MOUSE_DEFAULT)
        {
            mouse_state.state = JT_MOUSE_DRAG_RIGHT;

            /* TODO: This must go into the rendering code */
            {
                SDL_Cursor* cursor;
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
                SDL_SetCursor(cursor);
            }
        }
    }

    if (event->type == SDL_MOUSEBUTTONUP)
    {

        /* TODO: This should go into the rendering code */
        {
            SDL_Cursor* cursor;
            cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
            SDL_SetCursor(cursor);
        }
        
        switch (event->button.button)
        {
            /* TODO: Refactor this to better deal with sidbar buttons
             *       and world interaction */
            case SDL_BUTTON_LEFT:
                left_button_down = 0;
                if (mouse_has_moved)
                {
                    mouse_state.state = JT_MOUSE_RELEASE_LEFT;
                }
                else if (pixel_x > (*global_screen_width - 256))
                {
                    /* The click was in the sidebar */
                    mouse_state.state = JT_MOUSE_CLICK_SIDEBAR_LEFT;
                    mouse_state.sidebar_x = pixel_x - (*global_screen_width - 256);
                    mouse_state.sidebar_y = pixel_y;
                }
                else
                {
                    mouse_state.state = JT_MOUSE_CLICK_LEFT;
                }
                break;

            case SDL_BUTTON_RIGHT:
                right_button_down = 0;
                if (mouse_has_moved)
                {
                    mouse_state.state = JT_MOUSE_RELEASE_RIGHT;
                }
                else if (pixel_x > (*global_screen_width - 256))
                {
                    /* The click was in the sidebar */
                    mouse_state.state = JT_MOUSE_CLICK_SIDEBAR_RIGHT;
                    mouse_state.sidebar_x = pixel_x - (*global_screen_width - 256);
                    mouse_state.sidebar_y = pixel_y;
                }
                else
                {
                    mouse_state.state = JT_MOUSE_CLICK_RIGHT;
                }
                break;

            default:
                break;
        }

        mouse_has_moved = 0;
    }

    mouse_state.x = *global_camera_left + pixel_x / 32.0;
    mouse_state.y = *global_camera_top + pixel_y / 32.0;
    mouse_state.distance_x = (pixel_x - down_pixel_x) / 32.0;
    mouse_state.distance_y = (pixel_y - down_pixel_y) / 32.0;
}
