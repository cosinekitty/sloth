/*
    torpor_sloth_circuit.hpp  -  Don Cross  -  2023-09-07
*/
#pragma once

#include <algorithm>

namespace Analog
{
    // Power supply rail voltages.
    static const double VNEG = -12.0;
    static const double VPOS = +12.0;

    // Op-amp saturation voltages.
    // I measured these from my breadboard build of Sloth on the comparator U1.
    static const double QNEG = -10.64;
    static const double QPOS = +11.38;

    class TorporSlothCircuit
    {
    private:
        double variableResistance{};    // the total resistance R3 (fixed) + R9 (variable)
        double controlVoltage{};        // control voltage fed into the circuit via R8

        double x1{};    // voltage at the output of op-amp U3
        double w1{};    // voltage at the top of capacitor C3
        double y1{};    // voltage at the output of op-amp U4
        double z1{};    // voltage at the output of op-amp U2

    public:
        TorporSlothCircuit()
        {
            setKnobPosition(0.0);
            setControlVoltage(0.0);
        }

        void setKnobPosition(double fraction)
        {
            // Clamp the fraction to the range [0, 1].
            double clamped = std::max(0.0, std::min(1.0, fraction));

            // The maximum value of the variable resistor is 10K.
            // This is in series with a fixed resistance of 100K.
            variableResistance = 100.0e+3 + (clamped * 10.0e+3);
        }

        void setControlVoltage(double cv)
        {
            // Clamp the control voltage to the circuit's supply rails.
            controlVoltage = std::max(VNEG, std::min(VPOS, cv));
        }

        double xVoltage() const
        {
            return x1;
        }

        double yVoltage() const
        {
            return y1;
        }

        double zVoltage() const
        {
            return z1;
        }

        void update(int sampleRateHz)
        {
            
        }
    };
}
