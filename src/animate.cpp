/*
    animate.cpp  -  Don Cross <cosinekitty@gmail.com>  -  2023-09-10

    Runs the Sloth Torpor simulation and renders an X/Y plot.

    https://github.com/cosinekitty/sloth
*/

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <vector>
#include "raylib.h"
#include "torpor_sloth_circuit.hpp"

const int SCREEN_WIDTH  = 800;
const int SCREEN_HEIGHT = 800;
const int SAMPLE_RATE = 44100;
const int FRAME_RATE = 60;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / FRAME_RATE;

const double MIN_VOLTAGE = -9.0;
const double MAX_VOLTAGE = +9.0;

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

    void plot(const Analog::TorporSlothCircuit& circuit)
    {
        using namespace Analog;

        double vx = circuit.xVoltage();
        double vy = circuit.yVoltage();
        //printf("vx=%lg, vy=%lg\n", vx, vy);

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

int main()
{
    using namespace Analog;

    Plotter plotter(5000);

    TorporSlothCircuit circuit;
    circuit.setControlVoltage(-1.0);
    circuit.setKnobPosition(0.0);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sloth Torpor");
    SetTargetFPS(30);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        plotter.plot(circuit);
        EndDrawing();
        for (int s = 0; s < SAMPLES_PER_FRAME; ++s)
            circuit.update(SAMPLE_RATE);
    }
    CloseWindow();
    return 0;
}

