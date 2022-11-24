#ifndef QUADTREE_H
#define QUADTREE_H

#include "IntList.h"

#ifdef __cplusplus
#define QTREE_FUNC extern "C"
#else
#define QTREE_FUNC
#endif

typedef struct Quadtree Quadtree;

struct Quadtree {
    // Stores all the nodes in the quadtree. The first node in this
    // sequence is always the root.
    IntList *nodes;

    // Stores all the elements in the quadtree.
    IntList *elts;

    // Stores all the element nodes in the quadtree.
    IntList *enodes;

    // Stores the quadtree extents.
    int root_mx, root_my, root_sx, root_sy;

    // Maximum allowed elements in a leaf before the leaf is subdivided/split unless
    // the leaf is at the maximum allowed tree depth.
    int max_elements;

    // Stores the maximum depth allowed for the quadtree.
    int max_depth;

    // Temporary buffer used for queries.
    char* temp;

    // Stores the size of the temporary buffer.
    int temp_size;
};

// Function signature used for traversing a tree node.
typedef void QtNodeFunc(Quadtree* qt, void* user_data, int node, int depth, int mx, int my, int sx, int sy);

// Creates a quadtree with the requested extents, maximum elements per leaf, and maximum tree depth.
QTREE_FUNC void qt_create(Quadtree* qt, int width, int height, int max_elements, int max_depth);

// Destroys the quadtree.
QTREE_FUNC void qt_destroy(Quadtree* qt);

// Inserts a new element to the tree.
// Returns an index to the new element.
QTREE_FUNC int qt_insert(Quadtree* qt, int id, int x1, int y1, int x2, int y2);

// Removes the specified element from the tree.
QTREE_FUNC void qt_remove(Quadtree* qt, int element);

// Cleans up the tree, removing empty leaves.
QTREE_FUNC void qt_cleanup(Quadtree* qt);

// Outputs a list of elements found in the specified rectangle.
QTREE_FUNC void qt_query(Quadtree* qt, IntList* out, int x1, int y1, int x2, int y2, int omit_element);

// Traverses all the nodes in the tree, calling 'branch' for branch nodes and 'leaf'
// for leaf nodes.
QTREE_FUNC void qt_traverse(Quadtree* qt, void* user_data, QtNodeFunc* branch, QtNodeFunc* leaf);

#endif
