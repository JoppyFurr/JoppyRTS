
typedef struct jt_path_node_struct
{
    int x;
    int y;
} jt_path_node;

typedef struct jt_path_struct
{
    jt_path_node *nodes;
    int count;
} jt_path;

/* Function declarations */
jt_path *jt_path_simplify (ASPath original_path);
void jt_path_free (jt_path *path);
int jt_path_is_clear (jt_path_node *a, jt_path_node *b);

/* AStar Callbacks */
void jt_path_node_neighbours (ASNeighborList neighbors, void *node, void *context);
float jt_path_node_heuristic (void *fromNode, void *toNode, void *context);
