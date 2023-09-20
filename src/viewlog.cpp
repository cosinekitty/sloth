/*
    animate.cpp  -  Don Cross <cosinekitty@gmail.com>  -  2023-09-10

    Runs the Sloth Torpor simulation and renders an X/Y plot.

    https://github.com/cosinekitty/sloth
*/

#include <cstdio>
#include <cstring>
#include "plotter.hpp"


double ArduinoVoltage(int a)
{
    // https://docs.google.com/spreadsheets/d/12UB6mihdSQhdCoypwO_CTBR4rIwEmjkxOjuWeSCxXrg/edit#gid=0
    // The range a=[18, 1019] maps to circuit voltages [-12, +12].
    return ((a - 18.0) / (1019.0 - 18.0))*24.0 - 12.0;
}


double Node3Voltage(int a)
{
    // https://docs.google.com/spreadsheets/d/12UB6mihdSQhdCoypwO_CTBR4rIwEmjkxOjuWeSCxXrg/edit#gid=1457893736
    // I used a different op-amp circuit than the other voltages, because
    // this node's voltage varies from about -0.25 V to +0.25 V.
    // I assumed a safe range of -0.5 V to +0.5 V.
    // Then I chose 4 resistors combined with the op-amp to produce
    // the formula A = 5*w + 2.5, to convert to the range [0 V, +5 V].
    // Based on direct measurements, I found the following:
    // w = -0.499 V     ==>  A = -0.014 V   ==>     -2.96 arduino units
    // w =  0 V         ==>  A =  2.52 V    ==>    515.59 arduino units
    // w = +0.500 V     ==>  A = +5.06 V    ==>   1035.18 arduino units
    // Using these as a reference, I came up with the following formula
    // for the inferred value of the voltage `w` from the arduino units `a`:
    return (a*(5.0 / 1023.0) - 2.52) / 5.079079079;
}


double SelectVoltage(char varname, double x, double y, double z, double w)
{
    switch (varname)
    {
    case 'x': return x;
    case 'y': return y;
    case 'z': return z;
    case 'w': return w;
    default:  return 0;
    }
}


int main(int argc, const char *argv[])
{
    if (argc != 3)
    {
        printf("USAGE: viewlog filename.csv varpair\n");
        return 1;
    }
    const char *filename = argv[1];
    const char *varlist = argv[2];
    if (strlen(varlist) != 2)
    {
        printf("ERROR: The second parameter must contain a pair of variables to plot from the list x, y, z, w.\n");
        return 1;
    }

    FILE *infile = fopen(filename, "rt");
    if (infile == nullptr)
    {
        printf("ERROR: Cannot open input file: %s\n", filename);
        return 1;
    }

    Plotter plotter(500);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sloth Torpor Data");
    SetTargetFPS(FRAME_RATE);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        if (infile != nullptr)
        {
            char line[100];
            if (fgets(line, sizeof(line), infile))
            {
                int millis, ax, ay, az, aw;
                int n = sscanf(line, "%d,%d,%d,%d,%d", &millis, &ax, &ay, &az, &aw);
                if (n < 4)
                {
                    printf("Unexpected number of circuit data read from %s: %d\n", filename, n);
                    fclose(infile);
                    infile = nullptr;
                }
                else
                {
                    if (n < 5) aw = 511;        // placeholder value near 0V for files that lack `w`
                    ClearBackground(BLACK);
                    double vx = ArduinoVoltage(ax);
                    double vy = ArduinoVoltage(ay);
                    double vz = ArduinoVoltage(az);
                    double vw = Node3Voltage(aw);       // `w` requires a different conversion formula
                    double first = SelectVoltage(varlist[0], vx, vy, vz, vw);
                    double second = SelectVoltage(varlist[1], vx, vy, vz, vw);
                    plotter.plot(first, second);
                }
            }
            else
            {
                printf("Hit end of file: %s\n", filename);
                fclose(infile);
                infile = nullptr;
            }
        }
        EndDrawing();
    }
    CloseWindow();
    if (infile != nullptr) fclose(infile);
    return 0;
}
