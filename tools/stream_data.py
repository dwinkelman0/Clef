#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import datetime
import os
import serial
import sys
import time
import re

import numpy as np


DATAPOINT_RE_STR = "[a-zA-Z]+=-?\d+"
VALID_LINE_RE = re.compile(";({0},)*{0}".format(DATAPOINT_RE_STR))
DATAPOINT_RE = re.compile(DATAPOINT_RE_STR)


parser = argparse.ArgumentParser()
parser.add_argument("--port", type=str, default="/dev/ttyUSB0")
parser.add_argument("--baud", type=int, default=57600)
parser.add_argument("--output-dir", type=str, default="data-{}".format(
    datetime.datetime.now().isoformat()))
parser.add_argument("--time", type=int, default=0)


def parseDatapoint(datapoint):
    parts = datapoint.split("=")
    return parts[0], int(parts[1])


def collect(dirname, port, baud, runtime):
    data = {}
    count = 0

    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(2)
    t0 = time.time()
    try:
        while (runtime > 0 and time.time() < (t0 + runtime)) or runtime == 0:
            message = ser.readline().decode("utf-8").rstrip()
            if VALID_LINE_RE.match(message) is None:
                continue
            datapoints = DATAPOINT_RE.findall(message)
            if parseDatapoint(datapoints[0])[0] != "t":
                continue
            t = parseDatapoint(datapoints[0])[1]
            for datapoint in datapoints[1:]:
                count += 1
                name, value = parseDatapoint(datapoint)
                if name == "t":
                    continue
                if not name in data:
                    data[name] = []
                array = data[name]
                array.append((t, value))
            if count % 500 == 0:
                print("Collected {} samples".format(count))
    except KeyboardInterrupt:
        print()

    if not os.path.exists(dirname):
        os.makedirs(dirname)
    for name, array in data.items():
        fname = "{0}/{1}-vs-t.npy".format(dirname, name)
        print("Saving {0} samples to {1}....".format(len(array), fname))
        np.save(fname, np.array(array))


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])
    collect(args.output_dir, args.port, args.baud, args.time)
