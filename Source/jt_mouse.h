
#define JT_MOUSE_DEFAULT                0
#define JT_MOUSE_CLICK_LEFT             1
#define JT_MOUSE_CLICK_RIGHT            2
#define JT_MOUSE_DRAG_LEFT              3
#define JT_MOUSE_DRAG_RIGHT             4
#define JT_MOUSE_RELEASE_LEFT           5
#define JT_MOUSE_RELEASE_RIGHT          6
#define JT_MOUSE_CLICK_SIDEBAR_LEFT     7
#define JT_MOUSE_CLICK_SIDEBAR_RIGHT    8

typedef struct jt_mouse_state_struct
{
    /* World-space */
    double x;
    double y;
    double down_x;
    double down_y;
    double distance_x;
    double distance_y;

    /* Sidebar clicks */
    int sidebar_x;
    int sidebar_y;

    /* State */
    int state;

} jt_mouse_state;

jt_mouse_state *jt_mouse_state_get();
void jt_mouse_input (SDL_Event *event);
