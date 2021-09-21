#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import datetime
import os
import re
import sys

import numpy as np
import matplotlib.pyplot as plt


DATA_DEFAULT_DIRNAME_RE = re.compile(
    "data-\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d\.\d\d\d\d\d\d")
DATA_FILENAME_RE = re.compile("[a-zA-Z]+-vs-[a-zA-Z]+\.npy")


DEFAULT_INPUT_DIR = max(
    [name for name in os.listdir(".") if os.path.isdir(name) and DATA_DEFAULT_DIRNAME_RE.match(name)])


parser = argparse.ArgumentParser()
parser.add_argument("--input-dir", type=str, default=DEFAULT_INPUT_DIR)


VARIABLES = {
    "t": {
        "name": "Time",
        "type": "Time",
        "normalize": lambda array: array / 1e6,
        "units": "seconds",
    },
    "P": {
        "name": "Pressure",
        "type": "Pressure",
        "normalize": lambda array: array - array[0],
        "units": "Pressure units",
    },
    "xe": {
        "name": "Stepper Motor Position",
        "type": "Displacement",
        "normalize": lambda array: array - array[0],
        "units": "E-Axis usteps",
    },
    "xs": {
        "name": "Syringe Plunger Position",
        "type": "Displacement",
        "normalize": lambda array: array - array[0],
        "units": "E-Axis usteps",
    },
    "deltax": {
        "name": "Displacement Difference",
        "type": "Displacement",
        "normalize": lambda array: array - array[0],
        "units": "E-Axis usteps",
    },
}


def normalize(names, array):
    return np.column_stack((
        VARIABLES[names[0]]["normalize"](array[:, 0]),
        VARIABLES[names[1]]["normalize"](array[:, 1])
    ))


def normalizeY(names, array):
    return np.column_stack((
        array[:, 0],
        VARIABLES[names[1]]["normalize"](array[:, 1])
    ))


def interp(*arrays):
    ts = np.concatenate([array[:, 0] for array in arrays])
    ts.sort()
    ts = np.unique(ts)
    return (ts,) + tuple((np.interp(ts, array[:, 0], array[:, 1]) for array in arrays))


def difference(array1, array2):
    ts, interp1, interp2 = interp(array1, array2)
    return np.column_stack((ts, interp1 - interp2))


def join(data1, data2):
    (xname1, yname1), array1 = data1
    (xname2, yname2), array2 = data2
    ts, interp1, interp2 = interp(array1, array2)
    return (
        (data1[0][1], data2[0][1]),
        np.column_stack((interp1, interp2)))


def lowpassFilter(array, a):
    average = 0
    output = array.copy()
    for i in range(array.shape[0]):
        average = average * (1 - a) + array[i][1] * a
        output[i][1] = average
    return output


def derivative(data):
    (xname, yname), array = data
    newYname = "d{}d{}".format(yname, xname)
    VARIABLES[newYname] = {
        "name": "{} Derivative".format(VARIABLES[yname]["name"]),
        "type": "{} Derivative".format(VARIABLES[yname]["type"]),
        "normalize": lambda array: array,
        "units": "{} / {}".format(VARIABLES[yname]["units"], VARIABLES[xname]["units"]),
    }
    return (
        (xname, newYname),
        np.column_stack(((array[:-1, 0] + array[1:, 0]) / 2,
                        (array[1:, 1] - array[:-1, 1]) / (array[1:, 0] - array[:-1, 0])))
    )


def importDataFiles(dirname):
    output = {}
    for name in os.listdir(dirname):
        filepath = "{}/{}".format(dirname, name)
        if os.path.isfile(filepath) and DATA_FILENAME_RE.match(name):
            output[name] = np.load(filepath)
    return output


def plotSeries(series, outputFile):
    for (xname, yname), array in series:
        plt.plot(array[:, 0], array[:, 1], label=VARIABLES[yname]["name"])
        plt.xlabel("{} ({})".format(
            VARIABLES[xname]["type"], VARIABLES[xname]["units"]))
        plt.ylabel("{} ({})".format(
            VARIABLES[yname]["type"], VARIABLES[yname]["units"]))

    plt.legend()
    plt.savefig(outputFile)
    plt.clf()


if __name__ == "__main__":
    args = parser.parse_args((sys.argv[1:]))
    print("Opening data directory {}...".format(args.input_dir))
    data = {}
    for name, array in importDataFiles(args.input_dir).items():
        print("{}: {} samples ({})".format(name, array.shape, array.dtype))
        components = name.split(".")[0].split("-")
        key = (components[2], components[0])
        data[key] = (key, normalize(key, array))
    data[("t", "P")] = (("t", "P"), lowpassFilter(data[("t", "P")][1], 0.1))
    plotSeries([data[("t", "xe")], data[("t", "xs")]],
               "{}/displacement.png".format(args.input_dir))
    plotSeries([data[("t", "P")]],
               "{}/pressure.png".format(args.input_dir))
    data[("t", "deltax")] = (
        ("t", "deltax"),
        normalizeY(("t", "deltax"), difference(
            data[("t", "xe")][1], data[("t", "xs")][1])))
    plotSeries([data[("t", "deltax")]], "{}/deltax.png".format(args.input_dir))
    data[("P", "deltax")] = join(data[("t", "P")], data[("t", "deltax")])
    plotSeries([data[("P", "deltax")]],
               "{}/phase_space.png".format(args.input_dir))
    data[("t", "dPdt")] = derivative(data[("t", "P")])
    plotSeries([data[("t", "dPdt")]],
               "{}/pressure_derivative.png".format(args.input_dir))
    data[("P", "dPdt")] = join(data[("t", "P")], data[("t", "dPdt")])
    plotSeries([data[("P", "dPdt")]],
               "{}/pressure_phase_space.png".format(args.input_dir))
    allData = np.column_stack(
        interp(*(i[1] for i in data.values() if i[0][0] == "t")))
