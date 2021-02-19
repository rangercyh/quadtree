// ************************************************************************************
// LGrid.cpp
// ************************************************************************************
#include "LGrid.hpp"
#include <cstdlib>
#include <cfloat>
#include <utility>

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

static SimdVec4i to_tcell_idx4(const LGrid* grid, __m128 rect)
{
    __m128 inv_cell_size_vec = simd_create4f(grid->tight.inv_cell_w, grid->tight.inv_cell_h,
                                             grid->tight.inv_cell_w, grid->tight.inv_cell_h);
    __m128 cell_xyf_vec = simd_mul4f(rect, inv_cell_size_vec);
    __m128i clamp_vec = simd_create4i(grid->tight.num_cols-1, grid->tight.num_rows-1,
                                      grid->tight.num_cols-1, grid->tight.num_rows-1);
    __m128i cell_xy_vec = simd_clamp4i(simd_ftoi4f(cell_xyf_vec), simd_zero4i(), clamp_vec);
    return simd_store4i(cell_xy_vec);
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
    const SimdVec4f prev_rect = {lcell->rect[0], lcell->rect[1], lcell->rect[2], lcell->rect[3]};
    lcell->rect[0] = min_flt(lcell->rect[0], mx - hx);
    lcell->rect[1] = min_flt(lcell->rect[1], my - hy);
    lcell->rect[2] = max_flt(lcell->rect[2], mx + hx);
    lcell->rect[3] = max_flt(lcell->rect[3], my + hy);

    // Determine the cells occupied by the loose cell in the tight grid.
    const SimdVec4f elt_rect = {mx-hx, my-hy, mx+hx, my+hy};
    const SimdVec4i trect = to_tcell_idx4(grid, simd_load4f(&elt_rect));

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
        const SimdVec4i prev_trect = to_tcell_idx4(grid, simd_load4f(&prev_rect));
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

static __m128 element_rect(const LGridElt* elt)
{
    return simd_create4f(elt->mx-elt->hx, elt->my-elt->hy,
                         elt->mx+elt->hx, elt->my+elt->hy);
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
    const SimdVec4f qrect = {mx-hx, my-hy, mx+hx, my+hy};
    __m128 qrect_vec = simd_load4f(&qrect);
    const SimdVec4i trect = to_tcell_idx4(grid, qrect_vec);

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
                if (lcell_idxs.find_index(tcell->lcell) == -1 && simd_rect_intersect4f(qrect_vec, simd_loadu4f(lcell->rect)))
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
            if (elt->id != omit_id && simd_rect_intersect4f(qrect_vec, element_rect(elt)))
                res.push_back(elt->id);
            elt_idx = elt->next;
        }
    }
    return res;
}

LGridQuery4 lgrid_query4(const LGrid* grid, const SimdVec4f* mx4, const SimdVec4f* my4, 
                         const SimdVec4f* hx4, const SimdVec4f* hy4, const SimdVec4i* omit_id4)
{
    __m128 hx_vec = simd_load4f(hx4), hy_vec = simd_load4f(hy4);
    __m128 mx_vec = simd_sub4f(simd_load4f(mx4), simd_scalar4f(grid->x));
    __m128 my_vec = simd_sub4f(simd_load4f(my4), simd_scalar4f(grid->y));
    __m128 ql_vec = simd_sub4f(mx_vec, hx_vec), qt_vec = simd_sub4f(my_vec, hy_vec);
    __m128 qr_vec = simd_add4f(mx_vec, hx_vec), qb_vec = simd_add4f(my_vec, hy_vec);

    __m128 inv_cell_w_vec = simd_scalar4f(grid->tight.inv_cell_w), inv_cell_h_vec = simd_scalar4f(grid->tight.inv_cell_h);
    __m128i max_x_vec = simd_scalar4i(grid->tight.num_cols-1), max_y_vec = simd_scalar4i(grid->tight.num_rows-1);
    __m128i tmin_x_vec = simd_clamp4i(simd_ftoi4f(simd_mul4f(ql_vec, inv_cell_w_vec)), simd_zero4i(), max_x_vec);
    __m128i tmin_y_vec = simd_clamp4i(simd_ftoi4f(simd_mul4f(qt_vec, inv_cell_h_vec)), simd_zero4i(), max_y_vec);
    __m128i tmax_x_vec = simd_clamp4i(simd_ftoi4f(simd_mul4f(qr_vec, inv_cell_w_vec)), simd_zero4i(), max_x_vec);
    __m128i tmax_y_vec = simd_clamp4i(simd_ftoi4f(simd_mul4f(qb_vec, inv_cell_h_vec)), simd_zero4i(), max_y_vec);

    const SimdVec4i tmin_x4 = simd_store4i(tmin_x_vec), tmin_y4 = simd_store4i(tmin_y_vec);
    const SimdVec4i tmax_x4 = simd_store4i(tmax_x_vec), tmax_y4 = simd_store4i(tmax_y_vec);
    const SimdVec4f ql4 = simd_store4f(ql_vec), qt4 = simd_store4f(qt_vec);
    const SimdVec4f qr4 = simd_store4f(qr_vec), qb4 = simd_store4f(qb_vec);

    LGridQuery4 res4;
    for (int k=0; k < 4; ++k)
    {
        const int trect[4] = {tmin_x4.data[k], tmin_y4.data[k], tmax_x4.data[k], tmax_y4.data[k]};
        const int omit_id = omit_id4->data[k];

        // Gather the intersecting loose cells in the tight cells that intersect.
        SmallList<int> lcell_idxs;
        __m128 qrect_vec = simd_create4f(ql4.data[k], qt4.data[k], qr4.data[k], qb4.data[k]);
        for (int ty = trect[1]; ty <= trect[3]; ++ty)
        {
            const int* tight_row = grid->tight.heads + ty*grid->tight.num_cols;
            for (int tx = trect[0]; tx <= trect[2]; ++tx)
            {
                // Iterate through the loose cells that intersect the tight cells.
                int tcell_idx = tight_row[tx];
                while (tcell_idx != -1)
                {
                    const LGridTightCell* tcell = &grid->tight.cells[tcell_idx];
                    if (lcell_idxs.find_index(tcell->lcell) && simd_rect_intersect4f(qrect_vec, simd_loadu4f(grid->loose.cells[tcell->lcell].rect)))
                        lcell_idxs.push_back(tcell->lcell);
                    tcell_idx = tcell->next;
                }
            }
        }

        // For each loose cell, determine what elements intersect.
        for (int j=0; j < lcell_idxs.size(); ++j)
        {
            const LGridLooseCell* lcell = &grid->loose.cells[lcell_idxs[j]];
            int elt_idx = lcell->head;
            while (elt_idx != -1)
            {
                // If the element intersects the search rectangle, add it to the
                // resulting elements unless it has an ID that should be omitted.
                const LGridElt* elt = &grid->elts[elt_idx];
                if (elt->id != omit_id && simd_rect_intersect4f(qrect_vec, element_rect(elt)))
                    res4.elements[k].push_back(elt->id);
                elt_idx = elt->next;
            }
        }
    }
    return res4;
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
        const SimdVec4i trect = to_tcell_idx4(grid, simd_loadu4f(lcell->rect));
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
