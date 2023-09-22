#include <cstdio>
#include "torpor_sloth_circuit.hpp"

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

double TimeInSeconds()
{
    double sec;         // Seconds since midnight January 1, 1970.

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sec = (double)tv.tv_sec + tv.tv_usec/1.0e+6;
#elif defined(_WIN32)
    FILETIME ft;
    ULARGE_INTEGER large;
    // Get time in 100-nanosecond units from January 1, 1601.
    GetSystemTimePreciseAsFileTime(&ft);
    large.u.LowPart  = ft.dwLowDateTime;
    large.u.HighPart = ft.dwHighDateTime;
    sec = (large.QuadPart - 116444736000000000ULL) / 1.0e+7;
#else
    #error Microsecond time resolution is not supported on this platform.
#endif

    return sec;
}

int main()
{
    using namespace Analog;

    TorporSlothCircuit circuit;
    circuit.setControlVoltage(+5.0);
    circuit.setKnobPosition(0.0);

    const int SAMPLE_RATE = 44100;
    const int SIMULATION_SECONDS = 3600;
    const int SIMULATION_SAMPLES = SIMULATION_SECONDS * SAMPLE_RATE;

    printf("Simulating %d seconds of sloth...\n", SIMULATION_SECONDS);
    long iterSum = 0;
    int maxIter = 0;
    double startTime = TimeInSeconds();
    for (int sample = 0; sample < SIMULATION_SAMPLES; ++sample)
    {
        int iter = circuit.update(SAMPLE_RATE);
        if (iter < 1)
        {
            printf("SIMULATION FAILURE at sample %d.\n", sample);
            return 1;
        }
        iterSum += iter;
        maxIter = std::max(maxIter, iter);
        double x = circuit.xVoltage();
        if (x < circuit.VNEG || x > circuit.VPOS)
        {
            printf("INVALID x voltage %0.6lg at sample %d.\n", x, sample);
            return 1;
        }

        double y = circuit.yVoltage();
        if (y < circuit.VNEG || y > circuit.VPOS)
        {
            printf("INVALID y voltage %0.6lg at sample %d.\n", y, sample);
            return 1;
        }
    }
    double elapsedSeconds = TimeInSeconds() - startTime;
    printf("Elapsed time = %0.3lf seconds, ratio = %0.6lg, CPU overhead = %lg percent.\n",
        elapsedSeconds,
        SIMULATION_SECONDS / elapsedSeconds,
        100.0 * (elapsedSeconds / SIMULATION_SECONDS)
    );

    double meanIter = static_cast<double>(iterSum) / SIMULATION_SAMPLES;
    printf("Mean iter = %0.16lf, max iter = %d.\n", meanIter, maxIter);

    return 0;
}
