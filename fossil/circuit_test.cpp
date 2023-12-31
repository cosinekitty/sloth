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

static void Print(const Analog::Circuit& circuit);
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
    double voltageTolerance = 3.3e-6)
{
    Analog::SolutionResult result(0, 0, 0.0);

    for (int s = 0; s < nsamples; ++s)
        result = circuit.update(SAMPLE_RATE);

    double vOut = V(circuit.getNodeVoltage(outNodeIndex));
    double diff = ABS(vOut - vOutExpected);
    printf("CheckSolution(%s): %d node voltage updates, %d current updates, rms = %lg amps, diff = %lg V on node %d\n",
        name, result.adjustNodeVoltagesCount, result.currentUpdates, result.rmsCurrentError, diff, outNodeIndex);

    if (diff > voltageTolerance)
    {
        printf("FAIL(%s): EXCESSIVE voltage error %lg on node %d.\n", name, diff, outNodeIndex);
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

    int n0 = circuit.createNode();
    int n1 = circuit.createNode();
    int n2 = circuit.createNode();

    if (circuit.getNodeCount() != 3)
    {
        printf("FAIL(ResistorFeedback): Incorrect node count = %d\n", circuit.getNodeCount());
        return 1;
    }

    circuit.allocateForcedVoltageNode(n0);
    circuit.addResistor(1000.0f, n0, n1);
    circuit.addResistor(10000.0f, n1, n2);
    circuit.addLinearAmp(n1, n2);
    circuit.lock();

    double& vIn = circuit.nodeVoltage(n0);
    vIn = 1.0;
    if (CheckSolution(circuit, 1, "ResistorFeedback1", n2, -10.0 * vIn)) return 1;

    vIn = 2.0;
    if (CheckSolution(circuit, 1, "ResistorFeedback2", n2, -10.0 * vIn)) return 1;

    vIn = -2.0;
    if (CheckSolution(circuit, 1, "ResistorFeedback3", n2, -10.0 * vIn)) return 1;

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
    if (diff > 6.0e-10)
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

    fprintf(outfile, "sample,time,adjustNodeVoltagesCount,score,voltage,expected,diff\n");

    // Charge up the capacitor by running for a simulated 3 seconds.
    const int nsamples = SAMPLE_RATE * 3;
    int adjustNodeVoltagesCount = 0;
    int maxAdjustNodeVoltagesCount = 0;
    long totalAdjustNodeVoltagesCount = 0;
    long totalCurrentUpdates = 0;
    double rms = 0.0;
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
                sample, time, adjustNodeVoltagesCount, rms, voltage, expected, diff);

            fflush(outfile);
        }

        SolutionResult result = circuit.update(SAMPLE_RATE);
        adjustNodeVoltagesCount = result.adjustNodeVoltagesCount;
        maxAdjustNodeVoltagesCount = std::max(maxAdjustNodeVoltagesCount, adjustNodeVoltagesCount);
        totalAdjustNodeVoltagesCount += adjustNodeVoltagesCount;
        totalCurrentUpdates += result.currentUpdates;
        rms = result.rmsCurrentError;
    }

    fclose(outfile);

    PerformanceStats stats = circuit.getPerformanceStats();
    if (stats.totalSamples != nsamples)
    {
        printf("ResistorCapacitorTimeConstant: stats.totalSamples=%ld, but nsamples=%d\n", stats.totalSamples, nsamples);
        return 1;
    }

    if (stats.totalAdjustNodeVoltagesCount != totalAdjustNodeVoltagesCount)
    {
        printf("ResistorCapacitorTimeConstant: stats.totalAdjustNodeVoltagesCount=%ld, but totalAdjustNodeVoltagesCount=%ld\n", stats.totalAdjustNodeVoltagesCount, totalAdjustNodeVoltagesCount);
        return 1;
    }

    if (stats.totalCurrentUpdates != totalCurrentUpdates)
    {
        printf("ResistorCapacitorTimeConstant: stats.totalCurrentUpdates=%ld, but totalCurrentUpdates=%ld\n", stats.totalCurrentUpdates, totalCurrentUpdates);
        return 1;
    }

    if (maxdiff > 1.8e-5)
    {
        printf("ResistorCapacitorTimeConstant: FAIL - excessive capacitor voltage error = %0.6lg\n", maxdiff);
        return 1;
    }

    double meanIterations = static_cast<double>(totalAdjustNodeVoltagesCount) / nsamples;
    printf("ResistorCapacitorTimeConstant: PASS (mean iterations = %0.3lf, max = %d, mean current updates = %lg, capacitor voltage error = %lg)\n",
        meanIterations, maxAdjustNodeVoltagesCount, stats.meanCurrentUpdatesPerSample(), maxdiff);
    return 0;
}


