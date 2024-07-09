#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265358979323846

class Interval
{
public:

    float min, max;

    Interval(float min, float max)
        : min(min), max(max)
    {}

    float clamp(float value)
    {
        if (value < min) value = min;
        else if (value > max) value = max;
        return value;
    }

};

#endif