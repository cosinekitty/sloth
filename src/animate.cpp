#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <vector>
#include "raylib.h"
#include "torpor_sloth_circuit.hpp"

const int SCREEN_WIDTH  = 1000;
const int SCREEN_HEIGHT = 1000;
const int SAMPLE_RATE = 44100;
const int FRAME_RATE = 60;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / FRAME_RATE;

const double MIN_VOLTAGE = -10.0;
const double MAX_VOLTAGE = +10.0;

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
    const int maxTrailLength;
    int trailIndex = 0;
    std::vector<PlotPoint> trail;

public:
    explicit Plotter(int _maxTrailLength)
        : maxTrailLength(_maxTrailLength)
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

        int length = static_cast<int>(trail.size());
        if (length < maxTrailLength)
        {
            trail.push_back(PlotPoint(sx, sy));
            for (int i = 1; i < length; ++i)
            {
                const PlotPoint &a = trail.at(i-1);
                const PlotPoint &b = trail.at(i);
                DrawLine(a.x, a.y, b.x, b.y, GREEN);
            }
        }
        else
        {
            trail.at(trailIndex) = PlotPoint(sx, sy);
            trailIndex = (trailIndex + 1) % maxTrailLength;

            Color color = BLACK;
            Color target = GREEN;

            int i = trailIndex;
            while (true)
            {
                int j = (i + 1) % maxTrailLength;
                if (j == trailIndex)
                    break;

                const PlotPoint &a = trail.at(i);
                const PlotPoint &b = trail.at(j);
                DrawLine(a.x, a.y, b.x, b.y, color);
                i = j;

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
    circuit.setControlVoltage(-1.3);
    circuit.setKnobPosition(0.25);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sloth Torpor");
    SetTargetFPS(80);
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

