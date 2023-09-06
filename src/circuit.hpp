/*
    circuit.hpp  -  Don Cross  -  2023-09-03

    Generate audio by simulating analog circuits
    made of op-amps, capacitors, and resistors.
*/

#pragma once

#include <cmath>
#include <memory>
#include <vector>
#include <stdexcept>

namespace Analog
{
    struct Node
    {
        double prevVoltage{};   // solution for voltage in the previous sample. [volts]
        double voltage{};       // guess/solution for voltage in the current sample. [volts]
        double savedVoltage{};  // temporary scratch-pad for holding pre-mutated voltage [volts]
        double current{};       // net current flowing into the node. must be zero to achieve a solution. [amps]
        double slope{};         // delta E, where E = sum(current^2); gradient steepness from changing this node's voltage
        bool forced = false;    // has a voltage forcer already assigned a required value to this node's voltage?
        // maybe have a solved flag also?
    };


    struct Resistor
    {
        // Configuration
        double resistance;   // [ohms]
        int aNodeIndex;
        int bNodeIndex;

        // Dynamic state
        double current;      // current into the resistor from node A and out from the resistor to node B [amps]

        Resistor(double _resistance, int _aNodeIndex, int _bNodeIndex)
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
        double capacitance;  // [farads]
        int aNodeIndex;
        int bNodeIndex;

        // Dynamic state
        double current;      // [amps]

        Capacitor(double _capacitance, int _aNodeIndex, int _bNodeIndex)
            : capacitance(_capacitance)
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


    struct OpAmp    // Positive input is always connected to ground, so we don't need to model it.
    {
        // Configuration
        int posNodeIndex;
        int negNodeIndex;
        int outNodeIndex;

        // Dynamic state
        double current;    // current leaving the output terminal [amps]

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


    struct SolutionResult
    {
        int iterations;
        double score;

        SolutionResult(int _iterations, double _score)
            : iterations(_iterations)
            , score(_score)
            {}
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
            (void) nodeList.at(nodeIndex);     // Validate the node index. Throw an exception if invalid.
            return nodeIndex;
        }

        double updateCurrents(double dt)     // returns sum-of-squares of node current discrepancies
        {
            // Based on the voltage at each op-amp's input, calculate its output voltage.
            for (OpAmp& o : opAmpList)
            {
                const Node& posNode = nodeList.at(o.posNodeIndex);
                const Node& negNode = nodeList.at(o.negNodeIndex);
                Node& outNode = nodeList.at(o.outNodeIndex);

                outNode.voltage = OPAMP_GAIN * (posNode.voltage - negNode.voltage);
                if (outNode.voltage < VNEG)
                    outNode.voltage = VNEG;
                else if (outNode.voltage > VPOS)
                    outNode.voltage = VPOS;
            }

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
                double dV = (n1.voltage - n2.voltage) - (n1.prevVoltage - n2.prevVoltage);
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
                const Node &n = nodeList.at(o.outNodeIndex);
                o.current = -n.current;
                // Node current will be zeroed out below. No need to do it here also.
            }

            double score = 0;
            for (Node& n : nodeList)
                if (n.forced)
                    n.current = 0;      // Nodes with forced voltages must source/sink arbitrary current.
                else
                    score += n.current * n.current;     // sum of squared currents = simulation error

            return score;
        }

