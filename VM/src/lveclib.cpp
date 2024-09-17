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

static int vector_create(lua_State* L)
{
    float x = float(luaL_optnumber(L, 1, 0.0f));
    float y = float(luaL_optnumber(L, 2, 0.0f));
    float z = float(luaL_optnumber(L, 3, 0.0f));
#if LUA_VECTOR_SIZE == 4
    float w = float(luaL_optnumber(L, 4, 0.0f));
#else
    float w = 0.0f;
#endif

    pushvector(L, x, y, z, w);
    return 1;
}

static int vector_abs(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    
    pushvector(L, fabsf(v[0]), fabsf(v[1]), fabsf(v[2]), fabsf(v[3]));
    return 1;
}

static int vector_angle(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);
    const float* axis = luaL_optvector(L, 3, NULL);

    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    float mag1 = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    float mag2 = sqrtf(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);

    float angle = acosf(dot / (mag1 * mag2));

    if (axis)
    {
        float crossx = a[1] * b[2] - a[2] * b[1];
        float crossy = a[2] * b[0] - a[0] * b[2];
        float crossz = a[0] * b[1] - a[1] * b[0];

        float crossDotAxis = crossx * axis[0] + crossy * axis[1] + crossz * axis[2];
        angle *= crossDotAxis < 0.0f ? -1.0f : 1.0f;
    }

    lua_pushnumber(L, double(angle));
    return 1;
}

static int vector_ceil(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    
    pushvector(L, ceilf(v[0]), ceilf(v[1]), ceilf(v[2]), ceilf(v[3]));
    return 1;
}

static int vector_clamp(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    const float* min = luaL_checkvector(L, 2);
    const float* max = luaL_checkvector(L, 3);

    float x = v[0] < min[0] ? min[0] : (v[0] > max[0] ? max[0] : v[0]);
    float y = v[1] < min[1] ? min[1] : (v[1] > max[1] ? max[1] : v[1]);
    float z = v[2] < min[2] ? min[2] : (v[2] > max[2] ? max[2] : v[2]);
#if LUA_VECTOR_SIZE == 4
    float w = v[3] < min[3] ? min[3] : (v[3] > max[3] ? max[3] : v[3]);
#else
    float w = 0.0f;
#endif

    pushvector(L, x, y, z, w);
    return 1;
}

static int vector_cross(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

    pushvector(L, a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0], 0.0f);
    return 1;
}

static int vector_dot(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

#if LUA_VECTOR_SIZE == 4
    dot += a[3] * b[3];
#endif

    lua_pushnumber(L, double(dot));
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
    float squareSum = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

#if LUA_VECTOR_SIZE == 4
    squareSum += v[3] * v[3];
#endif

    lua_pushnumber(L, double(sqrtf(squareSum)));
    return 1;
}

static int vector_max(lua_State* L)
{
    int n = lua_gettop(L); // number of arguments
    const float* v1 = luaL_checkvector(L, 1);
    float xmax = v1[0];
    float ymax = v1[1];
    float zmax = v1[2];
    float wmax = v1[3];

    int i;
    for (int i = 2; i <= n; i++)
    {
        const float* v = luaL_checkvector(L, i);
        if (v[0] > xmax)
            xmax = v[0];
        if (v[1] > ymax)
            ymax = v[1];
        if (v[2] > zmax)
            zmax = v[2];
#if LUA_VECTOR_SIZE == 4
        if (v[3] > wmax)
            wmax = v[3];
#endif
    }

    pushvector(L, xmax, ymax, zmax, wmax);
    return 1;
}

static int vector_min(lua_State* L)
{
    int n = lua_gettop(L); // number of arguments
    const float* v1 = luaL_checkvector(L, 1);
    float xmin = v1[0];
    float ymin = v1[1];
    float zmin = v1[2];
    float wmin = v1[3];

    int i;
    for (int i = 2; i <= n; i++)
    {
        const float* v = luaL_checkvector(L, i);
        if (v[0] < xmin)
            xmin = v[0];
        if (v[1] < ymin)
            ymin = v[1];
        if (v[2] < zmin)
            zmin = v[2];
#if LUA_VECTOR_SIZE == 4
        if (v[3] < wmin)
            wmin = v[3];
#endif
    }

    pushvector(L, xmin, ymin, zmin, wmin);
    return 1;
}

static int vector_normalize(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

    float squareSum = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
#if LUA_VECTOR_SIZE == 4
    squareSum += v[3] * v[3];
#endif

    float invMag = 1.0f / sqrtf(squareSum);

    pushvector(L, v[0] * invMag, v[1] * invMag, v[2] * invMag, v[3] * invMag);
    return 1;
}

static const luaL_Reg veclib[] = {
    {"create", vector_create},
    {"abs", vector_abs},
    {"angle", vector_angle},
    {"ceil", vector_ceil},
    {"clamp", vector_clamp},
    {"cross", vector_cross},
    {"dot", vector_dot},
    {"floor", vector_floor},
    {"magnitude", vector_magnitude},
    {"max", vector_max},
    {"min", vector_min},
    {"normalize", vector_normalize},
    {NULL, NULL},
};

static void createmetatable(lua_State* L)
{
    lua_createtable(L, 0, 1);       // create metatable for vectors
    pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);   // dummy vector
    lua_pushvalue(L, -2);
    lua_setmetatable(L, -2);        // set vector metatable
    lua_pop(L, 1);                  // pop dummy vector
    lua_pushvalue(L, -2);           // vector library...
    lua_setfield(L, -2, "__index"); // ...is the __index metamethod
    lua_pop(L, 1);                  // pop metatable
}

/*
** Open vector library
*/
int luaopen_vector(lua_State* L)
{
    luaL_register(L, LUA_VECLIBNAME, veclib);
    createmetatable(L);	
    return 1;
}
