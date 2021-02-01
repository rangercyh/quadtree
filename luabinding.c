#if __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"
#include "Quadtree.h"

#ifndef LUA_LIB_API
#define LUA_LIB_API extern
#endif

#define MT_NAME ("_quadtree_metatable")

static inline int
getfield(lua_State *L, const char *f) {
    if (lua_getfield(L, -1, f) != LUA_TNUMBER) {
        return luaL_error(L, "invalid type %s", f);
    }
    int v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

static inline int check_in_map(int x, int y, int w, int h) {
    return x >= 0 && y >= 0 && x <= w && y <= h;
}

static int
insert(lua_State *L) {
    struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    int id = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int x2 = luaL_checkinteger(L, 5);
    int y2 = luaL_checkinteger(L, 6);
    if (!check_in_map(x1, y1, qt->root_sx << 1, qt->root_sy << 1)) {
        luaL_error(L, "Position (%d,%d) is out of map", x1, y1);
    }
    if (!check_in_map(x2, y2, qt->root_sx << 1, qt->root_sy << 1)) {
        luaL_error(L, "Position (%d,%d) is out of map", x1, y1);
    }
    lua_pushinteger(L, qt_insert(qt, id, x1, y1, x2, y2));
    return 1;
}

static int
gc(lua_State *L) {
    struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    qt_destroy(qt);
    free(qt)
    return 0;
}

static int
lmetatable(lua_State *L) {
    if (luaL_newmetatable(L, MT_NAME)) {
        luaL_Reg l[] = {
            { "insert", insert },
            { "remove", remove },
            { "cleanup", cleanup },
            { "query", query },
            { NULL, NULL }
        };
        luaL_newlib(L, l);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, gc);
        lua_setfield(L, -2, "__gc");
    }
    return 1;
}

static int
lnew(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    int width = getfield(L, "w");
    int height = getfield(L, "h");
    int max_num = getfield(L, "max_num");
    int max_depth = getfield(L, "max_depth");
    lua_assert(width > 0 && height > 0 && max_num > 0 && max_depth > 0);

    struct Quadtree *qt = lua_newuserdatauv(L, sizeof(struct Quadtree), 0);
    qt_create(qt, w, h, max_num, max_depth);
    lua_pop(L, 1);
    lmetatable(L);
    lua_setmetatable(L, -2);
    return 1;
}

LUA_LIB_API int
luaopen_quadtree(lua_State* L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "new", lnew },
        { NULL, NULL },
    };
    luaL_newlib(L, l);
    return 1;
}

#if __cplusplus
}
#endif
