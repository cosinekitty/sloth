#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include "circuit.hpp"

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

static int Animate();
static int UnitTest_ResistorFeedback();
static int UnitTest_VoltageDivider();
static int UnitTest_ResistorCapacitorTimeConstant();

int main(int argc, const char *argv[])
{
    if (UnitTest_ResistorFeedback()) return 1;
    if (UnitTest_VoltageDivider()) return 1;
    if (UnitTest_ResistorCapacitorTimeConstant()) return 1;

    if (argc == 2 && !strcmp(argv[1], "test"))
        return 0;   // stop after unit tests

    return Animate();
}


static int CheckSolution(
    Analog::Circuit& circuit,
    const char *name,
    int outNodeIndex,
    double vOutExpected,
    double tolerance = 1.0e-4)
{
    Analog::SolutionResult result = circuit.update(SAMPLE_RATE);
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

    Circuit circuit;

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
    if (CheckSolution(circuit, "ResistorFeedback1", n2, vExact)) return 1;

    vIn = 2.0;
    if (CheckSolution(circuit, "ResistorFeedback2", n2, circuit.VNEG)) return 1;

    vIn = -2.0;
    if (CheckSolution(circuit, "ResistorFeedback3", n2, circuit.VPOS)) return 1;

    printf("ResistorFeedback: PASS\n");
    return 0;
}


static int UnitTest_VoltageDivider()
{
    using namespace Analog;

    // Exercise series and parallel resistors combined in a voltage divider pattern.

    Circuit circuit;

    const double vpos = 3.0;
    const double res1 = 1000.0;

    int np = circuit.createFixedVoltageNode(vpos);
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

    if (CheckSolution(circuit, "VoltageDivider1", n1, 2.0)) return 1;
    if (CheckSolution(circuit, "VoltageDivider2", n2, 1.0)) return 1;

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

    // Define a circuit consisting of a resistor in series with a capacitor.
    // The top side of the resistor connects to +1V.
    // The bottom side of the resistor connects to the top of the capacitor.
    // The bottom of the capacitor connects to ground.
    // The capacitor voltage starts at 0V.
    // We want to keep the math simple, so let R = 1 ohm, F = 1 farad.

    const double resistance = 1.0;
    const double capacitance = 1.0;
    const double rc = resistance * capacitance;
    const double supplyVoltage = 1.0;

    Circuit circuit;
    int n0 = circuit.createFixedVoltageNode(supplyVoltage);
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

    fprintf(outfile, "sample,time,iterations,voltage,expected,diff\n");

    // Charge up the capacitor by running for a simulated 3 seconds.
    const int nsamples = SAMPLE_RATE * 3;
    for (int sample = 0; sample < nsamples; ++sample)
    {
        SolutionResult result = circuit.update(SAMPLE_RATE);

        double time = static_cast<double>(sample) / SAMPLE_RATE;
        double voltage = circuit.getNodeVoltage(n1);

        // Compare to theoretical voltage.
        double expected = supplyVoltage*(1.0 - std::exp(-time/rc));

        double diff = voltage - expected;

        // Every 0.01 seconds (441 samples), write a CSV record to the output file.
        if (sample % (SAMPLE_RATE / 100) == 0)
        {
            fprintf(outfile, "%d,%0.6lf,%d,%0.16lg,%0.16lg,%0.16lg\n",
                sample, time, result.iterations, voltage, expected, diff);
        }
    }

    fclose(outfile);

    printf("ResistorCapacitorTimeConstant: PASS\n");
    return 0;
}


static int Animate()
{
    printf("INFO: Animation not yet implemented.\n");
    return 0;
}
