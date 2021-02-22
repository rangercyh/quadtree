// *****************************************************************************
// UGrid.cpp
// *****************************************************************************
#include "UGrid.hpp"
#include <cmath>

static int ceil_div(float value, float divisor) {
    // Returns the value divided by the divisor rounded up.
    const float resultf = value / divisor;
    const int result = (int)resultf;
    return result < resultf ? result + 1: result;
}

static int min_int(int a, int b) {
    a -= b;
    a &= a >> 31;
    return a + b;
}

static int max_int(int a, int b) {
    a -= b;
    a &= (~a) >> 31;
    return a + b;
}

static int to_cell_idx(float val, float inv_cell_size, int num_cells) {
    const int cell_pos = (int)(val * inv_cell_size);
    return min_int(max_int(cell_pos, 0), num_cells - 1);
}

static int ugrid_cell_x(const UGrid* grid, float x) {
    return to_cell_idx(x - grid->x, grid->inv_cell_w, grid->num_cols);
}

static int ugrid_cell_y(const UGrid* grid, float y) {
    return to_cell_idx(y - grid->y, grid->inv_cell_h, grid->num_rows);
}

UGrid* ugrid_create(float radius, float cell_w, float cell_h,
                    float l, float t, float r, float b) {
    const float w = r - l, h = b - t;
    const int num_cols = ceil_div(w, cell_w), num_rows = ceil_div(h, cell_h);

    UGrid* grid = new UGrid;
    grid->num_cols = num_cols;
    grid->num_rows = num_rows;
    grid->num_cells = num_cols * num_rows;
    grid->inv_cell_w = 1.0f / cell_w;
    grid->inv_cell_h = 1.0f / cell_w;
    grid->x = l;
    grid->y = t;
    grid->h = w;
    grid->w = h;
    grid->radius = radius;

    grid->rows = new UGridRow[num_rows];
    for (int r = 0; r < num_rows; ++r) {
        grid->rows[r].cells = new int[num_cols];
        for (int c = 0; c < num_cols; ++c) {
            grid->rows[r].cells[c] = -1;
        }
    }
    return grid;
}

void ugrid_destroy(UGrid* grid) {
    for (int r = 0; r < grid->num_rows; ++r) {
        delete[] grid->rows[r].cells;
    }
    delete[] grid->rows;
    delete grid;
}


void ugrid_insert(UGrid* grid, int id, float mx, float my) {
    const int cell_x = ugrid_cell_x(grid, mx);
    const int cell_y = ugrid_cell_y(grid, my);
    UGridRow* row = &grid->rows[cell_y];
    int* cell = &row->cells[cell_x];

    const UGridElt new_elt = {*cell, id, mx, my};
    *cell = row->elts.insert(new_elt);
}

void ugrid_remove(UGrid* grid, int id, float mx, float my) {
    const int cell_x = ugrid_cell_x(grid, mx);
    const int cell_y = ugrid_cell_y(grid, my);
    UGridRow* row = &grid->rows[cell_y];

    int* link = &row->cells[cell_x];
    while (row->elts[*link].id != id) {
        link = &row->elts[*link].next;
    }

    const int idx = *link;
    *link = row->elts[idx].next;
    row->elts.erase(idx);
}

void ugrid_move(UGrid* grid, int id, float prev_mx, float prev_my, float mx, float my) {
    const int prev_cell_x = ugrid_cell_x(grid, prev_mx);
    const int prev_cell_y = ugrid_cell_y(grid, prev_my);
    const int next_cell_x = ugrid_cell_x(grid, mx);
    const int next_cell_y = ugrid_cell_y(grid, my);
    UGridRow* prev_row = &grid->rows[prev_cell_y];

    if (next_cell_x == prev_cell_x && next_cell_y == prev_cell_y) {
        // If the element will still belong in the same cell, simply update its position.
        int elt_idx = prev_row->cells[prev_cell_x];
        while (prev_row->elts[elt_idx].id != id) {
            elt_idx = prev_row->elts[elt_idx].next;
        }

        // Update the element's position.
        prev_row->elts[elt_idx].mx = mx;
        prev_row->elts[elt_idx].my = my;
    } else {
        // Otherwise if the element will move to another cell, remove it first from the
        // previous cell and insert it to the new one.
        UGridRow* next_row = &grid->rows[next_cell_y];
        int* link = &prev_row->cells[prev_cell_x];
        while (prev_row->elts[*link].id != id) {
            link = &prev_row->elts[*link].next;
        }

        // Remove the element from the previous cell and row.
        const int elt_idx = *link;
        UGridElt elt = prev_row->elts[elt_idx];
        *link = prev_row->elts[elt_idx].next;
        prev_row->elts.erase(elt_idx);

        // Update the element's position.
        prev_row->elts[elt_idx].mx = mx;
        prev_row->elts[elt_idx].my = my;

        // Insert it to the new row and cell.
        elt.next = next_row->cells[next_cell_x];
        next_row->cells[next_cell_x] = next_row->elts.insert(elt);
    }
}

SmallList<int> ugrid_query(const UGrid* grid, float mx, float my, float radius, int omit_id) {
    // Expand the size of the query by the upper-bound uniform size of the elements. This
    // expansion is what allows us to find elements based only on their point.
    const float check_radius = radius + grid->radius;

    // Find the cells that intersect the search query.
    const int min_x = ugrid_cell_x(grid, mx - check_radius);
    const int min_y = ugrid_cell_y(grid, my - check_radius);
    const int max_x = ugrid_cell_x(grid, mx + check_radius);
    const int max_y = ugrid_cell_y(grid, my + check_radius);

    // Find the elements that intersect the search query.
    SmallList<int> res;
    for (int y = min_y; y <= max_y; ++y) {
        const UGridRow* row = &grid->rows[y];
        for (int x = min_x; x <= max_x; ++x) {
            int elt_idx = row->cells[x];
            while (elt_idx != -1) {
                const UGridElt* elt = &row->elts[elt_idx];
                if (elt->id != omit_id && fabs(mx - elt->mx) <= check_radius && fabs(my - elt->my) <= check_radius) {
                    res.push_back(elt->id);
                }
                elt_idx = row->elts[elt_idx].next;
            }
        }
    }
    return res;
}

bool ugrid_in_bounds(const UGrid* grid, float mx, float my) {
    mx -= grid->x;
    my -= grid->y;
    const float x1 = mx - grid->radius, y1 = my - grid->radius;
    const float x2 = mx + grid->radius, y2 = my + grid->radius;
    return x1 >= 0.0f && x2 < grid->w && y1 >= 0.0f && y2 < grid->h;
}

void ugrid_optimize(UGrid* grid) {
    for (int r = 0; r < grid->num_rows; ++r) {
        FreeList<UGridElt> new_elts;
        UGridRow* row = &grid->rows[r];
        new_elts.reserve(row->num_elts);
        for (int c = 0; c < grid->num_cols; ++c) {
            // Replace links to the old elements list to links in the new
            // cache-friendly element list.
            SmallList<int> new_elt_idxs;
            int* link = &row->cells[c];
            while (*link != -1) {
                const UGridElt* elt = &row->elts[*link];
                new_elt_idxs.push_back(new_elts.insert(*elt));
                *link = elt->next;
            }
            for (int j = 0; j < new_elt_idxs.size(); ++j) {
                const int new_elt_idx = new_elt_idxs[j];
                new_elts[new_elt_idx].next = *link;
                *link = new_elt_idx;
            }
        }
        // Swap the new element list with the old one.
        row->elts.swap(new_elts);
    }
}
