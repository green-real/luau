// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"

#include <math.h>

static LUAU_FORCEINLINE void pushvector(lua_State* L, float x, float y, float z, float w)
{
#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, x, y, z, w);
#else
    lua_pushvector(L, x, y, z);
#endif
}

static int vector_constructor(lua_State* L)
{
    float x = float(luaL_optnumber(L, 2, 0.0f));
    float y = float(luaL_optnumber(L, 3, 0.0f));
    float z = float(luaL_optnumber(L, 4, 0.0f));
    
#if LUA_VECTOR_SIZE == 4
    float w = float(luaL_optnumber(L, 5, 0.0f));
    lua_pushvector(L, x, y, z, w);
#else
    lua_pushvector(L, x, y, z);
#endif

    return 1;
}

static int vector_angle(lua_State* L)
{
    const float* v1 = luaL_checkvector(L, 1);
    const float* v2 = luaL_checkvector(L, 2);

    float dot = v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
    float mag1 = sqrtf(v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2]);
    float mag2 = sqrtf(v2[0] * v2[0] + v2[1] * v2[1] + v2[2] * v2[2]);

    float angle = acosf(dot / (mag1 * mag2));

    lua_pushnumber(L, angle);

    return 1;
}

static int vector_ceil(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    
    pushvector(L, ceilf(v[0]), ceilf(v[1]), ceilf(v[2]), ceilf(v[3]));

    return 1;
}

static int vector_cross(lua_State* L)
{
    const float* v1 = luaL_checkvector(L, 1);
    const float* v2 = luaL_checkvector(L, 2);

    pushvector(L, v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0], 0.0f);

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

static int vector_floor(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    
    pushvector(L, floorf(v[0]), floorf(v[1]), floorf(v[2]), floorf(v[3]));
    
    return 1;
}

static int vector_magnitude(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    
    float x = v[0], y = v[1], z = v[2], w = 0.0f;

    // only assign w if the vector is 4D, since v[3] is otherwise undefined
#if LUA_VECTOR_SIZE == 4
    w = v[3];
#endif

    float mag = sqrtf(x * x + y * y + z * z + w * w);

    lua_pushnumber(L, mag);

    return 1;
}

static int vector_max(lua_State* L)
{
    const float* v1 = luaL_checkvector(L, 1);
    const float* v2 = luaL_checkvector(L, 2);
    
    pushvector(
        L,
        v1[0] > v2[0] ? v1[0] : v2[0],
        v1[1] > v2[1] ? v1[1] : v2[1],
        v1[2] > v2[2] ? v1[2] : v2[2],
        v1[3] > v2[3] ? v1[3] : v2[3]
    );

    return 1;
}

static int vector_min(lua_State* L)
{
    const float* v1 = luaL_checkvector(L, 1);
    const float* v2 = luaL_checkvector(L, 2);
    
    pushvector(
        L,
        v1[0] < v2[0] ? v1[0] : v2[0],
        v1[1] < v2[1] ? v1[1] : v2[1],
        v1[2] < v2[2] ? v1[2] : v2[2],
        v1[3] < v2[3] ? v1[3] : v2[3]
    );

    return 1;
}

static int vector_normalized(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

    float x = v[0], y = v[1], z = v[2], w = 0.0f;

    // only assign w if the vector is 4D, since v[3] is otherwise undefined
#if LUA_VECTOR_SIZE == 4
    w = v[3];
#endif

    float mag = sqrtf(x * x + y * y + z * z + w * w);
    
    if (mag == 0.0f)
    {
        pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {   
        float invMag = 1.0f / mag;
        pushvector(L, x * invMag, y * invMag, z * invMag, w * invMag);
    }

    return 1;
}

static const luaL_Reg vectorlib[] = {
    {"angle", vector_angle},
    {"ceil", vector_ceil},
    {"cross", vector_cross},
    {"dot", vector_dot},
    {"floor", vector_floor},
    {"magnitude", vector_magnitude},
    {"max", vector_max},
    {"min", vector_min},
    {"normalized", vector_normalized},
    {NULL, NULL},
};

static void createmetatables(lua_State* L)
{   
    // metatable for vector objects
    lua_createtable(L, 0, 1);
    pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f); // dummy vector
    lua_pushvalue(L, -2);           // push vector metatable
    lua_setmetatable(L, -2);
    lua_pop(L, 1);                  // pop dummy vector

    lua_pushvalue(L, -2);           // push vector library
    lua_setfield(L, -2, "__index"); // vector metatable.__index = vectorlib
    lua_pop(L, 1);                  // pop vector metatable

    // metatable for vector library
    lua_createtable(L, 0, 1);
    lua_pushvalue(L, -1);           // push vectorlib metatable
    lua_setmetatable(L, -3);
    lua_pushcfunction(L, vector_constructor, "vector");
    lua_setfield(L, -2, "__call");  // vectorlib metatable.__call = vector_constructor
    lua_pop(L, 1);                  // pop vectorlib metatable
}

int luaopen_vector(lua_State* L)
{
    luaL_register(L, LUA_VECTORLIBNAME, vectorlib);
    createmetatables(L);

    pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "zero");
    pushvector(L, 1.0f, 1.0f, 1.0f, 1.0f);
    lua_setfield(L, -2, "one");

    return 1;
}
