
typedef struct jt_path_node_struct {
    int x;
    int y;
} jt_path_node;

/* AStar Callbacks */
void jt_path_node_neighbours (ASNeighborList neighbors, void *node, void *context);
float jt_path_node_heuristic (void *fromNode, void *toNode, void *context);
