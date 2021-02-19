#if __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "Quadtree.h"

#ifndef LUA_LIB_API
#define LUA_LIB_API extern
#endif

static IntList out_put;

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
quad_insert(lua_State *L) {
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
        luaL_error(L, "Position (%d,%d) is out of map", x2, y2);
    }
    lua_pushinteger(L, qt_insert(qt, id, x1, y1, x2, y2));
    return 1;
}

static int
quad_remove(lua_State *L) {
    struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    int index = luaL_checkinteger(L, 2);
    qt_remove(qt, index);
    return 0;
}

static int
quad_cleanup(lua_State *L) {
    struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    qt_cleanup(qt);
    return 0;
}

static int
quad_query(lua_State *L) {
    struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    int x1 = luaL_checkinteger(L, 2);
    int y1 = luaL_checkinteger(L, 3);
    int x2 = luaL_checkinteger(L, 4);
    int y2 = luaL_checkinteger(L, 5);
    if (!check_in_map(x1, y1, qt->root_sx << 1, qt->root_sy << 1)) {
        luaL_error(L, "Position (%d,%d) is out of map", x1, y1);
    }
    if (!check_in_map(x2, y2, qt->root_sx << 1, qt->root_sy << 1)) {
        luaL_error(L, "Position (%d,%d) is out of map", x2, y2);
    }
    qt_query(qt, &out_put, x1, y1, x2, y2, -1);
    lua_newtable(L);
    for (int i = 0; i < il_size(&out_put); i++) {
        lua_pushinteger(L, il_get(&out_put, i, 0));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int
quad_traverse(lua_State *L) {
    // struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    // struct IntList *out_put = malloc(sizeof(struct IntList));
    // il_create(out_put, 1);
    // qt_traverse(qt, out_put, check_collision, check_collision)
    // lua_newtable(L);
    // for (int i = 0; i < il_size(out_put); i++) {
    //     lua_pushinteger(L, il_get(out_put, i, 0));
    //     lua_rawseti(L, -2, i + 1);
    // }
    // il_destroy(out_put);
    // free(out_put);
    return 1;
}

static int
gc(lua_State *L) {
    struct Quadtree *qt = luaL_checkudata(L, 1, MT_NAME);
    qt_destroy(qt);
    return 0;
}

static int
lmetatable(lua_State *L) {
    if (luaL_newmetatable(L, MT_NAME)) {
        luaL_Reg l[] = {
            { "insert", quad_insert },
            { "remove", quad_remove },
            { "cleanup", quad_cleanup },
            { "query", quad_query },
            { "traverse", quad_traverse },
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
    struct Quadtree *qt = lua_newuserdata(L, sizeof(struct Quadtree));
    qt_create(qt, width, height, max_num, max_depth);
    lmetatable(L);
    lua_setmetatable(L, -2);
    return 1;
}

LUA_LIB_API int
luaopen_quadtree(lua_State* L) {
    il_create(&out_put, 1);
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
