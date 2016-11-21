#ifndef _JINTS_H_
#define _JINTS_H_

typedef struct Int3
{
    int x, y, z;
    Int3(const int xVal, const int yVal, const int zVal) : x(xVal), y(xVal), z(zVal) {};
} Int3;

typedef struct Int2
{
    int x, y;
    Int2(const int xVal, const int yVal) : x(xVal), y(yVal) {};
    Int2() : x(0), y(0) {};
} Int2;

#endif /* include guard */