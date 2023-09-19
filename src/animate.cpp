/*
    animate.cpp  -  Don Cross <cosinekitty@gmail.com>  -  2023-09-10

    Runs the Sloth Torpor simulation and renders an X/Y plot.

    https://github.com/cosinekitty/sloth
*/

#include "torpor_sloth_circuit.hpp"
#include "plotter.hpp"


int main()
{
    using namespace Analog;

    Plotter plotter(5000);

    TorporSlothCircuit circuit;
    circuit.setControlVoltage(-1.0);
    circuit.setKnobPosition(0.0);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sloth Torpor Simulation");
    SetTargetFPS(30);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        plotter.plot(circuit.xVoltage(), circuit.yVoltage());
        EndDrawing();
        for (int s = 0; s < SAMPLES_PER_FRAME; ++s)
            circuit.update(SAMPLE_RATE);
    }
    CloseWindow();
    return 0;
}

