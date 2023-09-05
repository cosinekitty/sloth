/*
    circuit.hpp  -  Don Cross  -  2023-09-03

    Generate audio by simulating analog circuits
    made of op-amps, capacitors, and resistors.
*/

#pragma once

#include <memory>
#include <vector>
#include <stdexcept>

namespace Analog
{
    struct Node
    {
        float voltage{};        // guess/solution for voltage in the current sample. [volts]
        float prevVoltage{};    // solution for voltage in the previous sample. [volts]
        float current{};        // net current flowing into the node. must be zero to achieve a solution. [amps]
        bool forced = false;    // has a voltage forcer already assigned a required value to this node's voltage?
        // maybe have a solved flag also?
    };


    struct Resistor
    {
        // Configuration
        float resistance;   // [ohms]
        int aNodeIndex;
        int bNodeIndex;

        // Dynamic state
        float current;      // current into the resistor from node A and out from the resistor to node B [amps]

        Resistor(float _resistance, int _aNodeIndex, int _bNodeIndex)
            : resistance(_resistance)
            , aNodeIndex(_aNodeIndex)
            , bNodeIndex(_bNodeIndex)
        {
            initialize();
        }

        void initialize()
        {
            current = 0;
        }
    };


    struct Capacitor
    {
        // Configuration
        float capacitance;  // [farads]
        int aNodeIndex;
        int bNodeIndex;

        // Dynamic state
        float current;      // [amps]
        float drop;         // potential drop based on current charge level [volts]

        Capacitor(float _capacitance, int _aNodeIndex, int _bNodeIndex)
            : capacitance(_capacitance)
            , aNodeIndex(_aNodeIndex)
            , bNodeIndex(_bNodeIndex)
        {
            initialize();
        }

        void initialize()
        {
            current = 0;
            drop = 0;
        }
    };


    struct OpAmp    // Positive input is always connected to ground, so we don't need to model it.
    {
        // Configuration
        int posNodeIndex;
        int negNodeIndex;
        int outNodeIndex;

        // Dynamic state
        float current;    // current leaving the output terminal [amps]

        OpAmp(int _posNodeIndex, int _negNodeIndex, int _outNodeIndex)
            : posNodeIndex(_posNodeIndex)
            , negNodeIndex(_negNodeIndex)
            , outNodeIndex(_outNodeIndex)
        {
            initialize();
        }

        void initialize()
        {
            current = 0;
        }
    };


    class Circuit
    {
    private:
        std::vector<Node> nodeList;
        std::vector<Resistor> resistorList;
        std::vector<Capacitor> capacitorList;
        std::vector<OpAmp> opAmpList;

        int v(int nodeIndex) const
        {
            nodeList.at(nodeIndex);     // Validate the node index. Throw an exception if invalid.
            return nodeIndex;
        }

        void sumNodeCurrents(float dt)
        {
            // Add up currents flowing into each node.

            for (Node& n : nodeList)
                n.current = 0;

            // Each resistor current immediately reflects the voltage drop across the resistor.
            for (Resistor& r : resistorList)
            {
                Node& n1 = nodeList.at(r.aNodeIndex);
                Node& n2 = nodeList.at(r.bNodeIndex);
                r.current = (n1.voltage - n2.voltage) / r.resistance;
                n1.current -= r.current;
                n2.current += r.current;
            }

            // Capacitor current: i = C (dV/dt)
            for (Capacitor& c : capacitorList)
            {
                Node& n1 = nodeList.at(c.aNodeIndex);
                Node& n2 = nodeList.at(c.bNodeIndex);
                float dV = (n1.voltage - n2.voltage) - (n1.prevVoltage - n2.prevVoltage);
                c.current = c.capacitance * (dV/dt);
                n1.current -= c.current;
                n2.current += c.current;
            }

            // Fixed voltage nodes must source/sink any amount of current discrepancy.
            // For example, a ground node must allow any excess current to flow into it.
            // An op-amp output must do the same.
            // This relieves pressure on the solver to focus on adjusting voltages
            // only on the nodes for which it is *possible* to adjust voltages,
            // i.e. the unforced nodes.

            // Store the sourced current coming out of op-amp outputs.
            for (OpAmp& o : opAmpList)
            {
                Node &n = nodeList.at(o.outNodeIndex);
                o.current = -n.current;
                // Node current will be zeroed out below. No need to do it here also.
            }

            for (Node& n : nodeList)
                if (n.forced)
                    n.current = 0;
        }

