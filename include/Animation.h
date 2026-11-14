#pragma once
#include <algorithm>
#include <cmath>

struct Animation
{
    bool playing = false;
    float speed = 1;
    double time = 0;
    void advance(float seconds)
    {
        if (playing)
            time = std::fmod(time + std::clamp(seconds, 0.0f, 0.1f) * speed, 100000.0);
    }
};
