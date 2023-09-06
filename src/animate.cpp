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

int main(int argc, const char *argv[])
{
    if (UnitTest_ResistorFeedback()) return 1;
    if (UnitTest_VoltageDivider()) return 1;
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
    using namespace std;
    using namespace Analog;

    // Exercise series and parallel resistors combined in a voltage divider pattern.

    Circuit circuit;

    int np = circuit.createFixedVoltageNode(3.0);
    int n1 = circuit.createNode();
    int n2 = circuit.createNode();
    int ng = circuit.createGroundNode();

    circuit.addResistor(1000, np, n1);
    circuit.addResistor(2000, n1, n2);
    circuit.addResistor(2000, n1, n2);
    circuit.addResistor(1000, n2, ng);

    if (CheckSolution(circuit, "VoltageDivider1", n1, 2.0)) return 1;
    if (CheckSolution(circuit, "VoltageDivider2", n2, 1.0)) return 1;

    printf("VoltageDivider: PASS\n");
    return 0;
}


static int Animate()
{
    printf("INFO: Animation not yet implemented.\n");
    return 0;
}
