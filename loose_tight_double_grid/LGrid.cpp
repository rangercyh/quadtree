// ************************************************************************************
// LGrid.cpp
// ************************************************************************************
#include "LGrid.hpp"
#include <cstdlib>
#include <cfloat>
#include <utility>
#include <algorithm>

static int ceil_div(float value, float divisor)
{
    // Returns the value divided by the divisor rounded up.
    const float resultf = value / divisor;
    const int result = (int)resultf;
    return result < resultf ? result+1: result;
}

static int min_int(int a, int b)
{
    assert(sizeof(int) == 4);
    a -= b;
    a &= a >> 31;
    return a + b;
}

static int max_int(int a, int b)
{
    assert(sizeof(int) == 4);
    a -= b;
    a &= (~a) >> 31;
    return a + b;
}

static float min_flt(float a, float b)
{
    return std::min(a, b);
}

static float max_flt(float a, float b)
{
    return std::max(a, b);
}

static int to_cell_idx(float val, float inv_cell_size, int num_cells)
{
    const int cell_pos = (int)(val * inv_cell_size);
    return min_int(max_int(cell_pos, 0), num_cells - 1);
}

static float clampf(float num, float min, float max) {
    return num > max ? max : num < min ? min : num;
}

static int intersects(const float rect[4], const float other[4]) {
    return rect[0] <= other[2] && rect[2] >= other[0] && rect[1] <= other[3] && rect[3] >= other[1];
}

struct rect {
    float data[4];
};

static struct rect to_tcell_idx4(const LGrid* grid, float min_x, float min_y, float max_x, float max_y)
{
    min_x *= grid->tight.inv_cell_w;
    min_y *= grid->tight.inv_cell_h;
    max_x *= grid->tight.inv_cell_w;
    max_y *= grid->tight.inv_cell_h;

    min_x = clampf(min_x, 0, grid->tight.num_cols - 1);
    min_y = clampf(min_y, 0, grid->tight.num_rows - 1);
    max_x = clampf(max_x, 0, grid->tight.num_cols - 1);
    max_y = clampf(max_y, 0, grid->tight.num_rows - 1);

    return (struct rect) {
        .data = { min_x, min_y, max_x, max_y }
    };
}

static void grid_optimize(LGrid* grid)
{
    FreeList<LGridElt> new_elts;
    new_elts.reserve(grid->num_elts);
    for (int c=0; c < grid->loose.num_cells; ++c)
    {
        // Replace links to the old elements list to links in the new
        // cache-friendly element list.
        SmallList<int> new_elt_idxs;
        LGridLooseCell* lcell = &grid->loose.cells[c];
        while (lcell->head != -1)
        {
            const LGridElt* elt = &grid->elts[lcell->head];
            new_elt_idxs.push_back(new_elts.insert(*elt));
            lcell->head = elt->next;
        }
        for (int j=0; j < new_elt_idxs.size(); ++j)
        {
            const int new_elt_idx = new_elt_idxs[j];
            new_elts[new_elt_idx].next = lcell->head;
            lcell->head = new_elt_idx;
        }
    }
    // Swap the new element list with the old one.
    grid->elts.swap(new_elts);
}

