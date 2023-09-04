/*
    circuit.hpp  -  Don Cross  -  2023-09-03

    Generate audio by simulating analog circuits
    made of op-amps, capacitors, and resistors.
*/

#pragma once

#include <memory>
#include <vector>

namespace Analog
{
    struct Node
    {
        float voltage{};    // [volts]
    };


    struct Terminal
    {
        float current{};    // [amps]
    };


    class Component
    {
    public:
        std::vector<Terminal> terminalList;

        Component(std::size_t nterminals)
        {
            terminalList.resize(nterminals);
        }

        virtual ~Component() {}
    };


    using ComponentRef = std::unique_ptr<Component>;


    class Resistor: public Component
    {
    public:
        float resistance;   // [ohms]

        Resistor(float _resistance)
            : Component(2)
            , resistance(_resistance)
        {
        }
    };


    class Capacitor: public Component
    {
    public:
        float capacitance;   // [farads]

        Capacitor(float _capacitance)
            : Component(2)
            , capacitance(_capacitance)
        {
        }
    };


    class OpAmp: public Component
    {
    public:
        OpAmp()
            : Component(3)
        {
        }
    };


    class Circuit
    {
    private:
        std::vector<Node> nodeList;
        std::vector<ComponentRef> componentList;

    public:
    };
}
