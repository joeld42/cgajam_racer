#include "raylib.h"
#include "raymath.h"

#include <stdlib.h>

inline Vector2 Vector2Make( float x, float y )
{
    Vector2 result;
    result.x = x;
    result.y = y;
    return result;
}

inline Vector3 Vector3Make( float x, float y, float z )
{
    Vector3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

inline Vector3 Vector3MultScalar( Vector3 v, float x )
{
    return Vector3Make( v.x * x, v.y*x, v.z*x );
}

inline Vector2 Vector2MultScalar( Vector2 v, float x )
{
    return Vector2Make( v.x * x, v.y*x );
}

inline Vector3 Vector3Hadamard( Vector3 a, Vector3 b )
{
    return (Vector3){ a.x*b.x, a.y * b.y, a.z*b.z };
}

inline float RandUniform() {
    return (float)rand() / (float)RAND_MAX;
}

inline float RandUniformRange( float rangeMin, float rangeMax ) {
    return rangeMin + RandUniform()*(rangeMax - rangeMin);
}

inline float fsgn( float v ) {
    if (v < 0.0) return -1.0f;
    else return 1.0f;
}