static void expand_aabb(LGrid* grid, int cell_idx, float mx, float my, float hx, float hy)
{
    LGridLooseCell* lcell = &grid->loose.cells[cell_idx];
    const struct rect prev_rect = (struct rect){ .data = {lcell->rect[0], lcell->rect[1], lcell->rect[2], lcell->rect[3]} };
    lcell->rect[0] = min_flt(lcell->rect[0], mx - hx);
    lcell->rect[1] = min_flt(lcell->rect[1], my - hy);
    lcell->rect[2] = max_flt(lcell->rect[2], mx + hx);
    lcell->rect[3] = max_flt(lcell->rect[3], my + hy);

    // Determine the cells occupied by the loose cell in the tight grid.
    const struct rect trect = to_tcell_idx4(grid, mx-hx, my-hy, mx+hx, my+hy);

    if (prev_rect.data[0] > prev_rect.data[2])
    {
        // If the loose cell was empty, simply insert the loose cell
        // to all the tight cells it occupies. We don't need to check
        // to see if it was already inserted.
        for (int ty = trect.data[1]; ty <= trect.data[3]; ++ty)
        {
            int* tight_row = grid->tight.heads + ty*grid->tight.num_cols;
            for (int tx = trect.data[0]; tx <= trect.data[2]; ++tx)
            {
                const LGridTightCell new_tcell = {tight_row[tx], cell_idx};
                tight_row[tx] = grid->tight.cells.insert(new_tcell);
            }
        }
    }
    else
    {
        // Only perform the insertion if the loose cell overlaps new tight cells.
        const struct rect prev_trect = to_tcell_idx4(grid, prev_rect.data[0], prev_rect.data[1], prev_rect.data[2], prev_rect.data[3]);
        if (trect.data[0] != prev_trect.data[0] || trect.data[1] != prev_trect.data[1] ||
            trect.data[2] != prev_trect.data[2] || trect.data[3] != prev_trect.data[3])
        {
            for (int ty = trect.data[1]; ty <= trect.data[3]; ++ty)
            {
                int* tight_row = grid->tight.heads + ty*grid->tight.num_cols;
                for (int tx = trect.data[0]; tx <= trect.data[2]; ++tx)
                {
                    if (tx < prev_trect.data[0] || tx > prev_trect.data[2] ||
                        ty < prev_trect.data[1] || ty > prev_trect.data[3])
                    {
                        const LGridTightCell new_tcell = {tight_row[tx], cell_idx};
                        tight_row[tx] = grid->tight.cells.insert(new_tcell);
                    }
                }
            }
        }
    }
}

LGrid* lgrid_create(float lcell_w, float lcell_h, float tcell_w, float tcell_h,
                    float l, float t, float r, float b)
{
    const float w = r - l, h = b - t;
    const int num_lcols = ceil_div(w, lcell_w), num_lrows = ceil_div(h, lcell_h);
    const int num_tcols = ceil_div(w, tcell_w), num_trows = ceil_div(h, tcell_h);

    LGrid* grid = new LGrid;
    grid->num_elts = 0;
    grid->x = l;
    grid->y = t;
    grid->h = w;
    grid->w = h;

    grid->loose.num_cols = num_lcols;
    grid->loose.num_rows = num_lrows;
    grid->loose.num_cells = grid->loose.num_cols * grid->loose.num_rows;
    grid->loose.inv_cell_w = 1.0f / lcell_w;
    grid->loose.inv_cell_h = 1.0f / lcell_h;

    grid->tight.num_cols = num_tcols;
    grid->tight.num_rows = num_trows;
    grid->tight.num_cells = grid->tight.num_cols * grid->tight.num_rows;
    grid->tight.inv_cell_w = 1.0f / tcell_w;
    grid->tight.inv_cell_h = 1.0f / tcell_h;

    // Initialize tight cell heads with -1 to indicate empty indexed SLLs.
    grid->tight.heads = new int[grid->tight.num_cells];
    for (int j=0; j < grid->tight.num_cells; ++j)
        grid->tight.heads[j] = -1;

    // Initialize all the loose cells.
    grid->loose.cells = new LGridLooseCell[grid->loose.num_cells];
    for (int c=0; c < grid->loose.num_cells; ++c)
    {
        grid->loose.cells[c].head = -1;
        grid->loose.cells[c].rect[0] = FLT_MAX;
        grid->loose.cells[c].rect[1] = FLT_MAX;
        grid->loose.cells[c].rect[2] = -FLT_MAX;
        grid->loose.cells[c].rect[3] = -FLT_MAX;
    }
    return grid;
}

void lgrid_destroy(LGrid* grid)
{
    delete[] grid->loose.cells;
    delete[] grid->tight.heads;
    delete grid;
}

