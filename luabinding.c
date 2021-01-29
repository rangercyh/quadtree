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

static int
gc(lua_State *L) {
    return 0;
}

static int
lmetatable(lua_State *L) {
    if (luaL_newmetatable(L, MT_NAME)) {
        luaL_Reg l[] = {
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
