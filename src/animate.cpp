#include <cstring>
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

    return 0;
}


static int Animate()
{
    return 0;
}
