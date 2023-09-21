#include <cstdio>
#include "torpor_sloth_circuit.hpp"

int main()
{
    using namespace Analog;

    TorporSlothCircuit circuit;
    circuit.setControlVoltage(+5.0);
    circuit.setKnobPosition(0.0);

    const int SAMPLE_RATE = 44100;
    const int SIMULATION_SECONDS = 10;
    const int SIMULATION_SAMPLES = SIMULATION_SECONDS * SAMPLE_RATE;

    const char *filename = "output/sloth.csv";
    FILE *outfile = fopen(filename, "wt");
    if (outfile == nullptr)
    {
        printf("ERROR: Cannot open output file: %s\n", filename);
        return 1;
    }

    for (int sample = 0; sample < SIMULATION_SAMPLES; ++sample)
    {
        circuit.update(SAMPLE_RATE);
        double x = circuit.xVoltage();
        double y = circuit.yVoltage();
        fprintf(outfile, "%d,%0.16lg,%0.16lg\n", sample, x, y);
    }

    fclose(outfile);
    return 0;
}
