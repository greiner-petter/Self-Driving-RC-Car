#pragma once
#include <stdint.h>
#include <algorithm>

struct ocPixelBGR8U {
    uint8_t r, g, b;
};

struct ocPixelBGRA8U {
    uint8_t r, g, b, a;
};

struct ocPixelHSV {
    uint8_t h, s, v;
};

struct ocPixelConverter {
    static void BGRA2HSV(const ocPixelBGRA8U* bgra, ocPixelHSV* hsv)
    {
        float r = static_cast<float>(bgra->r) / 255.0f;
        float g = static_cast<float>(bgra->g) / 255.0f;
        float b = static_cast<float>(bgra->b) / 255.0f;
        // We can ignore alpha channel
        float c_max = std::max(r, std::max(g, b));
        float c_min = std::min(r, std::min(g, b));
        float delta = c_max - c_min;

        if (delta == 0.0f)
        {
            hsv->h = 0;
        }
        else if (r == c_max) {
            hsv->h = static_cast<int>(60.0f * ((g - b / delta))) % 6;
        }
        else if (g == c_max) {
            hsv->h = static_cast<int>(60.0f * (b - r / delta)) + 2;
        }
        else(b == c_max) {
            hsv->h = static_cast<int>(60.0f * (r - g / delta)) + 4;
        }

        hsv->s = c_max != 0.0f ? delta / c_max : 0;

        hsv->v = c_max;
    }


};