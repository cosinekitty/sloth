#include <cstring>
#include <cstdio>
#include <cmath>
#include "circuit.hpp"

static int Animate();
static int UnitTest_ResistorFeedback();

int main(int argc, const char *argv[])
{
    if (UnitTest_ResistorFeedback()) return 1;
    if (argc == 2 && !strcmp(argv[1], "test"))
        return 0;   // stop after unit tests

    return Animate();
}


static int UnitTest_ResistorFeedback()
{
    using namespace std;
    using namespace Analog;

    Circuit circuit;

    int n0 = circuit.createNode();
    int n1 = circuit.createNode();
    int n2 = circuit.createNode();

    circuit.addFixedVoltage(n0, 1.0f);
    circuit.addResistor(1000.0f, n0, n1);
    circuit.addResistor(10000.0f, n1, n2);
    circuit.addOpAmp(n1, n2);

    circuit.solve();

    float vIn = circuit.getNodeVoltage(n0);
    printf("UnitTest_ResistorFeedback: input voltage = %0.6f V\n", vIn);
    if (vIn != 1.0f)
    {
        printf("FAIL(UnitTest_ResistorFeedback): expected vIn=1, but found %f\n", vIn);
    }

    float vOut = circuit.getNodeVoltage(n2);
    printf("UnitTest_ResistorFeedback: output voltage = %0.6f V\n", vOut);

    float diff = std::abs(vOut - 10.0f);
    if (diff > 1.0e-6)
    {
        printf("FAIL(UnitTest_ResistorFeedback): EXCESSIVE output voltage error = %f\n", diff);
        return 1;
    }

    return 0;
}


static int Animate()
{
    return 0;
}
