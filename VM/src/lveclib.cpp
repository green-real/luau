// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"

#include <math.h>

static int vector_new(lua_State* L)
{
    float x = float(luaL_optnumber(L, 1, 0.0f));
    float y = float(luaL_optnumber(L, 2, 0.0f));
    float z = float(luaL_optnumber(L, 3, 0.0f));

#if LUA_VECTOR_SIZE == 4
    float w = float(luaL_optnumber(L, 4, 0.0f));
    lua_pushvector(L, x, y, z, 0.0f);
#else
    lua_pushvector(L, x, y, z);
#endif

    return 1;
}

static int vector_cross(lua_State* L)
{
    const float* v1 = luaL_checkvector(L, 1);
    const float* v2 = luaL_checkvector(L, 2);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0], 0.0f);
#else
    lua_pushvector(L, v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]);
#endif

    return 1;
}

static int vector_dot(lua_State* L)
{
    const float* v1 = luaL_checkvector(L, 1);
    const float* v2 = luaL_checkvector(L, 2);

    float dot = v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];

    lua_pushnumber(L, dot);

    return 1;
}

static int vector_magnitude(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

    float mag = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    lua_pushnumber(L, mag);

    return 1;
}

static int vector_normalize(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

    float mag = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, v[0] / mag, v[1] / mag, v[2] / mag, v[3] / mag);
#else
    lua_pushvector(L, v[0] / mag, v[1] / mag, v[2] / mag);
#endif

    return 1;
}

static const luaL_Reg vectorlib[] = {
    {"new", vector_new},
    {"cross", vector_cross},
    {"dot", vector_dot},
    {"magnitude", vector_magnitude},
    {"normalize", vector_normalize},
    {NULL, NULL},
};

int luaopen_vector(lua_State* L)
{
    luaL_register(L, LUA_VECTORLIBNAME, vectorlib);

    return 1;
}
