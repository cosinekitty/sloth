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

    return 0;
}


static int Animate()
{
    return 0;
}
