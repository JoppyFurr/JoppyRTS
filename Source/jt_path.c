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

int jt_path_is_clear (jt_path_node *a, jt_path_node *b)
{
    int min_x = a->x < b->x ? a->x : b->x;
    int max_x = a->x < b->x ? b->x : a->x;
    int min_y = a->y < b->y ? a->y : b->y;
    int max_y = a->y < b->y ? b->y : a->y;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            if (world[y][x] == 1)
            {
                return 0;
            }
        }
    }
    return 1;
}

void jt_path_free (jt_path *path)
{
    free (path->nodes);
    free (path);
}

/*
 * Note: This works okay, although could use some improvement.
 *
 * Consider:
 *
 *       s  #  g
 *          #
 *
 * A diagonal will be taken on the left side, but not the right side.
 * This occurrs due to the node above the wall not being able to extend
 * down, so it gets locked in the y axis. A better solution would allow
 * a new block to begin as soon as the wall is out of the way.
 *
 * Instead of:
 *       aa*bbb*
 *       aaa#  c
 *          #
 *
 * It would be better to get:
 *       aa*b*cc
 *       aaa#ccc
 *          #
 *
 * One possibility is to force a node into the path when we detect that
 * we have gone past an obstacle. Or when we detect a non-clear block,
 * find out where in the non-extending axis the obstruction occurrs and
 * force a node at that point in the non-extending direction.
 */
jt_path *jt_path_simplify (ASPath original_path)
{
    jt_path *new_path = malloc (sizeof (jt_path));
    new_path->nodes = malloc (sizeof (jt_path_node));
    new_path->nodes[0] = *(jt_path_node *)ASPathGetNode (original_path, 0);
    int count = 1;

    jt_path_node *last_clear = NULL;
    jt_path_node *n = NULL;

    for (int i = 0; i < ASPathGetCount (original_path); i++)
    {
        n = ASPathGetNode (original_path, i);

        if (jt_path_is_clear (&new_path->nodes[count-1], n))
        {
            last_clear = n;
        }
        else
        {
            count++;
            new_path->nodes = realloc (new_path->nodes, sizeof (jt_path_node) * count);
            new_path->nodes[count-1] = *last_clear;
            i--;
        }
    }

    /* Ensure the final goal is in the list too */
    count++;
    new_path->nodes = realloc (new_path->nodes, sizeof (jt_path_node) * count);
    new_path->nodes[count-1] = *n;
    new_path->count = count;

    return new_path;
}

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