        void updateOpAmpOutputVoltages()
        {
            // Based on the voltage at the op-amp's input, calculate its output voltage.
            for (OpAmp& o : opAmpList)
            {
                Node& posNode = nodeList.at(o.posNodeIndex);
                Node& negNode = nodeList.at(o.negNodeIndex);
                Node& outNode = nodeList.at(o.outNodeIndex);

                outNode.voltage = OPAMP_GAIN * (posNode.voltage - negNode.voltage);
                if (outNode.voltage < VNEG)
                    outNode.voltage = VNEG;
                else if (outNode.voltage > VPOS)
                    outNode.voltage = VPOS;
            }
        }

        void adjustNodeVoltages()
        {
            // Check the net currents for each unforced node.
            // If there is excess current flowing into a node, pretend like
            // positive charge is "piling up" there, resulting in an increase of
            // voltage there. Over a few iterations, we want this to result in
            // all the free node currents to converge to zero.
        }

        bool refine(float dt)
        {
            sumNodeCurrents(dt);
            updateOpAmpOutputVoltages();
            adjustNodeVoltages();
            return true;
        }

    public:
        const float OPAMP_GAIN = 1.0e+6f;
        const float VPOS = +12;       // positive supply voltage fed to all op-amps
        const float VNEG = -12;       // negative supply voltage fed to all op-amps

        void initialize()
        {
            for (Resistor& r : resistorList)
                r.initialize();

            for (Capacitor& c : capacitorList)
                c.initialize();

            for (OpAmp& o : opAmpList)
                o.initialize();
        }

        int createNode()
        {
            int index = static_cast<int>(nodeList.size());
            nodeList.push_back(Node());
            return index;
        }

        float& allocateForcedVoltageNode(int nodeIndex)
        {
            Node& node = nodeList.at(nodeIndex);
            if (node.forced)
                throw std::logic_error("Node voltage was already forced.");
            node.forced = true;
            return node.voltage;
        }

        int createFixedVoltageNode(float voltage)
        {
            int nodeIndex = createNode();
            float &nodeVoltage = allocateForcedVoltageNode(nodeIndex);
            nodeVoltage = voltage;
            return nodeIndex;
        }

        int createGroundNode()
        {
            return createFixedVoltageNode(0.0f);
        }

        Resistor& addResistor(float resistance, int aNodeIndex, int bNodeIndex)
        {
            v(aNodeIndex);
            v(bNodeIndex);
            resistorList.push_back(Resistor(resistance, v(aNodeIndex), v(bNodeIndex)));
            return resistorList.back();
        }

        Capacitor& addCapacitor(float capacitance, int aNodeIndex, int bNodeIndex)
        {
            v(aNodeIndex);
            v(bNodeIndex);
            capacitorList.push_back(Capacitor(capacitance, aNodeIndex, bNodeIndex));
            return capacitorList.back();
        }

        OpAmp& addOpAmp(int posNodeIndex, int negNodeIndex, int outNodeIndex)
        {
            v(posNodeIndex);
            v(negNodeIndex);
            allocateForcedVoltageNode(outNodeIndex);
            opAmpList.push_back(OpAmp(posNodeIndex, negNodeIndex, outNodeIndex));
            return opAmpList.back();
        }

        int getNodeCount() const
        {
            return nodeList.size();
        }

        float getNodeVoltage(int nodeIndex) const
        {
            return nodeList.at(nodeIndex).voltage;
        }

        void update(float sampleRateHz)
        {
            const float dt = 1 / sampleRateHz;

            // Copy latest node voltages into previous node voltages.
            // This is needed to calculate capacitor currents, which are based on
            // the rate of change of the volage across each capacitor: i = C*(dV/dt).
            for (Node& node : nodeList)
                node.prevVoltage = node.voltage;

            const int limit = 100;
            for (int count = 0; count < limit; ++count)
                if (refine(dt))
                    return;

            throw std::logic_error("Circuit solver failed to converge on a solution.");
        }
    };
}
