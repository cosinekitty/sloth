/*
    circuit.hpp  -  Don Cross  -  2023-09-03

    Generate audio by simulating analog circuits
    made of op-amps, capacitors, and resistors.
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <stdexcept>

namespace Analog
{
    const int VOLTAGE_HISTORY = 3;   // the total number of consecutive samples for which each node holds a voltage

    struct Node
    {
        double voltage[VOLTAGE_HISTORY]{};    // voltage[0] = this sample, voltage[1] = previous sample, ... [volts]
        double savedVoltage{};              // temporary scratch-pad for holding pre-mutated voltage [volts]
        double current{};               // net current flowing into the node. must be zero to achieve a solution. [amps]
        double slope{};                 // delta E, where E = sum(current^2); gradient steepness from changing this node's voltage
        bool forcedVoltage = false;     // has a voltage forcer already assigned a required value to this node's voltage?
        bool currentSink = false;       // does this node automatically allow an arbitrary current excess/deficit?
        bool isActiveDeviceInput = false;   // helps validate correct evaluation order of active devices

        void initialize()
        {
            if (!forcedVoltage)
                for (int i = 0; i < VOLTAGE_HISTORY; ++i)
                    voltage[i] = 0;
        }
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
        double current[2];      // [0]=this current, [1]=previous current

        Capacitor(double _capacitance, int _aNodeIndex, int _bNodeIndex)
            : capacitance(_capacitance)
            , aNodeIndex(_aNodeIndex)
            , bNodeIndex(_bNodeIndex)
        {
            initialize();
        }

        void initialize()
        {
            current[0] = current[1] = 0;
        }
    };


    struct LinearAmp    // ideal op-amp with real ground on positive input, and assumed virtual ground on negative input
    {
        // Configuration
        int negNodeIndex;
        int outNodeIndex;

        LinearAmp(int _negNodeIndex, int _outNodeIndex)
            : negNodeIndex(_negNodeIndex)
            , outNodeIndex(_outNodeIndex)
        {
            initialize();
        }

        void initialize()
        {
        }
    };

    const double COMPARATOR_LO_VOLTAGE = -10.64;    // measured from my own Torpor circuit : TL074CN U1 pin 1
    const double COMPARATOR_HI_VOLTAGE = +11.38;    // measured from my own Torpor circuit : TL074CN U1 pin 1

    struct Comparator   // op-amp with positive input grounded, arbitrary negative input, output = +/- binary
    {
        // Configuration
        int negNodeIndex;
        int outNodeIndex;

        // Dynamic state
        double voltage;     // keep voltage stable from previous sample while solving the circuit, for stability

        Comparator(int _negNodeIndex, int _outNodeIndex)
            : negNodeIndex(_negNodeIndex)
            , outNodeIndex(_outNodeIndex)
        {
            initialize();
        }

        void initialize()
        {
            voltage = COMPARATOR_HI_VOLTAGE;
        }
    };


    struct SolutionResult
    {
        int adjustNodeVoltagesCount;
        int currentUpdates;
        double rmsCurrentError;

        SolutionResult(int _adjustNodeVoltagesCount, int _currentUpdates, double _rmsCurrentError)
            : adjustNodeVoltagesCount(_adjustNodeVoltagesCount)
            , currentUpdates(_currentUpdates)
            , rmsCurrentError(_rmsCurrentError)
            {}
    };


    struct PerformanceStats
    {
        long totalAdjustNodeVoltagesCount;
        long totalCurrentUpdates;
        long totalSamples;
        double simulationTimeInSeconds;

        PerformanceStats(long adjustNodeVoltagesCount, long currentUpdates, long samples, double simTime)
            : totalAdjustNodeVoltagesCount(adjustNodeVoltagesCount)
            , totalCurrentUpdates(currentUpdates)
            , totalSamples(samples)
            , simulationTimeInSeconds(simTime)
            {}

        double meanAdjustNodeVoltagesPerSample() const
        {
            return (totalSamples == 0) ? 0.0 : (static_cast<double>(totalAdjustNodeVoltagesCount) / totalSamples);
        }

        double meanCurrentUpdatesPerSample() const
        {
            return (totalSamples == 0) ? 0.0 : (static_cast<double>(totalCurrentUpdates) / totalSamples);
        }
    };


    class Circuit
    {
    private:
        bool isLocked = false;          // must lock the circuit before accessing its components
        std::vector<Node> nodeList;
        std::vector<Resistor> resistorList;
        std::vector<Capacitor> capacitorList;
        std::vector<LinearAmp> linearAmpList;
        std::vector<Comparator> comparatorList;
        long totalAdjustNodeVoltagesCount = 0;
        long totalCurrentUpdates = 0;
        long totalSamples = 0;
        double simulationTime = 0.0;
        double minAlpha = -1.0;     // sentinel value to indicate unknown
        double maxAlpha = -1.0;     // sentinel value to indicate unknown

        int v(int nodeIndex) const
        {
            (void) nodeList.at(nodeIndex);     // Validate the node index. Throw an exception if invalid.
            return nodeIndex;
        }

        void extrapolateUnforcedNodeVoltages()
        {
            // Try to give the solver an initial boost by extrapolating the recent
            // trend in voltages to the next sample.

            for (Node& n : nodeList)
            {
                if (!n.forcedVoltage)
                {
                    double dV = n.voltage[1] - n.voltage[2];
                    n.voltage[0] = n.voltage[1] + dV;
                }
            }
        }

        void debugState() const
        {
            if (debug)
            {
                printf("\n");
                int nn = nodeList.size();
                for (int i = 0; i < nn; ++i)
                {
                    const Node& n = nodeList[i];
                    printf("%c%c node[%d] voltage=%lg, current=%lg\n",
                        n.forcedVoltage ? 'F' : ' ',
                        n.currentSink ? 'S' : ' ',
                        i,
                        n.voltage[0],
                        n.current
                    );
                }
            }
        }

        double updateCurrents(double dt)     // returns sum-of-squares of node current discrepancies
        {
            ++totalCurrentUpdates;

            // No need to do anything here for linearAmpList.
            // All linear amp inputs are virtual grounds.
            // Their outputs are current sinks with unknown voltages to be solved.

            // No need to do anything here for comparatorList.
            // For stability, their binary output voltages are only updated once the solver
            // stabilizes this sample's solution.

            // Add up currents flowing into each node.

            for (Node& n : nodeList)
                n.current = 0;

            // Each resistor current immediately reflects the voltage drop across the resistor.
            for (Resistor& r : resistorList)
            {
                Node& n1 = nodeList.at(r.aNodeIndex);
                Node& n2 = nodeList.at(r.bNodeIndex);
                r.current = (n1.voltage[0] - n2.voltage[0]) / r.resistance;
                n1.current -= r.current;
                n2.current += r.current;
            }

            // Capacitor currents require extrapolation over the time interval.
            for (Capacitor& c : capacitorList)
            {
                Node& n1 = nodeList.at(c.aNodeIndex);
                Node& n2 = nodeList.at(c.bNodeIndex);
                // How much did the voltage across the capacitor change over the time interval?
                double dV = (n1.voltage[0] - n2.voltage[0]) - (n1.voltage[1] - n2.voltage[1]);
                // The change in voltage drop across the capacitor times the capacitance
                // is exactly equal to the total amount of charge that flowed through the
                // capacitor over the time interval. Divide charge by the time increment
                // to obtain the mean current over the entire interval [t, t+dt].
                double meanCurrent = c.capacitance * (dV/dt);
                // Assume the mean current over the time interval is halfway between
                // the previous current (stored in the Capacitor struct) and the new current
                // (the unknown we want to solve for). Solving for the new current, we obtain:
                c.current[0] = 2*meanCurrent - c.current[1];
                n1.current -= c.current[0];
                n2.current += c.current[0];
            }

            // Return the simulation error = sum of squared node currents.
            // Special case: current-sink nodes (ground, amplifier outputs, and forced voltages)
            // act like a single node, but with different voltages. The sum of all their
            // currents must collectively add to zero, in order to preserve the total amount
            // of electric charge in the circuit.

            double score = 0;
            double sink = 0;
            for (const Node& n : nodeList)
                if (n.currentSink)
                    sink += n.current;
                else
                    score += n.current * n.current;

            score += sink * sink;
            return score;
        }

        double adjustNodeVoltages(double dt, bool& halt)
        {
            halt = false;
            ++totalAdjustNodeVoltagesCount;

            // Get the baseline score, before changing any voltages.
            double score0 = updateCurrents(dt);

            // Before doing anything to the current node voltages, save them all.
            // That way we can "rewind" back to the original values as needed.
            for (Node& n : nodeList)
                n.savedVoltage = n.voltage[0];

            // The search space is the vector of all unforced node voltages.
            // Calculate a vector in the direction of steepest descent.
            // This is the negative of the gradient vector.
            // Store the vector in the `slope` fields of the unforced voltage nodes.
            // Approximate the gradient by a tiny change in voltage, up and down,
            // for each unforced voltage node.

            double magnitude = 0;
            for (Node& n : nodeList)
            {
                if (!n.forcedVoltage)
                {
                    n.voltage[0] = n.savedVoltage - deltaVoltage;
                    double nscore = updateCurrents(dt);

                    n.voltage[0] = n.savedVoltage + deltaVoltage;
                    double pscore = updateCurrents(dt);

                    n.voltage[0] = n.savedVoltage;

                    n.slope = (nscore - pscore) / (2 * deltaVoltage);
                    magnitude += n.slope * n.slope;
                }
            }

            if (magnitude == 0.0)
            {
                // We have hit an absolutely flat location on the score space.
                // We cannot make any progress, so quit.
                halt = true;
                return std::sqrt(score0);
            }

            magnitude = std::sqrt(magnitude);
            for (Node& n : nodeList)
                if (!n.forcedVoltage)
                    n.slope /= magnitude;       // convert to dimensionless unit vector

            // Now that we have the unit vector pointing in the direction of steepest descent,
            // we can search for a scale parameter `alpha` that moves us as far as possible while
            // still decreasing the score. This is like golf: we drive the ball as far as possible
            // while still setting us closer to the hole (the solution).
            // Keep track of the minimum and maximum alpha values we ever use.
            // This will help adjust `initAlpha` on a circuit-by-circuit basis,
            // for optimal safe convergence.

            // See document in this repo: theory/armijo_1966.pdf
            // See also: https://en.wikipedia.org/wiki/Backtracking_line_search
            double alpha = initAlpha;
            for(;;)
            {
                for (Node& n : nodeList)
                    if (!n.forcedVoltage)
                        n.voltage[0] = n.savedVoltage + (alpha * n.slope);

                double score1 = updateCurrents(dt);
                if (score1 <= score0 - stepControlParameter*alpha*magnitude)
                {
                    // Maintain debug info about the range of alpha values that were satisfactory.
                    if (minAlpha < 0.0 || alpha < minAlpha)
                        minAlpha = alpha;

                    if (alpha > maxAlpha)       // handles sentinel value maxAlpha = -1 also.
                        maxAlpha = alpha;

                    // Leave voltages adjusted and return the improved rms current error.
                    return std::sqrt(score1);
                }

                // Try again, closer to the starting point.
                alpha *= backtrackParameter;
            }
        }

        void confirmLocked()
        {
            if (!isLocked)
                throw std::logic_error("You must lock the circuit before accessing references to nodes or components.");
        }

        void confirmUnlocked()
        {
            if (isLocked)
                throw std::logic_error("Once the circuit is locked, you cannot add new nodes or components.");
        }

        SolutionResult simulationStep(double simSampleRateHz)
        {
            const double dt = 1.0 / simSampleRateHz;

            // Shift voltage history by one sample.
            // This is needed to calculate capacitor currents, which are based on
            // the rate of change of the voltage across each capacitor: i = C*(dV/dt).
            // It is also used to extrapolate an initial guess for the next voltage
            // on each sample.
            for (Node& node : nodeList)
                for (int i = VOLTAGE_HISTORY-1; i > 0; --i)
                    node.voltage[i] = node.voltage[i-1];

            // Remember the previous capacitor currents to accurately
            // estimate how to update the capacitor currents.
            for (Capacitor& c : capacitorList)
                c.current[1] = c.current[0];

            extrapolateUnforcedNodeVoltages();

            long currentUpdatesBefore = totalCurrentUpdates;

            for (int count = 1; count <= retryLimit; ++count)
            {
                bool halt;
                double rmsCurrentError = adjustNodeVoltages(dt, halt);
                if (halt || rmsCurrentError < rmsCurrentErrorTolerance)
                    return SolutionResult(count, totalCurrentUpdates - currentUpdatesBefore, rmsCurrentError);
            }

            throw std::logic_error(std::string("Circuit solver failed to converge at sample " + std::to_string(totalSamples)));
        }

        void allocateUnforcedCurrentSinkNode(int nodeIndex)
        {
            Node& node = nodeList.at(nodeIndex);
            if (node.forcedVoltage)
                throw std::logic_error("allocateUnforcedCurrentSinkNode: Node voltage was already forced.");
            if (node.currentSink)
                throw std::logic_error("allocateUnforcedCurrentSinkNode: Node was already defined as a current sink.");
            node.currentSink = true;
        }

        void allocateVirtualGroundNode(int nodeIndex)
        {
            Node& node = nodeList.at(nodeIndex);
            if (node.forcedVoltage)
                throw std::logic_error("allocateVirtualGroundNode: Node voltage was already forced.");
            if (node.currentSink)
                throw std::logic_error("allocateVirtualGroundNode: Node was already defined as a current sink.");
            node.forcedVoltage = true;
            node.voltage[0] = node.voltage[1] = 0;
        }

        void updateComparatorOutputs()
        {
            // For simulation stability, allow comparator outputs to change only
            // after we have solved a simulation step. This is essentially a 1-sample
            // slew rate for each comparator. This way, comparator outputs cannot toggle
            // back and forth while we are trying to solve the circuit.
            for (Comparator& k : comparatorList)
            {
                // Each comparator saturates its output voltage based on the negative input voltage.
                const Node& neg = nodeList.at(k.negNodeIndex);
                Node& out = nodeList.at(k.outNodeIndex);
                out.voltage[0] = (neg.voltage[0] < 0) ? COMPARATOR_HI_VOLTAGE : COMPARATOR_LO_VOLTAGE;
            }
        }

    public:
        const double VPOS = +12;       // positive supply voltage fed to all op-amps
        const double VNEG = -12;       // negative supply voltage fed to all op-amps

        bool debug = false;
        double rmsCurrentErrorTolerance = 1.0e-8;     // amps : adjust as necessary for a given circuit, to balance accuracy with convergence
        double deltaVoltage = 1.0e-9;       // step size to try each axis (node) in the search space
        int minInternalSamplingRate = 40000;            // we oversample as needed to reach this minimum sampling rate for the circuit solver
        int retryLimit = 100;
        double initAlpha = 1.0;     // [volts] how far away from previous voltage vector to start the gradient descent search
        double stepControlParameter = 0.5;      // `c` in https://en.wikipedia.org/wiki/Backtracking_line_search
        double backtrackParameter = 0.5;        // `tau` in https://en.wikipedia.org/wiki/Backtracking_line_search

        void lock()
        {
            // Locking the circuit prevents changing it,
            // which allows accessing references to components inside it.
            // This concept was necessary to fix a bug where I returned a
            // reference to a resistor (for example), then added another resistor.
            // The new resistor resized the resistorList, which left the first
            // resistor referencing freed memory!
            isLocked = true;
        }

        void initialize()
        {
            totalAdjustNodeVoltagesCount = 0;
            totalCurrentUpdates = 0;
            totalSamples = 0;
            simulationTime = 0.0;
            minAlpha = maxAlpha = -1;   // sentinel values to indicate unknowns

            for (Resistor& r : resistorList)
                r.initialize();

            for (Capacitor& c : capacitorList)
                c.initialize();

            for (LinearAmp& a : linearAmpList)
                a.initialize();

            for (Comparator& k : comparatorList)
                k.initialize();

            for (Node& n : nodeList)
                n.initialize();
        }

        int createNode()
        {
            confirmUnlocked();
            int index = static_cast<int>(nodeList.size());
            nodeList.push_back(Node());
            return index;
        }

        void allocateForcedVoltageNode(int nodeIndex)
        {
            Node& node = nodeList.at(nodeIndex);
            if (node.forcedVoltage)
                throw std::logic_error("allocateForcedVoltageNode: Node voltage was already forced.");
            if (node.currentSink)
                throw std::logic_error("allocateForcedVoltageNode: Node was already defined as a current sink.");
            node.forcedVoltage = true;
            node.currentSink = true;
        }

        int createForcedVoltageNode(double voltage)
        {
            int nodeIndex = createNode();
            allocateForcedVoltageNode(nodeIndex);
            Node& n = nodeList.at(nodeIndex);
            for (int i = 0; i < VOLTAGE_HISTORY; ++i)
                n.voltage[i] = voltage;
            return nodeIndex;
        }

        int createGroundNode()
        {
            return createForcedVoltageNode(0.0);
        }

        int addResistor(double resistance, int aNodeIndex, int bNodeIndex)
        {
            confirmUnlocked();
            v(aNodeIndex);
            v(bNodeIndex);
            resistorList.push_back(Resistor(resistance, v(aNodeIndex), v(bNodeIndex)));
            return resistorList.size() - 1;
        }

        int addCapacitor(double capacitance, int aNodeIndex, int bNodeIndex)
        {
            confirmUnlocked();
            v(aNodeIndex);
            v(bNodeIndex);
            capacitorList.push_back(Capacitor(capacitance, aNodeIndex, bNodeIndex));
            return capacitorList.size() - 1;
        }

        int addLinearAmp(int negNodeIndex, int outNodeIndex)
        {
            confirmUnlocked();
            v(negNodeIndex);
            v(outNodeIndex);

            // We always calculate op-amp output voltages in the order
            // the op-amps were added to the circuit. Prevent incorrect
            // calculation order by preventing an op-amp from being added
            // to the circuit if its output feeds into the input of another
            // active device that already was added before this one.
            const Node& outNode = nodeList.at(outNodeIndex);
            if (outNode.isActiveDeviceInput)
                throw std::logic_error("Linear amplifier output is not allowed to connect directly to an earlier active device's input.");

            // As a hack, do not allow adding any linear amps once any comparators have been added.
            // This is because we actually calculate all linear amps first, then all comparators.
            if (comparatorList.size() != 0)
                throw std::logic_error("Cannot add a linear amplifier after any comparators have been added.");

            // The linear amp is a little weird. Its output does NOT have a forced voltage.
            // Instead, the output is a current sink only. The voltage is an unknown to be
            // solved, such that the negative input remains a virtual ground.
            allocateUnforcedCurrentSinkNode(outNodeIndex);

            // The negative input is a virtual ground.
            // That means its node voltage is always zero, but the input
            // itself has infinite impedance, and therefore has no effect on node current.
            allocateVirtualGroundNode(negNodeIndex);

            // Prevent any other amplifier to be chained directly and out of calculation order.
            Node& negNode = nodeList.at(negNodeIndex);
            negNode.isActiveDeviceInput = true;

            linearAmpList.push_back(LinearAmp(negNodeIndex, outNodeIndex));
            return linearAmpList.size() - 1;
        }

        int addComparator(int negNodeIndex, int outNodeIndex)
        {
            confirmUnlocked();
            v(negNodeIndex);
            v(outNodeIndex);

            // We always calculate op-amp output voltages in the order
            // the op-amps were added to the circuit. Prevent incorrect
            // calculation order by preventing an op-amp from being added
            // to the circuit if its output feeds into the input of another
            // active device that already was added before this one.
            const Node& outNode = nodeList.at(outNodeIndex);
            if (outNode.isActiveDeviceInput)
                throw std::logic_error("Comparator output is not allowed to connect directly to an earlier active device's input.");

            allocateForcedVoltageNode(outNodeIndex);

            Node& negNode = nodeList.at(negNodeIndex);

            // Prevent any other amplifier to be chained directly and out of calculation order.
            negNode.isActiveDeviceInput = true;

            comparatorList.push_back(Comparator(negNodeIndex, outNodeIndex));
            return comparatorList.size() - 1;
        }

        int getNodeCount() const
        {
            return nodeList.size();
        }

        double getNodeVoltage(int nodeIndex) const
        {
            return nodeList.at(nodeIndex).voltage[0];
        }

        double& nodeVoltage(int nodeIndex)
        {
            confirmLocked();
            return nodeList.at(nodeIndex).voltage[0];
        }

        int getResistorCount() const
        {
            return resistorList.size();
        }

        Resistor& resistor(int index)
        {
            confirmLocked();
            return resistorList.at(index);
        }

        int getCapacitorCount() const
        {
            return capacitorList.size();
        }

        Capacitor& capacitor(int index)
        {
            confirmLocked();
            return capacitorList.at(index);
        }

        int getLinearAmpCount() const
        {
            return linearAmpList.size();
        }

        LinearAmp& linearAmp(int index)
        {
            confirmLocked();
            return linearAmpList.at(index);
        }

        int getComparatorCount() const
        {
            return comparatorList.size();
        }

        Comparator& comparator(int index)
        {
            return comparatorList.at(index);
        }

        SolutionResult update(double audioSampleRateHz)
        {
            if (audioSampleRateHz <= 0.0)
                throw std::range_error("Audio sampling rate must be a positive number.");

            // Calculate the oversampling factor needed to achieve our internal
            // minimum required simulation sampling rate.
            const double realFactor = minInternalSamplingRate / audioSampleRateHz;

            // Round up to the next higher integer.
            // Make absolutely sure the factor is a postive integer.
            const int factor = std::max(1, static_cast<int>(std::ceil(realFactor)));
            const double simSamplingRateHz = factor * audioSampleRateHz;

            SolutionResult result(0, 0, -1.0);
            for (int step = 0; step < factor; ++step)
            {
                if (debug) printf("\nupdate: audio sample %ld, step %d\n", totalSamples, step);
                SolutionResult stepResult = simulationStep(simSamplingRateHz);
                updateComparatorOutputs();
                result.adjustNodeVoltagesCount += stepResult.adjustNodeVoltagesCount;
                result.currentUpdates += stepResult.currentUpdates;
                result.rmsCurrentError = stepResult.rmsCurrentError;
                debugState();
            }

            ++totalSamples;
            simulationTime += (1 / audioSampleRateHz);
            return result;
        }

        PerformanceStats getPerformanceStats() const
        {
            return PerformanceStats(totalAdjustNodeVoltagesCount, totalCurrentUpdates, totalSamples, simulationTime);
        }
    };
}
