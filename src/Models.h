#include <Arduino.h>
#ifndef MODELS_H
#define MODELS_H

struct LedData
{
    int r;
    int g;
    int b;
    int cw;
    int cc;
};

enum class Color
{
    R = 0,
    B = 1,
    G = 2,
    CW = 3,
    CC = 4
};

String getColorString(Color color);

#endif