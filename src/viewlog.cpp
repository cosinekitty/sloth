/*
    animate.cpp  -  Don Cross <cosinekitty@gmail.com>  -  2023-09-10

    Runs the Sloth Torpor simulation and renders an X/Y plot.

    https://github.com/cosinekitty/sloth
*/

#include <cstdio>
#include "plotter.hpp"


double ArduinoVoltage(int a)
{
    // https://docs.google.com/spreadsheets/d/12UB6mihdSQhdCoypwO_CTBR4rIwEmjkxOjuWeSCxXrg/edit#gid=0
    // The range a=[18, 1019] maps to circuit voltages [-12, +12].
    return ((a - 18.0) / (1019.0 - 18.0))*24.0 - 12.0;
}


int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        printf("USAGE: viewlog filename.csv\n");
        return 1;
    }
    const char *filename = argv[1];
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
                int millis, ax, ay, az;
                int n = sscanf(line, "%d,%d,%d,%d", &millis, &ax, &ay, &az);
                if (n != 4)
                {
                    printf("Unexpected number of circuit data read from %s: %d\n", filename, n);
                    fclose(infile);
                    infile = nullptr;
                }
                else
                {
                    ClearBackground(BLACK);
                    double vx = ArduinoVoltage(ax);
                    double vy = ArduinoVoltage(ay);
                    plotter.plot(vx, vy);
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
