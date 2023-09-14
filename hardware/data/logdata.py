#!/usr/bin/env python3

import sys
import re
import serial
import time

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('USAGE: logger.py outfile.csv hh:mm:ss')
        sys.exit(1)
    filename = sys.argv[1]
    m = re.match(r'^([0-9\.]+):([0-9\.]+):([0-9\.]+)$', sys.argv[2])
    if not m:
        print('ERROR: not a valid hh:mm:ss duration: "{}"'.format(sys.argv[2]))
        sys.exit(1)
    duration = 3600.0*float(m.group(1)) + 60.0*float(m.group(2)) + float(m.group(3))
    targetTime = None
    try:
        sync = False
        with open(filename, 'wt') as outfile:
            with serial.Serial('/dev/ttyACM0', 115200, timeout=1) as port:
                port.flushInput()
                while (targetTime is None) or (time.time() < targetTime):
                    line = str(port.readline(), 'UTF-8')
                    if not sync:
                        if line.startswith('200,'):
                            sync = True
                            targetTime = time.time() + duration
                            print('Beginning timer for {:0.3f} seconds.'.format(duration))
                    if sync:
                        print(line, end='')
                        outfile.write(line)
                print('Finished logging for {:0.3f} seconds.'.format(duration))
                sys.exit(0)
    except KeyboardInterrupt:
        print('Canceled by user.')
        sys.exit(1)
