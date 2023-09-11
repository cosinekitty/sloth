#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include "raylib.h"
#include "torpor_sloth_circuit.hpp"

const int SCREEN_WIDTH  = 1000;
const int SCREEN_HEIGHT = 1000;
const int SAMPLE_RATE = 44100;
const int FRAME_RATE = 60;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / FRAME_RATE;

const double MIN_VOLTAGE = -12.1;
const double MAX_VOLTAGE = +12.1;

static void PlotVoltages(const Analog::TorporSlothCircuit& circuit);

int main()
{
    using namespace Analog;

    TorporSlothCircuit circuit;
    circuit.setControlVoltage(-1.3);
    circuit.setKnobPosition(0.25);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sloth Torpor");
    SetTargetFPS(80);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        PlotVoltages(circuit);
        EndDrawing();
        for (int s = 0; s < SAMPLES_PER_FRAME; ++s)
            circuit.update(SAMPLE_RATE);
    }
    CloseWindow();
    return 0;
}

static void PlotVoltages(const Analog::TorporSlothCircuit& circuit)
{
    using namespace Analog;

    double vx = circuit.xVoltage();
    double vy = circuit.yVoltage();

    // Map voltage ranges -12V..+12V to screen dimensions.
    int sx = static_cast<int>(std::round(((vx - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * SCREEN_WIDTH));
    int sy = static_cast<int>(std::round(((MAX_VOLTAGE - vy) / (MAX_VOLTAGE - MIN_VOLTAGE)) * SCREEN_HEIGHT));

    //printf("vx=%lg, vy=%lg\n", vx, vy);

    DrawCircle(sx, sy, 5.0f, GREEN);
}
