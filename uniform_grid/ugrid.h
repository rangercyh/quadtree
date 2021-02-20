#ifndef UGRID_H
#define UGRID_H

#include "elt_free_list.h"
#include "IntList.h"

#ifdef __cplusplus
#define UGRID_FUNC extern "C"
#else
#define UGRID_FUNC
#endif

typedef struct UGrid UGrid;
typedef struct UGridRow UGridRow;

struct UGridRow {
    // Stores all the elements in the grid row.
    EltFreeList* efl;

    // Stores all the cells in the row. Each cell stores an index pointing to
    // the first element in that cell, or -1 if the cell is empty.
    int* cells;

    // Stores the number of elements in the row.
    int num_elts;
};

struct UGrid {
    // Stores all the rows in the grid.
    UGridRow* rows;

    // Stores the number of columns, rows, and cells in the grid.
    int num_cols, num_rows, num_cells;

    // Stores the inverse size of a cell.
    float inv_cell_w, inv_cell_h;

    // Stores radius of all elements stored in the grid.
    float radius;

    // Stores the upper-left corner of the grid.
    float x, y;

    // Stores the size of the grid.
    float w, h;
};

// Creates a new grid storing elements that have a uniform upper-bound size. Because
// all elements are treated uniformly-sized for the sake of search queries, each one
// can be stored as a single point in the grid.
UGRID_FUNC void ugrid_create(UGrid* grid, float radius, float cell_w, float cell_h,
                    float l, float t, float r, float b);

// Destroys the grid.
UGRID_FUNC void ugrid_destroy(UGrid* grid);

// Inserts an element to the grid.
UGRID_FUNC int ugrid_insert(UGrid* grid, int id, float mx, float my);

// Removes an element from the grid.
UGRID_FUNC void ugrid_remove(UGrid* grid, int id, float mx, float my);

// Moves an element in the grid from the former position to the new one.
UGRID_FUNC void ugrid_move(UGrid* grid, int id, float prev_mx, float prev_my,
                    float mx, float my);

// Returns all the element IDs that intersect the specified radius excluding
// elements with the specified ID to omit.
UGRID_FUNC void ugrid_query(const UGrid* grid, IntList* out, float mx, float my,
                    float radius, int omit_id);

// Returns true if the specified element position is inside the grid boundaries.
UGRID_FUNC int ugrid_in_bounds(const UGrid* grid, float mx, float my);

// Optimizes the grid, rearranging the memory of the grid to allow cache-friendly
// cell traversal.
UGRID_FUNC void ugrid_optimize(UGrid* grid);

#endif