        double adjustNodeVoltages(double dt)
        {
            const double SCORE_TOLERANCE = 1.0e-16;     // amps^2
            const double DELTA_VOLTAGE = 1.0e-9;        // step size = 1 nanovolt

            // Measure the simulation error before changing any unforced node voltages.
            double score1 = updateCurrents(dt);

            // If the score is good enough, consider the update successful.
            if (score1 < SCORE_TOLERANCE)
            {
                // Indicate success and report the score.
                // Take the square root of the sum-of-squares to report
                // an RMS current error value. It is easier to interpret
                // the result when expressed in amps, rather than amps^2.
                return std::sqrt(score1);
            }

            // Calculate partial derivatives of how much each node's voltage
            // affects the simulation error.
            for (Node &n : nodeList)
            {
                if (!n.forced)
                {
                    n.savedVoltage = n.voltage;

                    // Tweak the voltage a tiny amount on this node.
                    // FIXFIXFIX: because of op-amps, slopes may not converge
                    // from both directions when we are near the boundary between
                    // "linear amplifier mode" and "comparator mode".
                    // I might have to try increasing and decreasing the voltage,
                    // pick whichever one reduces the overall simulation error.
                    n.voltage += DELTA_VOLTAGE;

                    // See how much change it makes to the solution score.
                    // We are looking for dE/dV, where E = error and V = voltage.
                    double score2 = updateCurrents(dt);

                    // Store this slope in each unforced node.
                    // We will use them later to update all node voltages to get
                    // closer to the desired solution.
                    n.slope = score2 - score1;

                    // Restore the original voltage.
                    n.voltage = n.savedVoltage;
                }
            }

            // Calculate the derivative of changing the entire system state in
            // the direction indicated by the hypervector that points downhill
            // toward the steepest direction of decreased system error.
            // Normalize the vector such that its magnitude is still DELTA_VOLTAGE.
            double magnitude = 0;
            for (const Node& n : nodeList)
                if (!n.forced)
                    magnitude += n.slope * n.slope;
            magnitude = std::sqrt(magnitude);

            for (Node& n : nodeList)
            {
                if (!n.forced)
                {
                    n.savedVoltage = n.voltage;
                    n.slope *= -DELTA_VOLTAGE / magnitude;  // negative to go "downhill"
                    n.voltage += n.slope;
                }
            }

            double score3 = updateCurrents(dt);

            // We should never lose ground. Otherwise we risk not converging.
            if (score3 >= score1)
            {
                printf("score change = %lg\n", score3 - score1);
                throw std::logic_error("Solver is losing ground.");
            }

            // The naive idea would be to assume everything is linear and that we
            // can extrapolate where we would hit zero error.
            // In reality we have to be more cautious, hence the ALPHA factor.
            // We want to approach the solution quickly, but without oscillating around it
            // or worse, becoming unstable and diverging.
            // We have just stepped DELTA_VOLTAGE along the hypervector.
            // What multiple of that distance would naively bring the error to zero?
            const double ALPHA = 0.6;   // fraction of the way to "jump" toward the naive ideal value

            double step = ALPHA * (score1 / (score1 - score3));
            for (Node& n : nodeList)
                if (!n.forced)
                    n.voltage = n.savedVoltage + (step * n.slope);

            return -1.0;    // indicate failure: negative scores are not possible
        }

    public:
        const double OPAMP_GAIN = 1.0e+6;
        const double VPOS = +12;       // positive supply voltage fed to all op-amps
        const double VNEG = -12;       // negative supply voltage fed to all op-amps

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

        double& allocateForcedVoltageNode(int nodeIndex)
        {
            Node& node = nodeList.at(nodeIndex);
            if (node.forced)
                throw std::logic_error("Node voltage was already forced.");
            node.forced = true;
            return node.voltage;
        }

        int createFixedVoltageNode(double voltage)
        {
            int nodeIndex = createNode();
            double &nodeVoltage = allocateForcedVoltageNode(nodeIndex);
            nodeVoltage = voltage;
            return nodeIndex;
        }

        int createGroundNode()
        {
            return createFixedVoltageNode(0.0);
        }

        Resistor& addResistor(double resistance, int aNodeIndex, int bNodeIndex)
        {
            v(aNodeIndex);
            v(bNodeIndex);
            resistorList.push_back(Resistor(resistance, v(aNodeIndex), v(bNodeIndex)));
            return resistorList.back();
        }

        Capacitor& addCapacitor(double capacitance, int aNodeIndex, int bNodeIndex)
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

        double getNodeVoltage(int nodeIndex) const
        {
            return nodeList.at(nodeIndex).voltage;
        }

        SolutionResult update(double sampleRateHz)
        {
            const double dt = 1.0 / sampleRateHz;

            // Copy latest node voltages into previous node voltages.
            // This is needed to calculate capacitor currents, which are based on
            // the rate of change of the voltage across each capacitor: i = C*(dV/dt).
            for (Node& node : nodeList)
                node.prevVoltage = node.voltage;

            const int limit = 100;
            for (int count = 1; count <= limit; ++count)
            {
                double score = adjustNodeVoltages(dt);
                if (score >= 0.0)
                    return SolutionResult(count, score);
            }

            throw std::logic_error("Circuit solver failed to converge on a solution.");
        }
    };
}