static int UnitTest_Torpor()
{
    using namespace Analog;

    printf("Torpor: starting\n");

    TorporSlothCircuit circuit;
    Print(circuit);

    circuit.debug = false;
    circuit.setControlVoltage(-1.3);
    circuit.setKnobPosition(0.25);

    long totalVoltageUpdates = 0;
    long totalCurrentUpdates = 0;

    double startTime = TimeInSeconds();
    const int nseconds = 120;
    const int nsamples = nseconds * SAMPLE_RATE;
    double maxRmsCurrentError = 0;
    for (int sample = 0; sample < nsamples; ++sample)
    {
        SolutionResult result = circuit.update(SAMPLE_RATE);
        totalVoltageUpdates += result.adjustNodeVoltagesCount;
        totalCurrentUpdates += result.currentUpdates;
        double vx = circuit.xVoltage();
        double vy = circuit.yVoltage();
        double vz = circuit.zVoltage();
        if (result.rmsCurrentError > maxRmsCurrentError)
            maxRmsCurrentError = result.rmsCurrentError;

        if (result.rmsCurrentError > 5.0)
        {
            printf("Torpor(sample %d): FAIL: EXCESSIVE rms current error = %lg nA\n", sample, maxRmsCurrentError);
            return 1;
        }

        if (sample < 10 || sample % SAMPLE_RATE == 0)
        {
            printf("Torpor: sample=%d, adjustNodeVoltagesCount=%d, currentUpdates=%d, rms=%lg, x=%0.6lf, y=%0.6lf, z=%0.6lf\n",
                sample,
                result.adjustNodeVoltagesCount,
                result.currentUpdates,
                result.rmsCurrentError,
                vx, vy, vz);
        }

        if (vx < circuit.VNEG || vx > circuit.VPOS)
        {
            printf("Torpor(%d): output voltage vx=%lg is out of bounds!\n", sample, vx);
            return 1;
        }

        if (vy < circuit.VNEG || vy > circuit.VPOS)
        {
            printf("Torpor(%d): output voltage vy=%lg is out of bounds!\n", sample, vy);
            return 1;
        }

        if (vz < circuit.VNEG || vz > circuit.VPOS)
        {
            printf("Torpor(%d): output voltage vz=%lg is out of bounds!\n", sample, vz);
            return 1;
        }
    }
    double elapsedTime = TimeInSeconds() - startTime;

    PerformanceStats stats = circuit.getPerformanceStats();

    if (stats.totalAdjustNodeVoltagesCount != totalVoltageUpdates)
    {
        printf("Torpor: FAIL: stats.totalAdjustNodeVoltagesCount = %ld, but totalVoltageUpdates = %ld\n",
            stats.totalAdjustNodeVoltagesCount,
            totalVoltageUpdates
        );
        return 1;
    }

    if (stats.totalCurrentUpdates != totalCurrentUpdates)
    {
        printf("Torpor: FAIL: stats.totalCurrentUpdates = %ld, but totalCurrentUpdates = %ld\n",
            stats.totalCurrentUpdates,
            totalCurrentUpdates
        );
        return 1;
    }

    printf("Torpor: PASS -- meanAdjustNodeVoltages=%lg, meanCurrentUpdates=%lg, max rms=%lg nA, simulated %d seconds in %0.3lf seconds of real time.\n",
        stats.meanAdjustNodeVoltagesPerSample(),
        stats.meanCurrentUpdatesPerSample(),
        maxRmsCurrentError,
        nseconds,
        elapsedTime
    );
    return 0;
}


inline const char *BoolString(bool x)
{
    return x ? "true" : "false";
}


inline const char *Sep(int i, int n)
{
    return (i+1 < n) ? "," : "";
}

static void Print(const Analog::Circuit& circuit)
{
    using namespace Analog;

    printf("{\n");

    printf("    \"nodes\": [\n");
    const int nn = circuit.getNodeCount();
    for (int i = 0; i < nn; ++i)
    {
        const Node& n = circuit.getNode(i);
        printf("        {\"forcedVoltage\":%s, \"currentSink\":%s}%s\n", BoolString(n.forcedVoltage), BoolString(n.currentSink), Sep(i, nn));
    }
    printf("    ],\n");

    printf("    \"resistors\": [\n");
    const int nr = circuit.getResistorCount();
    for (int i = 0; i < nr; ++i)
    {
        const Resistor& r = circuit.resistor(i);
        printf("        {\"resistance\": %0.16lg, \"nodes\":[%d, %d]}%s\n", r.resistance, r.aNodeIndex, r.bNodeIndex, Sep(i, nr));
    }
    printf("    ],\n");

    printf("    \"capacitors\": [\n");
    const int nc = circuit.getCapacitorCount();
    for (int i = 0; i < nc; ++i)
    {
        const Capacitor& c = circuit.capacitor(i);
        printf("        {\"capacitance\": %0.16lg, \"nodes\":[%d, %d]}%s\n", c.capacitance, c.aNodeIndex, c.bNodeIndex, Sep(i, nc));
    }
    printf("    ],\n");

    printf("    \"linearAmps\": [\n");
    const int na = circuit.getLinearAmpCount();
    for (int i = 0; i < na; ++i)
    {
        const LinearAmp& a = circuit.linearAmp(i);
        printf("        {\"nodes\": [%d, %d]}%s\n", a.negNodeIndex, a.outNodeIndex, Sep(i, na));
    }
    printf("    ],\n");

    printf("    \"comparators\": [\n");
    const int nk = circuit.getComparatorCount();
    for (int i = 0; i < nk; ++i)
    {
        const Comparator& k = circuit.comparator(i);
        printf("        {\"nodes\": [%d, %d]}%s\n", k.negNodeIndex, k.outNodeIndex, Sep(i, nk));
    }
    printf("    ]\n");

    printf("}\n");
}
