#!/usr/bin/env python3

import sys
import serial

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('USAGE: logger.py outfile.csv')
        sys.exit(1)
    filename = sys.argv[1]
    try:
        sync = False
        with open(filename, 'wt') as outfile:
            with serial.Serial('/dev/ttyACM0', 115200, timeout=1) as port:
                port.flushInput()
                while True:
                    line = str(port.readline(), 'UTF-8')
                    if not sync:
                        if line.startswith('200,'):
                            sync = True
                    if sync:
                        print(line, end='')
                        outfile.write(line)
    except KeyboardInterrupt:
        print('Exiting...')
        sys.exit(0)
