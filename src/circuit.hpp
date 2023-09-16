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
        double savedVoltage{};  // temporary scratch-pad for holding pre-mutated voltage [volts]
        double current{};       // net current flowing into the node. must be zero to achieve a solution. [amps]
        double slope{};         // delta E, where E = sum(current^2); gradient steepness from changing this node's voltage
        bool forced = false;    // has a voltage forcer already assigned a required value to this node's voltage?

        void initialize()
        {
            if (!forced)
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


    struct OpAmp    // Positive input is always connected to ground, so we don't need to model it.
    {
        // Configuration
        int posNodeIndex;
        int negNodeIndex;
        int outNodeIndex;

        // Dynamic state
        double current;     // current leaving the output terminal [amps]
        double voltage[2];  // low-pass filtered output voltage [0]=this, [1]=prev

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
            voltage[0] = voltage[1] = 0;
        }

        std::string to_string() const
        {
            std::string text = "OpAmp[POS=";
            text += std::to_string(posNodeIndex);
            text += ", NEG=";
            text += std::to_string(negNodeIndex);
            text += ", OUT=";
            text += std::to_string(outNodeIndex);
            text += "]";
            return text;
        }
    };


    struct SolutionResult
    {
        int adjustNodeVoltagesCount;
        int currentUpdates;
        double score;

        SolutionResult(int _adjustNodeVoltagesCount, int _currentUpdates, double _score)
            : adjustNodeVoltagesCount(_adjustNodeVoltagesCount)
            , currentUpdates(_currentUpdates)
            , score(_score)
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
        std::vector<OpAmp> opAmpList;
        long totalAdjustNodeVoltagesCount = 0;
        long totalCurrentUpdates = 0;
        long totalSamples = 0;
        double simulationTime = 0.0;
        double opAmpFilterCoeff = 0.0;

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
                if (!n.forced)
                {
                    double dV = n.voltage[1] - n.voltage[2];
                    n.voltage[0] = n.voltage[1] + dV;
                }
            }
        }

        double updateCurrents(double dt, const char *comment)     // returns sum-of-squares of node current discrepancies
        {
            ++totalCurrentUpdates;

            // Based on the voltage at each op-amp's input, calculate its output voltage.
            // Apply low-pass filtering to simulate a finite slew rate.
            // This is necessary to achieve convergence to a solution.
            if (debug) printf("\nupdateCurrents(%s)\n", comment);
            for (OpAmp& o : opAmpList)
            {
                const Node& posNode = nodeList.at(o.posNodeIndex);
                const Node& negNode = nodeList.at(o.negNodeIndex);
                Node& outNode = nodeList.at(o.outNodeIndex);

                // Calculate the instantaneous output voltage.
                // That is, the voltage the op-amp *would* present if its
                // slew rate were infinite.
                double vfast = opAmpOpenLoopGain * (posNode.voltage[0] - negNode.voltage[0]);
                if (vfast < VNEG)
                    vfast = VNEG;
                else if (vfast > VPOS)
                    vfast = VPOS;

                // Feed the rapid changes of output voltage through a low-pass slew filter.
                o.voltage[0] = (opAmpFilterCoeff * o.voltage[1]) + ((1-opAmpFilterCoeff) * vfast);

                // Copy the filtered voltage into the forced-voltage output node.
                outNode.voltage[0] = o.voltage[0];

                if (debug)
                {
                    printf("   opamp: POS[%d] = %lg V, NEG[%d] = %lg V, OUT[%d] = %lg V\n",
                        o.posNodeIndex, posNode.voltage[0],
                        o.negNodeIndex, negNode.voltage[0],
                        o.outNodeIndex, outNode.voltage[0]
                    );
                }
            }

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

            if (debug)
            {
                printf("\n");
                int nn = nodeList.size();
                for (int i = 0; i < nn; ++i)
                {
                    const Node& n = nodeList[i];
                    if (!n.forced)
                        printf("   node[%d] voltage=%lg, current=%lg\n", i, n.voltage[0], n.current);
                }
                for (int i = 0; i < nn; ++i)
                {
                    const Node& n = nodeList[i];
                    if (n.forced)
                        printf("F  node[%d] voltage=%lg, current=%lg\n", i, n.voltage[0], n.current);
                }
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
            ++totalAdjustNodeVoltagesCount;

            // Measure the simulation error before changing any unforced node voltages.
            double score1 = updateCurrents(dt, "before voltage adjust");
            if (debug) printf("\nadjustNodeVoltages: sqrt(score1) = %lg\n", std::sqrt(score1));

            // If the score is good enough, consider the update successful.
            if (score1 < scoreTolerance*scoreTolerance)
            {
                // Indicate success and report the score.
                // Take the square root of the sum-of-squares to report
                // an RMS current error value. It is easier to interpret
                // the result when expressed in amps, rather than amps^2.
                return std::sqrt(score1);
            }

            Node *fallbackNode = nullptr;
            double fallbackScore = score1;
            double fallbackAdjust = 0.0;

            // Calculate partial derivatives of how much each node's voltage
            // affects the simulation error.
            // At the same time, look for a fallback gradient along a single axis,
            // in case the all-axis gradient fails.
            double magnitude = 0.0;
            for (Node &n : nodeList)
            {
                if (!n.forced)
                {
                    n.savedVoltage = n.voltage[0];

                    // We are looking for dE/dV, where E = error and V = voltage.
                    // Tweak the voltage a tiny amount on this node.
                    // To get better partial derivatives, we do the extra work
                    // of increasing and decreasing the voltage, evaluating the currents
                    // at both, and drawing a secant line that approximates the tangent line.
                    n.voltage[0] += deltaVoltage;
                    double score2p = updateCurrents(dt, "partial derivative +");

                    if (score2p < fallbackScore)
                    {
                        fallbackNode = &n;
                        fallbackScore = score2p;
                        fallbackAdjust = +deltaVoltage;
                    }

                    n.voltage[0] = n.savedVoltage - deltaVoltage;
                    double score2n = updateCurrents(dt, "partial derivative -");

                    if (score2n < fallbackScore)
                    {
                        fallbackNode = &n;
                        fallbackScore = score2n;
                        fallbackAdjust = -deltaVoltage;
                    }

                    // Store this slope in each unforced node.
                    // We will use them later to update all node voltages to get
                    // closer to the desired solution.
                    n.slope = score2p - score2n;

                    // Accumulate the magnitude of the gradient vector so we can normalize it later.
                    magnitude += n.slope * n.slope;

                    // Restore the original voltage.
                    n.voltage[0] = n.savedVoltage;

                    if (debug) printf("\nadjustNodeVoltages: sqrt(score2p) = %lg, sqrt(score2n) = %lg, slope = %lg\n", std::sqrt(score2p), std::sqrt(score2n), n.slope);
                }
            }

            if (magnitude == 0.0)
                throw std::logic_error("Solver cannot make progress because gradient vector is zero.");

            // Calculate the derivative of changing the entire system state in
            // the direction indicated by the hypervector that points downhill
            // toward the steepest direction of decreased system error.
            // Normalize the vector such that its magnitude is still deltaVoltage.
            magnitude = std::sqrt(magnitude);
            if (debug) printf("magnitude = %lg\n", magnitude);

            for (Node& n : nodeList)
            {
                if (!n.forced)
                {
                    n.savedVoltage = n.voltage[0];
                    n.slope *= -deltaVoltage / magnitude;  // negative to go "downhill" : gradient descent
                    n.voltage[0] += n.slope;
                }
            }

            double score3 = updateCurrents(dt, "after voltage micro-adjustment");
            if (debug) printf("\nadjustNodeVoltages: sqrt(score3) = %lg\n", std::sqrt(score3));

            // We should never lose ground. Otherwise we risk not converging.
            if (score3 >= score1)
            {
                if (fallbackNode == nullptr)
                {
                    // Try as we might, we can't find a way to go downhill from here!
                    // Let's consider this successful finding of a local minimum.
                    // Revert the changes to the unforced node voltages.
                    for (Node& n : nodeList)
                        if (!n.forced)
                            n.voltage[0] = n.savedVoltage;

                    return std::sqrt(score1);
                }

                // See if we can fall back to a single-axis change that improves the score.
                if (debug) printf("using fallback\n");

                // Reset all node voltages.
                for (Node& n : nodeList)
                {
                    if (!n.forced)
                    {
                        n.voltage[0] = n.savedVoltage;
                        n.slope = 0.0;
                    }
                }

                // Prepare to step in the new direction, along this single axis.
                fallbackNode->slope = fallbackAdjust;
            }

            // We have just micro-stepped by `deltaVoltage` along the descending gradient,
            // and `score3` is confirmed to be an improvement at that micro-step.
            // Perform a "line search" along that direction, to find a rough minimum along it:
            // https://en.wikipedia.org/wiki/Line_search
            // Look farther away than deltaVoltage and keep going until we find a score
            // that is no longer better than score1.
            // Then we have a bracket in which we can search for a score near the minimum.

            char comment[100];
            comment[0] = '\0';      // in case debugging is disabled

            const double dilation = 2.0;
            double bestAlpha = 1.0;
            double bestScore = score3;
            double lineScore = -1.0;        // make sure we enter the `for` loop at least one time
            for (double alpha = dilation; lineScore < score1; alpha *= dilation)
            {
                voltageStep(alpha);

                if (debug) snprintf(comment, sizeof(comment), "upper bracket alpha = %lg", alpha);
                lineScore = updateCurrents(dt, comment);
                if (debug) printf("upper bracket score = %lg\n", lineScore);
                if (lineScore < bestScore)
                {
                    bestScore = lineScore;
                    bestAlpha = alpha;
                }
            }

            // Perform an iterative search over the open range (bestAlpha/dilation, bestAlpha*dilation)
            // for the best alpha and score we can find.

            const int nsteps = 9;
            double loAlpha = bestAlpha / dilation;
            double hiAlpha = bestAlpha * dilation;
            for (int step = 1; step < nsteps; ++step)   // iterate through interior points
            {
                double alpha = loAlpha + (step * (hiAlpha - loAlpha))/nsteps;
                voltageStep(alpha);
                if (debug) snprintf(comment, sizeof(comment), "linear step alpha = %lg", alpha);
                lineScore = updateCurrents(dt, comment);
                if (debug) printf("linear step score = %lg\n", lineScore);
                if (lineScore < bestScore)
                {
                    bestScore = lineScore;
                    bestAlpha = alpha;
                }
            }

            if (debug) printf("line search bestAlpha=%lg, bestScore=%lg, improvement=%lg\n", bestAlpha, bestScore, score1-bestScore);

            // Step to the best distance we found.
            voltageStep(bestAlpha);

            return -1.0;    // indicate the search is incomplete: we made progress, but not good enough yet
        }

        void voltageStep(double step)
        {
            for (Node& n : nodeList)
                if (!n.forced)
                    n.voltage[0] = n.savedVoltage + (step * n.slope);
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
            double score;

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

            // Update output voltages on the op-amps to reflect their new low-pass filtered state.
            for (OpAmp& o : opAmpList)
                o.voltage[1] = o.voltage[0];

            extrapolateUnforcedNodeVoltages();

            long currentUpdatesBefore = totalCurrentUpdates;

            for (int count = 1; count <= retryLimit; ++count)
            {
                if (debug) printf("simulationStep: count = %d\n", count);
                score = adjustNodeVoltages(dt);
                if (score >= 0.0)
                    return SolutionResult(count, totalCurrentUpdates - currentUpdatesBefore, score);
            }

            //throw std::logic_error(std::string("Circuit solver failed to converge at sample " + std::to_string(totalSamples)));
            // Just assume we have gotten as good a solution as is possible.
            score = updateCurrents(dt, "exhausted retry limit");
            return SolutionResult(retryLimit, totalCurrentUpdates - currentUpdatesBefore, std::sqrt(score));
        }

    public:
        const double VPOS = +12;       // positive supply voltage fed to all op-amps
        const double VNEG = -12;       // negative supply voltage fed to all op-amps

        bool debug = false;
        double scoreTolerance = 1.0e-8;     // amps : adjust as necessary for a given circuit, to balance accuracy with convergence
        double deltaVoltage = 1.0e-9;       // step size to try each axis (node) in the search space
        int minInternalSamplingRate = 40000;            // we oversample as needed to reach this minimum sampling rate for the circuit solver
        double opAmpSlewRateHalfLifeSeconds = 0.0;      // when positive, enables low-pass filter on op-amp outputs to make the solver converge easily
        double opAmpOpenLoopGain = 1.0e+6;
        int retryLimit = 100;

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
            totalSamples = 0;
            simulationTime = 0.0;

            for (Resistor& r : resistorList)
                r.initialize();

            for (Capacitor& c : capacitorList)
                c.initialize();

            for (OpAmp& o : opAmpList)
                o.initialize();

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
            if (node.forced)
                throw std::logic_error("Node voltage was already forced.");
            node.forced = true;
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

        int addOpAmp(int posNodeIndex, int negNodeIndex, int outNodeIndex)
        {
            confirmUnlocked();
            v(posNodeIndex);
            v(negNodeIndex);

            OpAmp newOpAmp(posNodeIndex, negNodeIndex, outNodeIndex);

            // We always calculate op-amp output voltages in the order
            // the op-amps were added to the circuit. Prevent incorrect
            // calculation order by preventing an op-amp from being added
            // to the circuit if its output feeds into either of an existing
            // op-amp's inputs.
            const int n = opAmpList.size();
            for (int i = 0; i < n; ++i)
            {
                const OpAmp& o = opAmpList[i];
                if (o.negNodeIndex == outNodeIndex || o.posNodeIndex == outNodeIndex)
                {
                    using namespace std;
                    string message = newOpAmp.to_string();
                    message += " would calculate after op-amp ";
                    message += o.to_string();
                    message += ", causing incorrect voltage to be received by the second op-amp.";
                    throw std::logic_error(message);
                }
            }

            allocateForcedVoltageNode(outNodeIndex);
            opAmpList.push_back(newOpAmp);
            return opAmpList.size() - 1;
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

        int getOpAmpCount() const
        {
            return opAmpList.size();
        }

        OpAmp& opAmp(int index)
        {
            confirmLocked();
            return opAmpList.at(index);
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

            if (!opAmpList.empty() && (opAmpSlewRateHalfLifeSeconds > 0.0))
            {
                double prev = opAmpFilterCoeff;

                // Calculate the lowpass filter coefficient necessary to achieve
                // the desired half-life slew response from op-amps.
                opAmpFilterCoeff = std::pow(0.5, 1.0 / (simSamplingRateHz * opAmpSlewRateHalfLifeSeconds));

                if (debug && (prev != opAmpFilterCoeff))
                    printf("\nupdate: opAmpFilterCoeff = %0.16lf\n", opAmpFilterCoeff);
            }
            else
            {
                // There are no op-amps, or we want to completely disable low-pass filtering.
                opAmpFilterCoeff = 0.0;
            }

            SolutionResult result(0, 0, -1.0);
            for (int step = 0; step < factor; ++step)
            {
                if (debug) printf("\nupdate: audio sample %ld, step %d\n", totalSamples, step);
                SolutionResult stepResult = simulationStep(simSamplingRateHz);
                result.adjustNodeVoltagesCount += stepResult.adjustNodeVoltagesCount;
                result.currentUpdates += stepResult.currentUpdates;
                result.score = stepResult.score;
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
