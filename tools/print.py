#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import os
import serial
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument("file")
parser.add_argument("--port", type=str, default="/dev/ttyACM0")
parser.add_argument("--baud", type=int, default=57600)


def printFromFile(fname, port, baud):
    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(2)
    with open(fname, "r") as inputFile:
        isFirstLine = True
        for line in inputFile:
            print("> {}:".format(line[:-1]).ljust(60, "."), end="")
            success = False
            timeout = 0.001
            resend = True
            while not success:
                if resend:
                    ser.write(line.encode('utf-8'))
                resend = False
                time.sleep(0.001)
                message = ""
                while message == "":
                    message = ser.readline().decode("utf-8").rstrip()
                if message == "ok":
                    success = True
                    isFirstLine = False
                    print("ok")
                elif message == "alloc_error":
                    time.sleep(timeout)
                    timeout *= 2
                    timeout = min(timeout, 0.1)
                    print(message)
                    print("".ljust(60, " "), end="")
                    resend = True
                elif message[0] == ";":
                    print(message)
                    print("".ljust(60, " "), end="")
                elif not isFirstLine:
                    print(message)
                    print("Encountered error, exiting!")
                    return
        print("Done printing!")
        while True:
            for line in ser:
                if line[0] == ";":
                    print(line)
                else:
                    print("Extra line: {}".format(line))


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])
    printFromFile(args.file, args.port, args.baud)
