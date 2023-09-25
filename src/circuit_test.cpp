#include <cstdio>
#include "SlothCircuit.hpp"
#include "TimeInSeconds.hpp"


static bool PerformanceAndStability();

int main()
{
    return (
        PerformanceAndStability()
    ) ? 0 : 1;
}

inline bool CheckVoltage(double v, const char *name, int sample)
{
    if (!std::isfinite(v))
    {
        printf("FAILURE - Non-finite value for %s in sample %d", name, sample);
        return false;
    }

    if (v < -10.0 || v > +10.0)
    {
        printf("FAILURE - %s = %lg V is out of bounds in sample %d", name, v, sample);
        return false;
    }

    return true;
}


static bool PerformanceAndStability()
{
    using namespace Analog;

    TorporSlothCircuit circuit;
    circuit.setControlVoltage(+0.1);
    circuit.setKnobPosition(0.5);

    const int SAMPLE_RATE = 44100;
    const int SIMULATION_SECONDS = 3600;
    const int SIMULATION_SAMPLES = SIMULATION_SECONDS * SAMPLE_RATE;

    printf("PerformanceAndStability: Simulating %d seconds of sloth...\n", SIMULATION_SECONDS);
    long iterSum = 0;
    int maxIter = 0;
    double startTime = TimeInSeconds();
    for (int sample = 0; sample < SIMULATION_SAMPLES; ++sample)
    {
        int iter = circuit.update(SAMPLE_RATE);
        if (iter < 1 || iter >= circuit.iterationLimit)
        {
            printf("PerformanceAndStability: SIMULATION FAILURE at sample %d: unexpected iteration count %d\n", sample, iter);
            return false;
        }

        if (!CheckVoltage(circuit.xVoltage(), "x", sample)) return false;
        if (!CheckVoltage(circuit.yVoltage(), "y", sample)) return false;
        if (!CheckVoltage(circuit.zVoltage(), "z", sample)) return false;

        iterSum += iter;
        maxIter = std::max(maxIter, iter);
    }
    double elapsedSeconds = TimeInSeconds() - startTime;
    printf("PerformanceAndStability: Elapsed time = %0.3lf seconds, ratio = %0.6lg, CPU overhead = %lg percent.\n",
        elapsedSeconds,
        SIMULATION_SECONDS / elapsedSeconds,
        100.0 * (elapsedSeconds / SIMULATION_SECONDS)
    );

    double meanIter = static_cast<double>(iterSum) / SIMULATION_SAMPLES;
    printf("PerformanceAndStability: Mean iter = %0.16lf, max iter = %d.\n", meanIter, maxIter);

    printf("PerformanceAndStability: PASS\n");
    return true;
}
