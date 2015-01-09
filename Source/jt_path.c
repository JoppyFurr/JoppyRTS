#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Libraries/AStar/AStar.h"

#include "jt_path.h"

/* Chosen pathfinding algorithm:
 *
 *   Step 1: Use A* to find a path from Start to Goal avoiding buildings.
 *   Step 2: Simplify the path by grouping chunks of path into blocks 
 *           where no building collisions will occur.
 *   Step 3: Use a force-based movement system to avoid units while
 *           traversing the simplified path. If a building is found to
 *           have been added, then go back to Step 1.
 */

/* TODO: Get rid of this [33][60] stuff */

extern int world [33][60];

void jt_path_node_neighbours (ASNeighborList neighbors, void *node, void *context)
{
    jt_path_node *path_node = (jt_path_node *) node;

    jt_path_node north = { path_node->x,     path_node->y - 1 };
    jt_path_node south = { path_node->x,     path_node->y + 1 };
    jt_path_node east  = { path_node->x + 1, path_node->y     };
    jt_path_node west  = { path_node->x - 1, path_node->y     };

    /* North */
    if (north.y >= 0 && world[north.y][north.x] == 0)
    {
        ASNeighborListAdd(neighbors, &north, 1);
    }
    /* South */
    if (south.y < 33 && world[south.y][south.x] == 0)
    {
        ASNeighborListAdd(neighbors, &south, 1);
    }
    /* East */
    if (east.x < 60 && world[east.y][east.x] == 0)
    {
        ASNeighborListAdd(neighbors, &east, 1);
    }
    /* West */
    if (west.x >= 0 && world[west.y][west.x] == 0)
    {
        ASNeighborListAdd(neighbors, &west, 1);
    }
}

float jt_path_node_heuristic (void *fromNode, void *toNode, void *context)
{
    jt_path_node *a = (jt_path_node *) fromNode;
    jt_path_node *b = (jt_path_node *) toNode;

    return sqrt ((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
}
