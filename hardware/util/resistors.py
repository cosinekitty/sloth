#/usr/bin/env python3
#
#   resistors.py  -  Don Cross  -  2023-09-12
#
#   Choose the best available resistors to convert
#   a voltage in the range [-12V, +12V] to fit within
#   the range [0V, +5V], so that it can be read by an Arduino.
#
import sys
from typing import List

VLO = -12.0
VHI = +12.0

def OutputVoltage(x:float, Q:float, R:float, S:float) -> float:
    return (x/Q + 5.0/S) / (1.0/Q + 1.0/R + 1.0/S)

def Score(Q:float, R:float, S:float) -> float:
    # Calculate the extreme values of the output from the extreme
    # input voltages that can be fed into the converter.
    y1 = OutputVoltage(VLO, Q, R, S)
    y2 = OutputVoltage(VHI, Q, R, S)
    if not(0.0 <= y1 <= 5.0) or not(0.0 <= y2 <= 5.0):
        # Not a valid solution: the Arduino could be damaged.
        return -1.0
    # The score is proportional to how wide the output range is.
    # The best possible score would be 5.0.
    return y2 - y1


def FindResistors() -> int:
    availableResistors: List[float] = [
        470.0, 510.0, 560.0, 680.0,
        820.0, 1000.0, 1200.0, 1500.0, 1800.0, 2200.0,
        2700.0, 3000.0, 3300.0, 3900.0, 4700.0, 5100.0,
        5600.0, 6800.0, 8200.0, 10000.0
    ]
    solution = None
    bestScore = -2.0
    for Q in availableResistors:
        for R in availableResistors:
            for S in availableResistors:
                score = Score(Q, R, S)
                if score > bestScore:
                    bestScore = score
                    solution = (Q, R, S)

    if solution is None:
        print('CANNOT SOLVE')
        return 1
    Q, R, S = solution
    print('Q = {:0.0f}, R = {:0.0f}, S = {:0.0f}'.format(Q, R, S))
    y1 = OutputVoltage(VLO, Q, R, S)
    y2 = OutputVoltage(VHI, Q, R, S)
    s1 = round((y1 / 5.0) * 1023)
    s2 = round((y2 / 5.0) * 1023)
    print('When x = {:0.1f}V, y = {:0.6f}, reading = {:4d}'.format(VLO, y1, s1))
    print('When x = {:0.1f}V, y = {:0.6f}, reading = {:4d}'.format(VHI, y2, s2))
    return 0

if __name__ == '__main__':
    sys.exit(FindResistors())
