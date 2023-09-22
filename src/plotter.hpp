#pragma once

#include <algorithm>
#include <cmath>
#include <vector>
#include "raylib.h"

const int SCREEN_WIDTH  = 800;
const int SCREEN_HEIGHT = 800;
const int SAMPLE_RATE = 44100;
const int FRAME_RATE = 60;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / FRAME_RATE;

const double MIN_VOLTAGE = -7.0;
const double MAX_VOLTAGE = +7.0;

struct PlotPoint
{
    int x;
    int y;

    PlotPoint()
        : x(-1)
        , y(-1)
        {}

    PlotPoint(int _x, int _y)
        : x(_x)
        , y(_y)
        {}
};

class Plotter
{
private:
    const std::size_t trailLength;
    std::size_t trailIndex = 0;
    std::vector<PlotPoint> trail;

public:
    explicit Plotter(int _trailLength)
        : trailLength(std::max(2, _trailLength))
        {}

    void plot(double vx, double vy)
    {
        // Map voltage ranges -12V..+12V to screen dimensions.
        int sx = static_cast<int>(std::round(((vx - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * SCREEN_WIDTH));
        int sy = static_cast<int>(std::round(((MAX_VOLTAGE - vy) / (MAX_VOLTAGE - MIN_VOLTAGE)) * SCREEN_HEIGHT));

        // On the first render, prefill the trail buffer.
        while (trail.size() < trailLength)
            trail.push_back(PlotPoint(sx, sy));

        trail.at(trailIndex) = PlotPoint(sx, sy);
        trailIndex = (trailIndex + 1) % trailLength;

        Color color = BLACK;
        Color target = GREEN;

        const std::size_t fadeLength = std::max({target.r, target.g, target.b, static_cast<unsigned char>(1)});
        const std::size_t fadeInterval = std::max(std::size_t{1}, trailLength / (2 * fadeLength));
        std::size_t fadeCount = fadeInterval;

        std::size_t i = trailIndex;
        while (true)
        {
            std::size_t j = (i + 1) % trailLength;
            if (j == trailIndex)
                break;

            const PlotPoint &a = trail.at(i);
            const PlotPoint &b = trail.at(j);
            DrawLine(a.x, a.y, b.x, b.y, color);
            i = j;

            if (--fadeCount == 0)
            {
                fadeCount = fadeInterval;
                if (color.r < target.r) ++color.r;
                if (color.g < target.g) ++color.g;
                if (color.b < target.b) ++color.b;
            }
        }

        DrawCircle(sx, sy, 2.0f, WHITE);
    }
};