int lgrid_lcell_idx(LGrid* grid, float x, float y)
{
    const int cell_x = to_cell_idx(x - grid->x, grid->loose.inv_cell_w, grid->loose.num_cols);
    const int cell_y = to_cell_idx(y - grid->y, grid->loose.inv_cell_h, grid->loose.num_rows);
    return cell_y * grid->loose.num_cols + cell_x;
}

void lgrid_insert(LGrid* grid, int id, float mx, float my, float hx, float hy)
{
    const int cell_idx = lgrid_lcell_idx(grid, mx, my);
    LGridLooseCell* lcell = &grid->loose.cells[cell_idx];

    // Insert the element to the appropriate loose cell and row.
    const LGridElt new_elt = {lcell->head, id, mx - grid->x, my - grid->y, hx, hy};
    lcell->head = grid->elts.insert(new_elt);
    ++grid->num_elts;

    // Expand the loose cell's bounding box to fit the new element.
    expand_aabb(grid, cell_idx, mx, my, hx, hy);
}

void lgrid_remove(LGrid* grid, int id, float mx, float my)
{
    // Find the element in the loose cell.
    LGridLooseCell* lcell = &grid->loose.cells[lgrid_lcell_idx(grid, mx, my)];
    int* link = &lcell->head;
    while (grid->elts[*link].id != id)
        link = &grid->elts[*link].next;

    // Remove the element from the loose cell and row.
    const int elt_idx = *link;
    *link = grid->elts[elt_idx].next;
    grid->elts.erase(elt_idx);
    --grid->num_elts;
}

void lgrid_move(LGrid* grid, int id, float prev_mx, float prev_my, float mx, float my)
{
    const int prev_cell_idx = lgrid_lcell_idx(grid, prev_mx, prev_my);
    const int new_cell_idx = lgrid_lcell_idx(grid, mx, my);
    LGridLooseCell* lcell = &grid->loose.cells[prev_cell_idx];

    if (prev_cell_idx == new_cell_idx)
    {
        // Find the element in the loose cell.
        int elt_idx = lcell->head;
        while (grid->elts[elt_idx].id != id)
            elt_idx = grid->elts[elt_idx].next;

        // Since the element is still inside the same cell, we can simply overwrite
        // its position and expand the loose cell's AABB.
        mx -= grid->x;
        my -= grid->y;
        grid->elts[elt_idx].mx = mx;
        grid->elts[elt_idx].my = my;
        expand_aabb(grid, prev_cell_idx, mx, my, grid->elts[elt_idx].hx, grid->elts[elt_idx].hy);
    }
    else
    {
        // Find the element in the loose cell.
        int* link = &lcell->head;
        while (grid->elts[*link].id != id)
            link = &grid->elts[*link].next;

        const int elt_idx = *link;
        const float hx = grid->elts[elt_idx].hx;
        const float hy = grid->elts[elt_idx].hy;

        // If the element has moved into a different loose cell, remove
        // remove the element from the previous loose cell and row.
        *link = grid->elts[elt_idx].next;
        grid->elts.erase(elt_idx);
        --grid->num_elts;

        // Now insert the element to its new position.
        lgrid_insert(grid, id, mx, my, hx, hy);
    }
}

