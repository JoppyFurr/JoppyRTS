#define JT_RENDER_NONE 0
#define JT_RENDER_WALL 1
#define JT_RENDER_TENT 2
typedef struct jt_world_cell_struct
{
    int contains_unit;
    int contains_building;
    int render_as;

} jt_world_cell;

