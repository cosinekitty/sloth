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

int main(int argc, const char *argv[])
{
    if (UnitTest_ResistorFeedback()) return 1;
    if (argc == 2 && !strcmp(argv[1], "test"))
        return 0;   // stop after unit tests

    return Animate();
}


static int CheckSolution(
    const Analog::Circuit& circuit,
    const char *name,
    int outNodeIndex,
    double vOutExpected,
    double tolerance = 1.0e-6)
{
    double vOut = V(circuit.getNodeVoltage(outNodeIndex));
    double diff = ABS(vOut - vOutExpected);
    if (diff > tolerance)
    {
        printf("FAIL(%s): EXCESSIVE voltage error %f on node %d.\n", name, diff, outNodeIndex);
        return 1;
    }
    return 0;
}


static int UnitTest_ResistorFeedback()
{
    using namespace std;
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

    double &vIn = circuit.allocateForcedVoltageNode(n0);
    circuit.addResistor(1000.0f, n0, n1);
    circuit.addResistor(10000.0f, n1, n2);
    circuit.addOpAmp(ng, n1, n2);

    vIn = 1.0;
    circuit.update(SAMPLE_RATE);

    double vCheck = circuit.getNodeVoltage(n0);
    printf("ResistorFeedback: input voltage = %0.6lf V\n", vCheck);
    if (vCheck != vIn)
    {
        printf("FAIL(ResistorFeedback): vIn=%lf, but vCheck=%lf\n", vIn, vCheck);
        return 1;
    }

    if (CheckSolution(circuit, "ResistorFeedback1", n2, -10.0f)) return 1;

    // Verify that op-amp clamping is working as expected, on both the low end and the high end.
    vIn = 2.0;
    circuit.update(SAMPLE_RATE);
    if (CheckSolution(circuit, "ResistorFeedback2", n2, circuit.VNEG)) return 1;

    vIn = -2.0;
    circuit.update(SAMPLE_RATE);
    if (CheckSolution(circuit, "ResistorFeedback3", n2, circuit.VPOS)) return 1;

    printf("ResistorFeedback: PASS\n");
    return 0;
}


static int Animate()
{
    return 0;
}
