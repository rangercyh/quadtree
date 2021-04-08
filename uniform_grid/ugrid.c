#include "ugrid.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

void ugrid_create(UGrid* grid, float radius, float cell_w, float cell_h,
                    float l, float t, float r, float b) {
    const float w = r - l, h = b - t;
    const int num_cols = ceil_div(w, cell_w), num_rows = ceil_div(h, cell_h);

    grid->num_cols = num_cols;
    grid->num_rows = num_rows;
    grid->num_cells = num_cols * num_rows;
    grid->inv_cell_w = 1.0f / cell_w;
    grid->inv_cell_h = 1.0f / cell_w;
    grid->x = l;
    grid->y = t;
    grid->h = h;
    grid->w = w;
    grid->radius = radius;

    grid->rows = (UGridRow *)malloc(num_rows * sizeof(UGridRow));
    for (int r = 0; r < num_rows; ++r) {
        grid->rows[r].cells = (int *)malloc(num_cols * sizeof(int));
        for (int c = 0; c < num_cols; ++c) {
            grid->rows[r].cells[c] = -1;
        }
        grid->rows[r].num_elts = 0;
        grid->rows[r].efl = efl_create();
    }
}

void ugrid_destroy(UGrid* grid) {
    for (int r = 0; r < grid->num_rows; ++r) {
        efl_destroy(grid->rows[r].efl);
        free(grid->rows[r].cells);
    }
    free(grid->rows);
}

int ugrid_insert(UGrid* grid, int id, float mx, float my) {
    const int cell_x = ugrid_cell_x(grid, mx);
    const int cell_y = ugrid_cell_y(grid, my);
    UGridRow* row = &grid->rows[cell_y];
    int* cell = &row->cells[cell_x];
    int temp = efl_insert(row->efl, id, mx, my, *cell);
    *cell = temp;
    return *cell;
}

void ugrid_remove(UGrid* grid, int id, float mx, float my) {
    const int cell_x = ugrid_cell_x(grid, mx);
    const int cell_y = ugrid_cell_y(grid, my);
    UGridRow* row = &grid->rows[cell_y];

    int* link = &row->cells[cell_x];
    Elt* elt = efl_get(row->efl, *link);
    while (elt && elt->id != id) {
        link = &elt->next;
        elt = efl_get(row->efl, *link);
    }
    if (elt) {
        const int idx = *link;
        *link = elt->next;
        efl_remove(row->efl, idx);
    }
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
        Elt* elt = efl_get(prev_row->efl, elt_idx);
        while (elt && elt->id != id) {
            elt = efl_get(prev_row->efl, elt->next);
        }
        if (elt) {
            // Update the element's position.
            elt->mx = mx;
            elt->my = my;
        }
    } else {
        // Otherwise if the element will move to another cell, remove it first from the
        // previous cell and insert it to the new one.
        UGridRow* next_row = &grid->rows[next_cell_y];
        int* link = &prev_row->cells[prev_cell_x];
        Elt* elt = efl_get(prev_row->efl, *link);
        while (elt && elt->id != id) {
            link = &elt->next;
            elt = efl_get(prev_row->efl, *link);
        }
        if (elt) {
            // Remove the element from the previous cell and row.
            const int elt_idx = *link;
            *link = elt->next;
            efl_remove(prev_row->efl, elt_idx);

            // Update the element's position.
            elt->mx = mx;
            elt->my = my;

            // Insert it to the new row and cell.
            elt->next = next_row->cells[next_cell_x];
            next_row->cells[next_cell_x] =  efl_insert(next_row->efl, elt->id, elt->mx,
                    elt->my, elt->next);
        }
    }
}

void ugrid_query(const UGrid* grid, IntList* out, float mx, float my,
                    float radius, int omit_id) {
    // Expand the size of the query by the upper-bound uniform size of the elements. This
    // expansion is what allows us to find elements based only on their point.
    const float check_radius = radius + grid->radius;

    // Find the cells that intersect the search query.
    const int min_x = ugrid_cell_x(grid, mx - check_radius);
    const int min_y = ugrid_cell_y(grid, my - check_radius);
    const int max_x = ugrid_cell_x(grid, mx + check_radius);
    const int max_y = ugrid_cell_y(grid, my + check_radius);

    // Find the elements that intersect the search query.
    il_clear(out);
    for (int y = min_y; y <= max_y; ++y) {
        const UGridRow* row = &grid->rows[y];
        for (int x = min_x; x <= max_x; ++x) {
            int elt_idx = row->cells[x];
            while (elt_idx != -1) {
                const Elt* elt = efl_get(row->efl, elt_idx);
                if (fabs(mx - elt->mx) <= check_radius && fabs(my - elt->my) <= check_radius &&
                    elt->id != omit_id) {
                    il_set(out, il_push_back(out), 0, elt->id);
                }
                elt_idx = elt->next;
            }
        }
    }
}

int ugrid_in_bounds(const UGrid* grid, float mx, float my) {
    mx -= grid->x;
    my -= grid->y;
    const float x1 = mx - grid->radius, y1 = my - grid->radius;
    const float x2 = mx + grid->radius, y2 = my + grid->radius;
    return x1 >= 0.0f && x2 < grid->w && y1 >= 0.0f && y2 < grid->h;
}

void ugrid_optimize(UGrid* grid) {
    for (int r = 0; r < grid->num_rows; ++r) {
        EltFreeList *new_efl = efl_create();
        UGridRow* row = &grid->rows[r];
        efl_reserve(new_efl, row->num_elts);
        for (int c = 0; c < grid->num_cols; ++c) {
            // Replace links to the old elements list to links in the new
            // cache-friendly element list.
            IntList new_elt_idxs;
            il_create(&new_elt_idxs, 1);
            int* link = &row->cells[c];
            while (*link != -1) {
                const Elt* elt = efl_get(row->efl, *link);
                il_set(&new_elt_idxs, il_push_back(&new_elt_idxs), 0,
                        efl_insert(new_efl, elt->id, elt->mx, elt->my, elt->next));
                *link = elt->next;
            }
            for (int j = 0; j < il_size(&new_elt_idxs); ++j) {
                const int new_elt_idx = il_get(&new_elt_idxs, j, 0);
                Elt* elt = efl_get(new_efl, new_elt_idx);
                elt->next = *link;
                *link = new_elt_idx;
            }
        }
        efl_destroy(row->efl);
        row->efl = new_efl;
    }
}
