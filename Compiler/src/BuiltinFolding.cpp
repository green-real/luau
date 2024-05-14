// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "BuiltinFolding.h"

#include "Luau/Bytecode.h"

#include <math.h>

namespace Luau
{
namespace Compile
{

const double kPi = 3.14159265358979323846;
const double kRadDeg = kPi / 180.0;

static Constant cvar()
{
    return Constant();
}

static Constant cbool(bool v)
{
    Constant res = {Constant::Type_Boolean};
    res.valueBoolean = v;
    return res;
}

static Constant cnum(double v)
{
    Constant res = {Constant::Type_Number};
    res.valueNumber = v;
    return res;
}

static Constant cvector(float x, float y, float z, float w)
{
    Constant res = {Constant::Type_Vector};
    res.valueVector[0] = (float)x;
    res.valueVector[1] = (float)y;
    res.valueVector[2] = (float)z;
    res.valueVector[3] = (float)w;
    return res;
}

static Constant cstring(const char* v)
{
    Constant res = {Constant::Type_String};
    res.stringLength = unsigned(strlen(v));
    res.valueString = v;
    return res;
}

static Constant ctype(const Constant& c)
{
    LUAU_ASSERT(c.type != Constant::Type_Unknown);

    switch (c.type)
    {
    case Constant::Type_Nil:
        return cstring("nil");

    case Constant::Type_Boolean:
        return cstring("boolean");

    case Constant::Type_Number:
        return cstring("number");

    case Constant::Type_Vector:
        return cstring("vector");

    case Constant::Type_String:
        return cstring("string");

    default:
        LUAU_ASSERT(!"Unsupported constant type");
        return cvar();
    }
}

static uint32_t bit32(double v)
{
    // convert through signed 64-bit integer to match runtime behavior and gracefully truncate negative integers
    return uint32_t(int64_t(v));
}

Constant foldBuiltin(int bfid, const Constant* args, size_t count)
{
    switch (bfid)
    {
    case LBF_MATH_ABS:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(fabs(args[0].valueNumber));
        break;

    case LBF_MATH_ACOS:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(acos(args[0].valueNumber));
        break;

    case LBF_MATH_ASIN:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(asin(args[0].valueNumber));
        break;

    case LBF_MATH_ATAN2:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
            return cnum(atan2(args[0].valueNumber, args[1].valueNumber));
        break;

    case LBF_MATH_ATAN:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(atan(args[0].valueNumber));
        break;

    case LBF_MATH_CEIL:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(ceil(args[0].valueNumber));
        break;

    case LBF_MATH_COSH:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(cosh(args[0].valueNumber));
        break;

    case LBF_MATH_COS:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(cos(args[0].valueNumber));
        break;

    case LBF_MATH_DEG:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(args[0].valueNumber / kRadDeg);
        break;

    case LBF_MATH_EXP:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(exp(args[0].valueNumber));
        break;

    case LBF_MATH_FLOOR:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(floor(args[0].valueNumber));
        break;

    case LBF_MATH_FMOD:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
            return cnum(fmod(args[0].valueNumber, args[1].valueNumber));
        break;

        // Note: FREXP isn't folded since it returns multiple values

    case LBF_MATH_LDEXP:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
            return cnum(ldexp(args[0].valueNumber, int(args[1].valueNumber)));
        break;

    case LBF_MATH_LOG10:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(log10(args[0].valueNumber));
        break;

    case LBF_MATH_LOG:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(log(args[0].valueNumber));
        else if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
        {
            if (args[1].valueNumber == 2.0)
                return cnum(log2(args[0].valueNumber));
            else if (args[1].valueNumber == 10.0)
                return cnum(log10(args[0].valueNumber));
            else
                return cnum(log(args[0].valueNumber) / log(args[1].valueNumber));
        }
        break;

    case LBF_MATH_MAX:
        if (count >= 1 && args[0].type == Constant::Type_Number)
        {
            double r = args[0].valueNumber;

            for (size_t i = 1; i < count; ++i)
            {
                if (args[i].type != Constant::Type_Number)
                    return cvar();

                double a = args[i].valueNumber;

                r = (a > r) ? a : r;
            }

            return cnum(r);
        }
        break;

    case LBF_MATH_MIN:
        if (count >= 1 && args[0].type == Constant::Type_Number)
        {
            double r = args[0].valueNumber;

            for (size_t i = 1; i < count; ++i)
            {
                if (args[i].type != Constant::Type_Number)
                    return cvar();

                double a = args[i].valueNumber;

                r = (a < r) ? a : r;
            }

            return cnum(r);
        }
        break;

        // Note: MODF isn't folded since it returns multiple values

    case LBF_MATH_POW:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
            return cnum(pow(args[0].valueNumber, args[1].valueNumber));
        break;

    case LBF_MATH_RAD:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(args[0].valueNumber * kRadDeg);
        break;

    case LBF_MATH_SINH:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(sinh(args[0].valueNumber));
        break;

    case LBF_MATH_SIN:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(sin(args[0].valueNumber));
        break;

    case LBF_MATH_SQRT:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(sqrt(args[0].valueNumber));
        break;

    case LBF_MATH_TANH:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(tanh(args[0].valueNumber));
        break;

    case LBF_MATH_TAN:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(tan(args[0].valueNumber));
        break;

    case LBF_BIT32_ARSHIFT:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
        {
            uint32_t u = bit32(args[0].valueNumber);
            int s = int(args[1].valueNumber);

            if (unsigned(s) < 32)
                return cnum(double(uint32_t(int32_t(u) >> s)));
        }
        break;

    case LBF_BIT32_BAND:
        if (count >= 1 && args[0].type == Constant::Type_Number)
        {
            uint32_t r = bit32(args[0].valueNumber);

            for (size_t i = 1; i < count; ++i)
            {
                if (args[i].type != Constant::Type_Number)
                    return cvar();

                r &= bit32(args[i].valueNumber);
            }

            return cnum(double(r));
        }
        break;

    case LBF_BIT32_BNOT:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(double(uint32_t(~bit32(args[0].valueNumber))));
        break;

    case LBF_BIT32_BOR:
        if (count >= 1 && args[0].type == Constant::Type_Number)
        {
            uint32_t r = bit32(args[0].valueNumber);

            for (size_t i = 1; i < count; ++i)
            {
                if (args[i].type != Constant::Type_Number)
                    return cvar();

                r |= bit32(args[i].valueNumber);
            }

            return cnum(double(r));
        }
        break;

    case LBF_BIT32_BXOR:
        if (count >= 1 && args[0].type == Constant::Type_Number)
        {
            uint32_t r = bit32(args[0].valueNumber);

            for (size_t i = 1; i < count; ++i)
            {
                if (args[i].type != Constant::Type_Number)
                    return cvar();

                r ^= bit32(args[i].valueNumber);
            }

            return cnum(double(r));
        }
        break;

    case LBF_BIT32_BTEST:
        if (count >= 1 && args[0].type == Constant::Type_Number)
        {
            uint32_t r = bit32(args[0].valueNumber);

            for (size_t i = 1; i < count; ++i)
            {
                if (args[i].type != Constant::Type_Number)
                    return cvar();

                r &= bit32(args[i].valueNumber);
            }

            return cbool(r != 0);
        }
        break;

    case LBF_BIT32_EXTRACT:
        if (count >= 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number &&
            (count == 2 || args[2].type == Constant::Type_Number))
        {
            uint32_t u = bit32(args[0].valueNumber);
            int f = int(args[1].valueNumber);
            int w = count == 2 ? 1 : int(args[2].valueNumber);

            if (f >= 0 && w > 0 && f + w <= 32)
            {
                uint32_t m = ~(0xfffffffeu << (w - 1));

                return cnum(double((u >> f) & m));
            }
        }
        break;

    case LBF_BIT32_LROTATE:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
        {
            uint32_t u = bit32(args[0].valueNumber);
            int s = int(args[1].valueNumber);

            return cnum(double((u << (s & 31)) | (u >> ((32 - s) & 31))));
        }
        break;

    case LBF_BIT32_LSHIFT:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
        {
            uint32_t u = bit32(args[0].valueNumber);
            int s = int(args[1].valueNumber);

            if (unsigned(s) < 32)
                return cnum(double(u << s));
        }
        break;

    case LBF_BIT32_REPLACE:
        if (count >= 3 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number && args[2].type == Constant::Type_Number &&
            (count == 3 || args[3].type == Constant::Type_Number))
        {
            uint32_t n = bit32(args[0].valueNumber);
            uint32_t v = bit32(args[1].valueNumber);
            int f = int(args[2].valueNumber);
            int w = count == 3 ? 1 : int(args[3].valueNumber);

            if (f >= 0 && w > 0 && f + w <= 32)
            {
                uint32_t m = ~(0xfffffffeu << (w - 1));

                return cnum(double((n & ~(m << f)) | ((v & m) << f)));
            }
        }
        break;

    case LBF_BIT32_RROTATE:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
        {
            uint32_t u = bit32(args[0].valueNumber);
            int s = int(args[1].valueNumber);

            return cnum(double((u >> (s & 31)) | (u << ((32 - s) & 31))));
        }
        break;

    case LBF_BIT32_RSHIFT:
        if (count == 2 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number)
        {
            uint32_t u = bit32(args[0].valueNumber);
            int s = int(args[1].valueNumber);

            if (unsigned(s) < 32)
                return cnum(double(u >> s));
        }
        break;

    case LBF_TYPE:
        if (count == 1 && args[0].type != Constant::Type_Unknown)
            return ctype(args[0]);
        break;

    case LBF_STRING_BYTE:
        if (count == 1 && args[0].type == Constant::Type_String)
        {
            if (args[0].stringLength > 0)
                return cnum(double(uint8_t(args[0].valueString[0])));
        }
        else if (count == 2 && args[0].type == Constant::Type_String && args[1].type == Constant::Type_Number)
        {
            int i = int(args[1].valueNumber);

            if (i > 0 && unsigned(i) <= args[0].stringLength)
                return cnum(double(uint8_t(args[0].valueString[i - 1])));
        }
        break;

    case LBF_STRING_LEN:
        if (count == 1 && args[0].type == Constant::Type_String)
            return cnum(double(args[0].stringLength));
        break;

    case LBF_TYPEOF:
        if (count == 1 && args[0].type != Constant::Type_Unknown)
            return ctype(args[0]);
        break;

    case LBF_MATH_CLAMP:
        if (count == 3 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number && args[2].type == Constant::Type_Number)
        {
            double min = args[1].valueNumber;
            double max = args[2].valueNumber;

            if (min <= max)
            {
                double v = args[0].valueNumber;
                v = v < min ? min : v;
                v = v > max ? max : v;

                return cnum(v);
            }
        }
        break;

    case LBF_MATH_SIGN:
        if (count == 1 && args[0].type == Constant::Type_Number)
        {
            double v = args[0].valueNumber;

            return cnum(v > 0.0 ? 1.0 : v < 0.0 ? -1.0 : 0.0);
        }
        break;

    case LBF_MATH_ROUND:
        if (count == 1 && args[0].type == Constant::Type_Number)
            return cnum(round(args[0].valueNumber));
        break;

    case LBF_VECTOR:
        if (count >= 3 && args[0].type == Constant::Type_Number && args[1].type == Constant::Type_Number && args[2].type == Constant::Type_Number)
        {
            float x = float(args[0].valueNumber);
            float y = float(args[1].valueNumber);
            float z = float(args[2].valueNumber);

            if (count == 3)
                return cvector(x, y, z, 0.0);
            else if (count == 4 && args[3].type == Constant::Type_Number)
                return cvector(x, y, z, float(args[3].valueNumber));
        }
        break;

    case LBF_VECTOR_NEW:
        {
            float x = count >= 1 && args[0].type == Constant::Type_Number ? float(args[0].valueNumber) : 0.0f;
            float y = count >= 2 && args[1].type == Constant::Type_Number ? float(args[1].valueNumber) : 0.0f;
            float z = count >= 3 && args[2].type == Constant::Type_Number ? float(args[2].valueNumber) : 0.0f;
            float w = count >= 4 && args[3].type == Constant::Type_Number ? float(args[3].valueNumber) : 0.0f;

            return cvector(x, y, z, w);
        }
        break;

    case LBF_VECTOR_CROSS:
        if (count == 2 && args[0].type == Constant::Type_Vector && args[1].type == Constant::Type_Vector)
        {
            float x1 = args[0].valueVector[0];
            float y1 = args[0].valueVector[1];
            float z1 = args[0].valueVector[2];

            float x2 = args[1].valueVector[0];
            float y2 = args[1].valueVector[1];
            float z2 = args[1].valueVector[2];

            return cvector(y1 * z2 - z1 * y2, z1 * x2 - x1 * z2, x1 * y2 - y1 * x2, 0.0f);
        }
        break;

    case LBF_VECTOR_DOT:
        if (count == 2 && args[0].type == Constant::Type_Vector && args[1].type == Constant::Type_Vector)
        {
            float x1 = args[0].valueVector[0];
            float y1 = args[0].valueVector[1];
            float z1 = args[0].valueVector[2];
            float w1 = args[0].valueVector[3];

            float x2 = args[1].valueVector[0];
            float y2 = args[1].valueVector[1];
            float z2 = args[1].valueVector[2];
            float w2 = args[1].valueVector[3];
            
#if LUAU_VECTOR_SIZE == 4
            return cnum(x1 * x2 + y1 * y2 + z1 * z2 + w1 * w2);
#else
            return cnum(x1 * x2 + y1 * y2 + z1 * z2);
#endif
        }
        break;

    case LBF_VECTOR_MAGNITUDE:
        if (count == 1 && args[0].type == Constant::Type_Vector)
        {
            float x = args[0].valueVector[0];
            float y = args[0].valueVector[1];
            float z = args[0].valueVector[2];
            float w = args[0].valueVector[3];

#if LUAU_VECTOR_SIZE == 4
            return cnum(sqrtf(x * x + y * y + z * z + w * w));
#else
            return cnum(sqrtf(x * x + y * y + z * z));
#endif
        }
        break;

    case LBF_VECTOR_NORMALIZED:
        if (count == 1 && args[0].type == Constant::Type_Vector)
        {
            float x = args[0].valueVector[0];
            float y = args[0].valueVector[1];
            float z = args[0].valueVector[2];
            float w = 0.0f;

            // only assign w if the vector is 4D, since v[3] is otherwise undefined
#if LUAU_VECTOR_SIZE == 4
            w = args[0].valueVector[3];
#endif

            float mag = sqrtf(x * x + y * y + z * z + w * w);
            
            if (mag == 0.0f)
                return cvector(0.0f, 0.0f, 0.0f, 0.0f);
            else {
                float invMag = 1.0f / mag;
                return cvector(x * invMag, y * invMag, z * invMag, w * invMag);
            } 
        }
        break;
    }

    return cvar();
}

Constant foldBuiltinMath(AstName index)
{
    if (index == "pi")
        return cnum(kPi);

    if (index == "huge")
        return cnum(HUGE_VAL);

    return cvar();
}

} // namespace Compile
} // namespace Luau
