#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include "circuit.hpp"
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

const int SAMPLE_RATE = 44100;

template <typename real_t>
real_t v(real_t x, const char *fn, int lnum)
{
    if (!std::isfinite(x))
    {
        std::string message = "FAIL(";
        message += fn;
        message += " line ";
        message += std::to_string(lnum);
        message += "): number is not finite.";
        throw std::range_error(message);
    }
    return x;
}

#define V(x)    v(x, __FILE__, __LINE__)
#define ABS(x)  std::abs(v(x, __FILE__, __LINE__))

static int UnitTest_ResistorFeedback();
static int UnitTest_VoltageDivider();
static int UnitTest_ResistorCapacitorTimeConstant();
static int UnitTest_Torpor();

int main(int argc, const char *argv[])
{
    try
    {
        if (UnitTest_ResistorCapacitorTimeConstant()) return 1;
        if (UnitTest_ResistorFeedback()) return 1;
        if (UnitTest_VoltageDivider()) return 1;
        if (UnitTest_Torpor()) return 1;
    }
    catch (const std::exception& ex)
    {
        printf("circuit_test EXCEPTION: %s\n", ex.what());
        return 9;
    }
}


static int CheckSolution(
    Analog::Circuit& circuit,
    int nsamples,
    const char *name,
    int outNodeIndex,
    double vOutExpected,
    double tolerance = 1.0e-4)
{
    Analog::SolutionResult result(0, 0.0);

    for (int s = 0; s < nsamples; ++s)
        result = circuit.update(SAMPLE_RATE);

    double vOut = V(circuit.getNodeVoltage(outNodeIndex));
    double diff = ABS(vOut - vOutExpected);
    printf("CheckSolution(%s): %d iterations, score = %lg amps, diff = %lg V on node %d\n",
        name, result.iterations, result.score, diff, outNodeIndex);

    if (diff > tolerance)
    {
        printf("FAIL(%s): EXCESSIVE voltage error %f on node %d.\n", name, diff, outNodeIndex);
        return 1;
    }
    return 0;
}


static int UnitTest_ResistorFeedback()
{
    using namespace Analog;

    printf("ResistorFeedback: starting.\n");

    Circuit circuit;

    circuit.debug = false;
    circuit.opAmpSlewRateHalfLifeSeconds = 0.02;

    int n0 = circuit.createNode();
    int n1 = circuit.createNode();
    int n2 = circuit.createNode();
    int ng = circuit.createGroundNode();

    if (circuit.getNodeCount() != 4)
    {
        printf("FAIL(ResistorFeedback): Incorrect node count = %d\n", circuit.getNodeCount());
        return 1;
    }

    circuit.allocateForcedVoltageNode(n0);
    circuit.addResistor(1000.0f, n0, n1);
    circuit.addResistor(10000.0f, n1, n2);
    circuit.addOpAmp(ng, n1, n2);
    circuit.lock();

    double& vIn = circuit.nodeVoltage(n0);
    vIn = 1.0;
    double vExact = -1.0e+7 / 1000011.0;    // manually derived exact solution
    if (CheckSolution(circuit, 2*SAMPLE_RATE, "ResistorFeedback1", n2, vExact)) return 1;

    vIn = 2.0;
    if (CheckSolution(circuit, 2*SAMPLE_RATE, "ResistorFeedback2", n2, circuit.VNEG)) return 1;

    vIn = -2.0;
    if (CheckSolution(circuit, 2*SAMPLE_RATE, "ResistorFeedback3", n2, circuit.VPOS)) return 1;

    printf("ResistorFeedback: PASS\n");
    return 0;
}


static int UnitTest_VoltageDivider()
{
    using namespace Analog;

    // Exercise series and parallel resistors combined in a voltage divider pattern.
    printf("VoltageDivider: starting.\n");

    Circuit circuit;

    const double vpos = 3.0;
    const double res1 = 1000.0;

    int np = circuit.createForcedVoltageNode(vpos);
    int n1 = circuit.createNode();
    int n2 = circuit.createNode();
    int ng = circuit.createGroundNode();

    int r0_index = circuit.addResistor(res1, np, n1);
    int r1_index = circuit.addResistor(2*res1, n1, n2);
    circuit.addResistor(2*res1, n1, n2);
    circuit.addResistor(res1, n2, ng);
    circuit.lock();

    const Resistor& r0 = circuit.resistor(r0_index);
    const Resistor& r1 = circuit.resistor(r1_index);

    if (CheckSolution(circuit, 1, "VoltageDivider1", n1, 2.0)) return 1;
    if (CheckSolution(circuit, 1, "VoltageDivider2", n2, 1.0)) return 1;

    const double i0 = vpos / (3 * res1);
    double diff = ABS(r0.current - i0);
    if (diff > 1.0e-8)
    {
        printf("FAIL(VoltageDivider): EXCESSIVE r0.current error = %lg; r0.current = %lg\n", diff, r0.current);
        return 1;
    }

    const double i1 = i0 / 2.0;     // half the current goes through the parallel resistor
    diff = ABS(r1.current - i1);
    if (diff > 1.0e-8)
    {
        printf("FAIL(VoltageDivider): EXCESSIVE r1.current error = %lg\n", diff);
        return 1;
    }

    printf("VoltageDivider: PASS (current diff = %lg)\n", diff);
    return 0;
}


