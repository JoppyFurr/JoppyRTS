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

extern int world [100][100];

/* Instead of using a rectangle of blocks to determine if a path is
 * clear, it would be better to use just the surrounding path nodes.
 *
 * Instead of:
 *
 *   ..............G
 *   ...............
 *   ...............
 *   S..............
 *
 * It would be better to check:
 *
 *           ......G
 *       ...........
 *   ...........
 *   S......
 *
 *   1. Calculate distance (S, G)
 *   2. Compare ( distance (S,n) + distance (n,G) with distancce (S,G).
 *      If the distance via n is significantly longer than a straight
 *      line then we don't need to check it.
 */
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

    jt_path_node north     = { path_node->x,     path_node->y - 1 };
    jt_path_node south     = { path_node->x,     path_node->y + 1 };
    jt_path_node east      = { path_node->x + 1, path_node->y     };
    jt_path_node west      = { path_node->x - 1, path_node->y     };

    jt_path_node northeast = { path_node->x + 1, path_node->y - 1 };
    jt_path_node northwest = { path_node->x - 1, path_node->y - 1 };
    jt_path_node southeast = { path_node->x + 1, path_node->y + 1 };
    jt_path_node southwest = { path_node->x - 1, path_node->y + 1 };

    int have_north = 0;
    int have_south = 0;
    int have_east  = 0;
    int have_west  = 0;

    /* North */
    if (north.y >= 0 && world[north.y][north.x] != 1)
    {
        ASNeighborListAdd(neighbors, &north, 1);
        have_north = 1;
    }
    /* South */
    if (south.y < 100 && world[south.y][south.x] != 1)
    {
        ASNeighborListAdd(neighbors, &south, 1);
        have_south = 1;
    }
    /* East */
    if (east.x < 100 && world[east.y][east.x] != 1)
    {
        ASNeighborListAdd(neighbors, &east, 1);
        have_east = 1;
    }
    /* West */
    if (west.x >= 0 && world[west.y][west.x] != 1)
    {
        ASNeighborListAdd(neighbors, &west, 1);
        have_west = 1;
    }

    /* Northeast */
    if (have_north && have_east && world[northeast.y][northeast.x] != 1)
    {
        ASNeighborListAdd(neighbors, &northeast, 1);
    }
    /* Northwest */
    if (have_north && have_west && world[northwest.y][northwest.x] != 1)
    {
        ASNeighborListAdd(neighbors, &northwest, 1);
    }
    /* Southeast */
    if (have_south && have_east && world[southeast.y][southeast.x] != 1)
    {
        ASNeighborListAdd(neighbors, &southeast, 1);
    }
    /* Southwest */
    if (have_south && have_west && world[southwest.y][southwest.x] != 1)
    {
        ASNeighborListAdd(neighbors, &southwest, 1);
    }
}

float jt_path_node_heuristic (void *fromNode, void *toNode, void *context)
{
    jt_path_node *a = (jt_path_node *) fromNode;
    jt_path_node *b = (jt_path_node *) toNode;

    return sqrt ((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
}
