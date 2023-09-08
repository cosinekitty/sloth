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
        int ledNodeIndex;
        int variableResistorIndex;

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

            addOpAmp(ng, n7, n8);       // U1
            addOpAmp(ng, n6, n7);       // U2
            addOpAmp(ng, n1, n2);       // U3
            addOpAmp(ng, n4, n5);       // U4

            addResistor(1.0e+6, n1, n7);    // R1
            addResistor(4.7e+6, n1, n8);    // R2
            variableResistorIndex = addResistor(100.0e+3, 1, 3);    // R3
            addResistor(100.0e+3, n6, n7);  // R4
            addResistor(100.0e+3, n5, n6);  // R5
            addResistor(100.0e+3, n2, n3);  // R6
            addResistor(100.0e+3, n3, n4);  // R7

            addCapacitor(2.0e-6, n1, n2);   // C1
            addCapacitor(1.0e-6, n4, n5);   // C2
            addCapacitor(50.0e-6, n3, ng);  // C3

            lock();

            ledNodeIndex = n8;
        }
    };
}
