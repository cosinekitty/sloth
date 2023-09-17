/*
    torpor_sloth_circuit.hpp  -  Don Cross  -  2023-09-07
*/

#pragma once

#include "circuit.hpp"

namespace Analog
{
    class TorporSlothCircuit : public Circuit
    {
    private:
        double *variableResistance{};
        double *controlVoltage{};
        const double *xNodeVoltage{};
        const double *yNodeVoltage{};
        const double *zNodeVoltage{};

    public:
        TorporSlothCircuit()
        {
            int ng = createGroundNode();
            int n1 = createNode();
            int n2 = createNode();
            int n3 = createNode();
            int n4 = createNode();
            int n5 = createNode();
            int n6 = createNode();
            int n7 = createNode();
            int n8 = createNode();
            int n9 = createForcedVoltageNode(0.0);   // CV input node

            addLinearAmp(n1, n2);       // U3
            addLinearAmp(n4, n5);       // U4
            addLinearAmp(n6, n7);       // U2
            addComparator(n7, n8);      // U1

            addResistor(1.0e+6, n1, n7);    // R1
            addResistor(4.7e+6, n1, n8);    // R2
            int variableResistorIndex = addResistor(100.0e+3, n1, n3);    // R3 + R9
            addResistor(100.0e+3, n6, n7);  // R4
            addResistor(100.0e+3, n5, n6);  // R5
            addResistor(100.0e+3, n2, n3);  // R6
            addResistor(100.0e+3, n3, n4);  // R7
            addResistor(470.0e+3, n9, n6);  // R8

            addCapacitor(2.0e-6, n1, n2);   // C1
            addCapacitor(1.0e-6, n4, n5);   // C2
            addCapacitor(50.0e-6, n3, ng);  // C3

            lock();     // Must lock the circuit BEFORE we can access nodes or resistors.

            variableResistance = &resistor(variableResistorIndex).resistance;
            controlVoltage = &nodeVoltage(n9);
            xNodeVoltage = &nodeVoltage(n2);
            yNodeVoltage = &nodeVoltage(n5);
            zNodeVoltage = &nodeVoltage(n7);

            rmsCurrentErrorTolerance = 1.0e-4;
        }

        void setKnobPosition(double fraction)
        {
            // Clamp the fraction to the range [0, 1].
            double clamped = std::max(0.0, std::min(1.0, fraction));

            // The maximum value of the variable resistor is 10K.
            // This is in series with a fixed resistance of 100K.
            *variableResistance = 100.0e+3 + (clamped * 10.0e+3);
        }

        void setControlVoltage(double cv)
        {
            // Clamp the control voltage to the circuit's supply rails.
            *controlVoltage = std::max(VNEG, std::min(VPOS, cv));
        }

        double xVoltage() const
        {
            return *xNodeVoltage;
        }

        double yVoltage() const
        {
            return *yNodeVoltage;
        }

        double zVoltage() const
        {
            return *zNodeVoltage;
        }
    };
}