SmallList<int> lgrid_query(const LGrid* grid, float mx, float my, float hx, float hy, int omit_id)
{
    mx -= grid->x;
    my -= grid->y;

    // Compute the tight cell extents [min_tx, min_ty, max_tx, max_ty].
    const float qrect[4] = {mx-hx, my-hy, mx+hx, my+hy};
    const struct rect trect = to_tcell_idx4(grid, qrect[0], qrect[1], qrect[2], qrect[3]);

    // Gather the intersecting loose cells in the tight cells that intersect.
    SmallList<int> lcell_idxs;
    for (int ty = trect.data[1]; ty <= trect.data[3]; ++ty)
    {
        const int* tight_row = grid->tight.heads + ty*grid->tight.num_cols;
        for (int tx = trect.data[0]; tx <= trect.data[2]; ++tx)
        {
            // Iterate through the loose cells that intersect the tight cells.
            int tcell_idx = tight_row[tx];
            while (tcell_idx != -1)
            {
                const LGridTightCell* tcell = &grid->tight.cells[tcell_idx];
                const LGridLooseCell* lcell = &grid->loose.cells[tcell->lcell];
                if (lcell_idxs.find_index(tcell->lcell) == -1 && intersects(qrect, lcell->rect))
                    lcell_idxs.push_back(tcell->lcell);
                tcell_idx = tcell->next;
            }
        }
    }

    // For each loose cell, determine what elements intersect.
    SmallList<int> res;
    for (int j=0; j < lcell_idxs.size(); ++j)
    {
        const LGridLooseCell* lcell = &grid->loose.cells[lcell_idxs[j]];
        int elt_idx = lcell->head;
        while (elt_idx != -1)
        {
            // If the element intersects the search rectangle, add it to the
            // resulting elements unless it has an ID that should be omitted.
            const LGridElt* elt = &grid->elts[elt_idx];
            float nums[4] = {elt->mx-elt->hx,elt->my-elt->hy,elt->mx+elt->hx,elt->my+elt->hy};
            if (elt->id != omit_id && intersects(qrect, nums))
                res.push_back(elt->id);
            elt_idx = elt->next;
        }
    }
    return res;
}

bool lgrid_in_bounds(const LGrid* grid, float mx, float my, float hx, float hy)
{
    mx -= grid->x;
    my -= grid->y;
    const float x1 = mx-hx, y1 = my-hy, x2 = mx+hx, y2 = my+hy;
    return x1 >= 0.0f && x2 < grid->w && y1 >= 0.0f && y2 < grid->h;
}

void lgrid_optimize(LGrid* grid)
{
    // Clear all the tight cell data.
    for (int j=0; j < grid->tight.num_cells; ++j)
        grid->tight.heads[j] = -1;
    grid->tight.cells.clear();

    // Optimize the memory layout of the grid.
    grid_optimize(grid);

    #pragma omp parallel for
    for (int c=0; c < grid->loose.num_cells; ++c)
    {
        // Empty the loose cell's bounding box.
        LGridLooseCell* lcell = &grid->loose.cells[c];
        lcell->rect[0] = FLT_MAX;
        lcell->rect[1] = FLT_MAX;
        lcell->rect[2] = -FLT_MAX;
        lcell->rect[3] = -FLT_MAX;

        // Expand the bounding box by each element's extents in
        // the loose cell.
        int elt_idx = lcell->head;
        while (elt_idx != -1)
        {
            const LGridElt* elt = &grid->elts[elt_idx];
            lcell->rect[0] = min_flt(lcell->rect[0], elt->mx - elt->hx);
            lcell->rect[1] = min_flt(lcell->rect[1], elt->my - elt->hy);
            lcell->rect[2] = max_flt(lcell->rect[2], elt->mx + elt->hx);
            lcell->rect[3] = max_flt(lcell->rect[3], elt->my + elt->hy);
            elt_idx = elt->next;
        }
    }

    for (int c=0; c < grid->loose.num_cells; ++c)
    {
        // Insert the loose cell to all the tight cells in which
        // it now belongs.
        LGridLooseCell* lcell = &grid->loose.cells[c];
        const struct rect trect = to_tcell_idx4(grid, lcell->rect[0], lcell->rect[1], lcell->rect[2], lcell->rect[3]);
        for (int ty = trect.data[1]; ty <= trect.data[3]; ++ty)
        {
            int* tight_row = grid->tight.heads + ty*grid->tight.num_cols;
            for (int tx = trect.data[0]; tx <= trect.data[2]; ++tx)
            {
                const LGridTightCell new_tcell = {tight_row[tx], c};
                tight_row[tx] = grid->tight.cells.insert(new_tcell);
            }
        }
    }
}