static int UnitTest_ResistorCapacitorTimeConstant()
{
    using namespace Analog;

    printf("ResistorCapacitorTimeConstant: starting.\n");

    // Define a circuit consisting of a resistor in series with a capacitor.
    // The top side of the resistor connects to +1V.
    // The bottom side of the resistor connects to the top of the capacitor.
    // The bottom of the capacitor connects to ground.
    // The capacitor voltage starts at 0V.
    // We want to keep the math simple, so let RC = 1 second.

    const double resistance = 1.0e+6;
    const double capacitance = 1.0e-6;
    const double rc = resistance * capacitance;
    const double supplyVoltage = 1.0;

    Circuit circuit;
    circuit.scoreTolerance = 5.0e-11;
    int n0 = circuit.createForcedVoltageNode(supplyVoltage);
    int n1 = circuit.createNode();
    int n2 = circuit.createGroundNode();
    circuit.addResistor(resistance, n0, n1);
    circuit.addCapacitor(capacitance, n1, n2);
    circuit.lock();

    const char *filename = "output/rc.txt";
    FILE *outfile = fopen(filename, "wt");
    if (outfile == nullptr)
    {
        printf("ResistorCapacitorTimeConstant: Cannot open output file: %s\n", filename);
        return 1;
    }

    fprintf(outfile, "sample,time,iterations,score,voltage,expected,diff\n");

    // Charge up the capacitor by running for a simulated 3 seconds.
    const int nsamples = SAMPLE_RATE * 3;
    int iterations = 0;
    int maxIterations = 0;
    int totalIterations = 0;
    double score = 0.0;
    double maxdiff = 0.0;
    for (int sample = 0; sample < nsamples; ++sample)
    {
        double time = static_cast<double>(sample) / SAMPLE_RATE;
        double voltage = circuit.getNodeVoltage(n1);

        // Compare to theoretical voltage.
        double expected = supplyVoltage*(1.0 - std::exp(-time/rc));

        double diff = voltage - expected;
        maxdiff = std::max(maxdiff, ABS(diff));

        // Every 0.01 seconds, write a CSV record to the output file.
        if (sample % (SAMPLE_RATE / 100) == 0)
        {
            fprintf(outfile, "%d,%0.2lf,%d,%lg,%0.16lg,%0.16lg,%0.16lg\n",
                sample, time, iterations, score, voltage, expected, diff);

            fflush(outfile);
        }

        SolutionResult result = circuit.update(SAMPLE_RATE);
        iterations = result.iterations;
        maxIterations = std::max(maxIterations, iterations);
        totalIterations += iterations;
        score = result.score;
    }

    fclose(outfile);

    PerformanceStats stats = circuit.getPerformanceStats();
    if (stats.totalSamples != nsamples)
    {
        printf("ResistorCapacitorTimeConstant: stats.totalSamples=%ld, but nsamples=%d\n", stats.totalSamples, nsamples);
        return 1;
    }

    if (stats.totalIterations != totalIterations)
    {
        printf("ResistorCapacitorTimeConstant: stats.totalIterations=%ld, but totalIterations=%d\n", stats.totalIterations, totalIterations);
        return 1;
    }

    if (maxdiff > 3.31e-5)
    {
        printf("ResistorCapacitorTimeConstant: FAIL - excessive capacitor voltage error = %0.6lg\n", maxdiff);
        return 1;
    }

    double meanIterations = static_cast<double>(totalIterations) / nsamples;
    printf("ResistorCapacitorTimeConstant: PASS (mean iterations = %0.3lf, max = %d, capacitor voltage error = %lg)\n", meanIterations, maxIterations, maxdiff);
    return 0;
}


static int UnitTest_Torpor()
{
    using namespace Analog;

    printf("Torpor: starting\n");

    TorporSlothCircuit circuit;

    circuit.debug = false;
    circuit.deltaVoltage = 1.0e-9;
    circuit.retryLimit = 100;
    circuit.setControlVoltage(-1.3);
    circuit.setKnobPosition(0.25);

    double startTime = TimeInSeconds();
    const int nseconds = 10;
    const int nsamples = nseconds * SAMPLE_RATE;
    for (int sample = 0; sample < nsamples; ++sample)
    {
        SolutionResult result = circuit.update(SAMPLE_RATE);
        double vx = circuit.xVoltage();
        double vy = circuit.yVoltage();
        double vz = circuit.zVoltage();

        if (sample % SAMPLE_RATE == 0)
        {
            printf("Torpor: sample=%d, iterations=%d, score=%lg, x=%0.6lf, y=%0.6lf, z=%0.6lf\n",
                sample, result.iterations, result.score,
                vx, vy, vz);
        }

        if (vx < circuit.VNEG || vx > circuit.VPOS)
        {
            printf("Torpor: output voltage X is out of bounds!\n");
            return 1;
        }

        if (vy < circuit.VNEG || vy > circuit.VPOS)
        {
            printf("Torpor: output voltage Y is out of bounds!\n");
            return 1;
        }

        if (vz < circuit.VNEG || vz > circuit.VPOS)
        {
            printf("Torpor: output voltage Z is out of bounds!\n");
            return 1;
        }
    }
    double elapsedTime = TimeInSeconds() - startTime;

    PerformanceStats stats = circuit.getPerformanceStats();

    printf("Torpor: PASS -- mean_iter=%lg, simulated %d seconds in %0.3lf seconds of real time.\n", stats.meanIterationsPerSample(), nseconds, elapsedTime);
    return 0;
}
