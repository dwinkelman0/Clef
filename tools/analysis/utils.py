# Copyright 2021 by Daniel Winkelman. All rights reserved.

import datetime
import os
import re
import sys

import numpy as np
import matplotlib.pyplot as plt


DATA_DEFAULT_DIRNAME_RE = re.compile(
    "data-\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d\.\d\d\d\d\d\d")
DATA_FILENAME_RE = re.compile("[a-zA-Z]+-vs-[a-zA-Z]+\.npy")


availableDirs = [name for name in os.listdir(".") if os.path.isdir(
    name) and DATA_DEFAULT_DIRNAME_RE.match(name)]
DEFAULT_DATA_DIR = max(availableDirs) if len(availableDirs) > 0 else None


data = {}


def createSeries(names, array):
    data[names] = array


def importDataFiles(dirname):
    pairs = {}
    for name in os.listdir(dirname):
        filepath = "{}/{}".format(dirname, name)
        if os.path.isfile(filepath) and DATA_FILENAME_RE.match(name):
            pairs[name] = np.load(filepath)

    for name, array in pairs.items():
        print("{}: {} samples ({})".format(name, array.shape, array.dtype))
        components = name.split(".")[0].split("-")
        key = (components[2], components[0])
        array = np.column_stack(
            ((array[:, 0] - array[0, 0]) * 1e-6, array[:, 1]))
        array = array[1:][array[:-1, 0] < array[1:, 0]]
        createSeries(key, array)


def interp(*arrays):
    ts = np.concatenate([array[:, 0] for array in arrays])
    ts.sort()
    ts = np.unique(ts)
    return (ts,) + tuple((np.interp(ts, array[:, 0], array[:, 1]) for array in arrays))


def joinSeries(name1, name2):
    if name1[0] != name2[0]:
        print("Independent variables must be the same (tried {} and {})".format(
            name1, name2))
        return

    joint, col1, col2 = interp(data[name1], data[name2])
    createSeries((name1[1], name2[1]), np.column_stack((col1, col2)))
    print("Joined ({}, {}) -> {} samples".format(
        name1[1], name2[1], data[(name1[1], name2[1])].shape[0]))


def lowpassFilter(name, a):
    array = data[name]
    kernelSize = int(1 / a)
    createSeries(
        (name[0], "{}_filtered".format(name[1])),
        np.column_stack((
            np.convolve(array[:, 0], np.ones(
                kernelSize) / kernelSize)[:-kernelSize],
            np.convolve(array[:, 1], np.ones(
                kernelSize) / kernelSize)[:-kernelSize],
        ))
    )


def derivative(name):
    array = data[name]
    createSeries(
        (name[0], "d{}d{}".format(name[1], name[0])),
        np.column_stack(((array[:-1, 0] + array[1:, 0]) / 2,
                         (array[1:, 1] - array[:-1, 1]) / (array[1:, 0] - array[:-1, 0]))))


def binaryOperator(array1, array2, op):
    ts, interp1, interp2 = interp(array1, array2)
    return np.column_stack((ts, op(interp1, interp2)))


def plotSeries(names, outputDir, outputFileName=None, show=False, **kwargs):
    # Check that all independent variables are the same
    if not all((name[0] == names[0][0] for name in names)):
        print("Could not plot {} because the independent variables are different".format(
            (", ".join(map(str, names)))))
        return

    # Come up with a file name
    if outputFileName is None:
        outputFileName = "{}-vs-{}.png".format(
            ",".join((name[1] for name in names)),
            names[0][0])
    outputDir = "{}/{}".format(outputDir, outputFileName)

    print("Plot ({}) -> \"{}\"".format(", ".join(map(str, names)), outputDir))

    plt.figure(dpi=200)
    for name in names:
        array = data[name]
        if "style" in kwargs:
            plt.plot(array[:, 0], array[:, 1], kwargs["style"], label=name[1])
        else:
            plt.plot(array[:, 0], array[:, 1], label=name[1])
        plt.xlabel(name[0])
    plt.legend()
    plt.title(outputDir)
    plt.savefig(outputDir)
    plt.clf()
