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
        float voltage{};        // [volts]
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
        int negNodeIndex;
        int outNodeIndex;

        // Dynamic state
        float current;    // current leaving the output terminal [amps]

        OpAmp(int _negNodeIndex, int _outNodeIndex)
            : negNodeIndex(_negNodeIndex)
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

    public:
        const float vpos = +12;       // positive supply voltage fed to all op-amps
        const float vneg = -12;       // negative supply voltage fed to all op-amps

        int createNode()
        {
            int index = static_cast<int>(nodeList.size());
            nodeList.push_back(Node());
            return index;
        }

        void initialize()
        {
            for (Resistor& r : resistorList)
                r.initialize();

            for (Capacitor& c : capacitorList)
                c.initialize();

            for (OpAmp& o : opAmpList)
                o.initialize();
        }

        float& allocateForcedVoltageNode(int nodeIndex)
        {
            Node& node = nodeList.at(nodeIndex);
            if (node.forced)
                throw std::logic_error("Node voltage was already forced.");
            node.forced = true;
            return node.voltage;
        }

        void setForcedVoltageNode(int nodeIndex, float voltage)
        {
            allocateForcedVoltageNode(nodeIndex) = voltage;
        }

        void setGroundNode(int nodeIndex)
        {
            setForcedVoltageNode(nodeIndex, 0.0f);
        }

        Resistor& addResistor(float resistance, int aNodeIndex, int bNodeIndex)
        {
            resistorList.push_back(Resistor(resistance, aNodeIndex, bNodeIndex));
            return resistorList.back();
        }

        OpAmp& addOpAmp(int negNodeIndex, int outNodeIndex)
        {
            opAmpList.push_back(OpAmp(negNodeIndex, outNodeIndex));
            return opAmpList.back();
        }

        float getNodeVoltage(int nodeIndex) const
        {
            return nodeList.at(nodeIndex).voltage;
        }

        void solve()
        {
        }
    };
}
