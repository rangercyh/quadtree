// ************************************************************************************
// LGrid.hpp
// ************************************************************************************
#ifndef LGRID_HPP
#define LGRID_HPP

#include "SmallList.hpp"

struct LGridQuery4
{
    // Stores the resulting elements of the SIMD query.
    SmallList<int> elements[4];
};

struct LGridElt
{
    // Stores the index to the next element in the loose cell using an indexed SLL.
    int next;

    // Stores the ID of the element. This can be used to associate external
    // data to the element.
    int id;

    // Stores the center of the element.
    float mx, my;

    // Stores the half-size of the element relative to the upper-left corner
    // of the grid.
    float hx, hy;
};

struct LGridLooseCell
{
    // Stores the extents of the grid cell relative to the upper-left corner
    // of the grid which expands and shrinks with the elements inserted and
    // removed.
    float rect[4];

    // Stores the index to the first element using an indexed SLL.
    int head;
};

struct LGridLoose
{
    // Stores all the cells in the loose grid.
    LGridLooseCell* cells;

    // Stores the number of columns, rows, and cells in the loose grid.
    int num_cols, num_rows, num_cells;

    // Stores the inverse size of a loose cell.
    float inv_cell_w, inv_cell_h;
};

struct LGridTightCell
{
    // Stores the index to the next loose cell in the grid cell.
    int next;

    // Stores the position of the loose cell in the grid.
    int lcell;
};

struct LGridTight
{
    // Stores all the tight cell nodes in the grid.
    FreeList<LGridTightCell> cells;

    // Stores the tight cell heads.
    int* heads;

    // Stores the number of columns, rows, and cells in the tight grid.
    int num_cols, num_rows, num_cells;

    // Stores the inverse size of a tight cell.
    float inv_cell_w, inv_cell_h;
};

struct LGrid
{
    // Stores the tight cell data for the grid.
    LGridTight tight;

    // Stores the loose cell data for the grid.
    LGridLoose loose;

    // Stores all the elements in the grid.
    FreeList<LGridElt> elts;

    // Stores the number of elements in the grid.
    int num_elts;

    // Stores the upper-left corner of the grid.
    float x, y;

    // Stores the size of the grid.
    float w, h;
};

// Creates a loose grid encompassing the specified extents using the specified cell
// size. Elements inserted to the loose grid are only inserted in one cell, but the
// extents of each cell are allowed to expand and shrink. To avoid requiring every
// loose cell to be checked during a search, a second grid of tight cells referencing
// the loose cells is stored.
LGrid* lgrid_create(float lcell_w, float lcell_h, float tcell_w, float tcell_h,
                    float l, float t, float r, float b);

// Destroys the grid.
void lgrid_destroy(LGrid* grid);

// Returns the grid cell index for the specified position.
int lgrid_lcell_idx(LGrid* grid, float x, float y);

// Inserts an element to the grid.
void lgrid_insert(LGrid* grid, int id, float mx, float my, float hx, float hy);

// Removes an element from the grid.
void lgrid_remove(LGrid* grid, int id, float mx, float my);

// Moves an element in the grid from the former position to the new one.
void lgrid_move(LGrid* grid, int id, float prev_mx, float prev_my, float mx, float my);

// Returns all the element IDs that intersect the specified rectangle excluding elements
// with the specified ID to omit.
SmallList<int> lgrid_query(const LGrid* grid, float mx, float my, float hx, float hy, int omit_id);

// Returns all the element IDs that intersect the specified 4 rectangles excluding elements
// with the specified IDs to omit.
LGridQuery4 lgrid_query4(const LGrid* grid, const SimdVec4f* mx4, const SimdVec4f* my4,
                         const SimdVec4f* hx4, const SimdVec4f* hy4, const SimdVec4i* omit_id4);

// Returns true if the specified rectangle is inside the grid boundaries.
bool lgrid_in_bounds(const LGrid* grid, float mx, float my, float hx, float hy);

// Optimizes the grid, shrinking bounding boxes in response to removed elements and
// rearranging the memory of the grid to allow cache-friendly cell traversal.
void lgrid_optimize(LGrid* grid);

#endif
