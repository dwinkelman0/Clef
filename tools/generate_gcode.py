#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import os
import sys

import numpy as np


parser = argparse.ArgumentParser()
parser.add_argument("--output", type=str, required=True)
subparsers = parser.add_subparsers(dest="command")

extrude_parser = subparsers.add_parser("extrude")
extrude_parser.add_argument("--distance", type=float, default=30)
extrude_parser.add_argument("--min-feedrate", type=float, default=50)
extrude_parser.add_argument("--max-feedrate", type=float, default=200)
extrude_parser.add_argument("--num-segments", type=int, default=10)
extrude_parser.add_argument("--num-iterations", type=int, default=1)

impulse_parser = subparsers.add_parser("impulse")
impulse_parser.add_argument("--distance", type=float, default=10)
impulse_parser.add_argument("--feedrate", type=float, default=200)
impulse_parser.add_argument("--duty-cycle", type=float, default=0.5)
impulse_parser.add_argument("--num-iterations", type=int, default=1)


def generateExtrude(args):
    output = []
    fs = np.logspace(
        np.log10(args.min_feedrate),
        np.log10(args.max_feedrate),
        args.num_segments)
    total = 0
    for i in range(args.num_iterations):
        for f in np.concatenate((fs, np.flip(fs))):
            total += f
            output.append("G1 E{:.3f} F{:.3f}".format(
                args.distance / 2 * total / sum(fs), f))
    return output


def generateImpulse(args):
    output = []
    impulseTime = args.distance * 60 / args.feedrate
    delayTime = impulseTime / args.duty_cycle * (1 - args.duty_cycle)
    delayFeedrate = 10 / delayTime * 6
    total = 0
    currentX = 0
    for i in range(args.num_iterations):
        total += args.distance
        output.append("G1 E{:.3f} F{:.3f}".format(total, args.feedrate))
        currentX = 0 if currentX == 10 else 10
        output.append("G1 X{:.3f} F{:.3f}".format(currentX, delayFeedrate))
    return output


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])
    if args.command == "extrude":
        output = generateExtrude(args)
    elif args.command == "impulse":
        output = generateImpulse(args)

    fname = os.path.abspath(args.output)
    dirname = os.path.dirname(fname)
    if not os.path.exists(dirname):
        os.makedirs(dirname)
    with open(fname, "w") as outputFile:
        for line in output:
            outputFile.write(line + "\n")
