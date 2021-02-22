#include "UGrid.hpp"

#if __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifndef LUA_LIB_API
#define LUA_LIB_API extern
#endif

#define MT_NAME ("_ugrid_metatable")

static inline float
getfield(lua_State *L, const char *f) {
    if (lua_getfield(L, -1, f) != LUA_TNUMBER) {
        return luaL_error(L, "invalid type %s", f);
    }
    float v = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return v;
}

static int
gd_insert(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    int id = luaL_checkinteger(L, 2);
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    try {
        ugrid_insert(*ugrid, id, x, y);
    } catch(...) {
        return luaL_error(L, "gd_insert exception error");
    }
    return 0;
}

static int
gd_remove(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    int id = luaL_checkinteger(L, 2);
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    try {
        ugrid_remove(*ugrid, id, x, y);
    } catch(...) {
        return luaL_error(L, "gd_remove exception error");
    }
    return 0;
}

static int
gd_move(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    int id = luaL_checkinteger(L, 2);
    float px = luaL_checknumber(L, 3);
    float py = luaL_checknumber(L, 4);
    float nx = luaL_checknumber(L, 5);
    float ny = luaL_checknumber(L, 6);
    try {
        ugrid_move(*ugrid, id, px, py, nx, ny);
    } catch(...) {
        return luaL_error(L, "gd_move exception error");
    }
    return 0;
}

static int
gd_query(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    int idx = luaL_checkinteger(L, 2);
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    float r = luaL_checknumber(L, 5);
    SmallList<int> p;
    try {
        p = ugrid_query(*ugrid, x, y, r, idx);
    } catch(...) {
        return luaL_error(L, "gd_query exception error");
    }
    lua_newtable(L);
    for (int i = 0; i < p.size(); i++) {
        lua_pushinteger(L, p.pop_back());
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int
gd_in_bounds(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    bool ret = false;
    try {
        ret = ugrid_in_bounds(*ugrid, x, y);
    } catch(...) {
        return luaL_error(L, "gd_in_bounds exception error");
    }
    lua_pushboolean(L, ret);
    return 1;
}

static int
gd_optimize(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    try {
        ugrid_optimize(*ugrid);
    } catch(...) {
        return luaL_error(L, "gd_optimize exception error");
    }
    return 0;
}

static int
gc(lua_State *L) {
    UGrid **ugrid = (UGrid **)luaL_checkudata(L, 1, MT_NAME);
    if (*ugrid) {
        try {
            ugrid_destroy(*ugrid);
            *ugrid = NULL;
        } catch(...) {
            return luaL_error(L, "gc exception error");
        }
    }
    return 0;
}

static int
lmetatable(lua_State *L) {
    if (luaL_newmetatable(L, MT_NAME)) {
        luaL_Reg l[] = {
            { "insert", gd_insert },
            { "remove", gd_remove },
            { "move", gd_move },
            { "query", gd_query },
            { "in_bounds", gd_in_bounds },
            { "optimize", gd_optimize },
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
    float radius = getfield(L, "r");
    float cell_w = getfield(L, "cw");
    float cell_h = getfield(L, "ch");
    float l = getfield(L, "l");
    float t = getfield(L, "t");
    float r = getfield(L, "r");
    float b = getfield(L, "b");
    lua_assert(radius > 0 && cell_w > 0 && cell_h > 0);
    UGrid **ugrid = (UGrid **)lua_newuserdata(L, sizeof(UGrid *));
    try {
        *ugrid = ugrid_create(radius, cell_w, cell_h, l, t, r, b);
    } catch(...) {
        return luaL_error(L, "lnew exception error");
    }
    lmetatable(L);
    lua_setmetatable(L, -2);
    return 1;
}

LUA_LIB_API int
luaopen_ugrid2(lua_State* L) {
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
