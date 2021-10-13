#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import kalman

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


AXIS_DESCRIPTIONS = {
    "t": {
        "name": "Time",
        "type": "Time",
        "units": "seconds",
        "quantities": ["t"],
    },
    "xe": {
        "name": "Stepper Motor Position",
        "type": "Displacement",
        "units": "E-Axis usteps",
        "quantities": ["xe", "xe_kalman"],
    },
    "xs": {
        "name": "Syringe Plunger Position",
        "type": "Displacement",
        "units": "E-Axis usteps",
        "quantities": ["xs", "mux_kalman", "muxs"],
    },
    "xn": {
        "name": "Syringe Needle Position",
        "type": "Displacement",
        "units": "E-Axis usteps",
        "quantities": ["xn", "muxn"],
    },
    "feedrate": {
        "name": "Feedrate",
        "type": "Feedrate",
        "units": "E-Axis usteps / sec",
        "quantities": ["dxedt", "dxs_filtereddt"],
    },
    "deltax": {
        "name": "Displacement Difference",
        "type": "Displacement",
        "units": "E-Axis usteps",
        "quantities": ["deltax"],
    },
    "Ph": {
        "name": "Hydraulic Pressure",
        "type": "Pressure",
        "units": "Pressure units",
        "quantities": ["P", "Ph", "muPh_kalman", "Ph0_kalman"],
    },
    "Ps": {
        "name": "Syringe Pressure",
        "type": "Pressure",
        "units": "Pressure units",
        "quantities": ["muPs_kalman"],
    },
    "alpha": {
        "name": "Alpha Coefficient",
        "type": "Pressure Capacitance",
        "units": "Pressure units / E-Axis usteps",
        "quantities": ["alpha"],
    },
    "beta": {
        "name": "Beta Coefficient",
        "type": "Viscocity",
        "units": "E-Axis usteps / Pressure units / second",
        "quantities": ["beta"],
    },
    "AhAs": {
        "name": "Cylinder Area Coefficient",
        "type": "Ratio",
        "units": "Dimensionless",
        "quantities": ["AhAs"],
    },
    "FfdAs": {
        "name": "Dynamic Friction Coefficient",
        "type": "Dynamic Friction",
        "units": "Pressure units",
        "quantities": ["FfdAs"],
    },
    "rhoeta": {
        "name": "Viscocity Coefficient",
        "type": "Viscocity",
        "units": "idk",
        "quantities": ["rhoeta"],
    }
}


def getAxisDescriptions(quantity):
    for quantityType, desc in AXIS_DESCRIPTIONS.items():
        for quantityOption in desc["quantities"]:
            if quantity.startswith(quantityOption):
                return desc


def chooseNameModifiers(quantity):
    name = getAxisDescriptions(quantity)["name"]
    modifiers = []
    if "kalman" in quantity:
        modifiers.append("Kalman")
    if "filtered" in quantity:
        modifiers.append("Filtered")
    if "error" in quantity:
        modifiers.append("Error")
    if len(modifiers) > 0:
        return "{} ({}) [{}]".format(name, ", ".join(modifiers), quantity)
    else:
        return "{} [{}]".format(name, quantity)


def interp(*arrays):
    ts = np.concatenate([array[:, 0] for array in arrays])
    ts.sort()
    ts = np.unique(ts)
    return (ts,) + tuple((np.interp(ts, array[:, 0], array[:, 1]) for array in arrays))


data = {}


def createSeries(names, array):
    data[names] = array


def join(xpair, ypair):
    assert(xpair[0] == ypair[0])
    joint, col1, col2 = interp(data[xpair], data[ypair])
    createSeries((xpair[1], ypair[1]), np.column_stack((col1, col2)))


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


def unaryOperator(array, op):
    return np.column_stack((array[:, 0], op(array[:, 1])))


def binaryOperator(array1, array2, op):
    ts, interp1, interp2 = interp(array1, array2)
    return np.column_stack((ts, op(interp1, interp2)))


def derivative(name):
    array = data[name]
    createSeries(
        (name[0], "d{}d{}".format(name[1], name[0])),
        np.column_stack(((array[:-1, 0] + array[1:, 0]) / 2,
                         (array[1:, 1] - array[:-1, 1]) / (array[1:, 0] - array[:-1, 0]))))


def addition(x, y): return x + y
def difference(x, y): return x - y


def plotSeries(names, outputDir, outputFileName):
    print("Plot ({}) -> {}".format(", ".join(map(str, names)), outputFileName))
    plt.figure(dpi=200)
    for name in names:
        array = data[name]
        xquantity, yquantity = name
        xdesc = getAxisDescriptions(xquantity)
        ydesc = getAxisDescriptions(yquantity)
        plt.plot(array[:, 0], array[:, 1],
                 label=chooseNameModifiers(yquantity))
        plt.xlabel("{} ({})".format(xdesc["type"], xdesc["units"]))
        plt.ylabel("{} ({})".format(ydesc["type"], ydesc["units"]))
    plt.legend()
    plt.savefig("{}/{}.png".format(outputDir, outputFileName))
    plt.clf()


def importDataFiles(dirname):
    output = {}
    for name in os.listdir(dirname):
        filepath = "{}/{}".format(dirname, name)
        if os.path.isfile(filepath) and DATA_FILENAME_RE.match(name):
            output[name] = np.load(filepath)
    return output


if __name__ == "__main__":
    args = parser.parse_args((sys.argv[1:]))

    # Import data
    print("Opening data directory {}...".format(args.input_dir))
    for name, array in importDataFiles(args.input_dir).items():
        print("{}: {} samples ({})".format(name, array.shape, array.dtype))
        components = name.split(".")[0].split("-")
        key = (components[2], components[0])
        array = np.column_stack(
            ((array[:, 0] - array[0, 0]) * 1e-6, array[:, 1]))
        array = array[1:][array[:-1, 0] < array[1:, 0]]
        createSeries(key, array)

    # Normalize and Filter
    data[("t", "xe")] = unaryOperator(data[("t", "xe")], lambda x: x - x[0])
    data[("t", "xs")] = unaryOperator(data[("t", "xs")], lambda x: x - x[0])
    # data[("t", "P")] = unaryOperator(data[("t", "P")], lambda x: x - x[0])
    lowpassFilter(("t", "P"), 0.01)

    # Generate deltax and phase space
    createSeries(("t", "deltax"), binaryOperator(
        data[("t", "xe")], data[("t", "xs")], difference))
    lowpassFilter(("t", "xs"), 0.02)
    derivative(("t", "xe"))
    derivative(("t", "xs_filtered"))
    join(("t", "P"), ("t", "deltax"))
    join(("t", "P"), ("t", "dxs_filtereddt"))
    join(("t", "deltax"), ("t", "dxs_filtereddt"))

    # Create plots of original data
    plotSeries([("t", "xe"), ("t", "xs")], args.input_dir, "displacement")
    plotSeries([("t", "P")], args.input_dir, "pressure")
    plotSeries([("t", "deltax")], args.input_dir, "deltax")
    plotSeries([("P", "deltax")], args.input_dir,
               "phase_space_pressure_deltax")
    plotSeries([("t", "dxedt"), ("t", "dxs_filtereddt")],
               args.input_dir, "feedrate")
    plotSeries([("P", "dxs_filtereddt")],
               args.input_dir, "phase_space_pressure_feedrate")
    plotSeries([("deltax", "dxs_filtereddt")],
               args.input_dir, "phase_space_deltax_feedrate")

    # Perform Kalman analysis
    kalmanInput = np.column_stack(interp(data[("t", "xe")],
                                         data[("t", "P")], data[("t", "xs")]))
    kalmanOutput = kalman.analyze(kalmanInput)
    for i, name in enumerate(["muxe", "muxs", "muxn", "dmuxndt", "muPh", "muPs", "alphah", "alphas", "rhoeta", "AhAs", "FfdAs", "xs0", "Ph0"]):
        key = ("t", "{}_kalman".format(name))
        print(key)
        createSeries(key, np.column_stack(
            (kalmanOutput[:, 0], kalmanOutput[:, i+4])))

    # Calculate measurements
    createSeries(("t", "xs_kalman"), binaryOperator(
        data[("t", "muxs_kalman")], data[("t", "xs0_kalman")], addition))
    createSeries(("t", "Ph_kalman"), binaryOperator(
        data[("t", "muPh_kalman")], data[("t", "Ph0_kalman")], lambda muPh, Ph0: muPh + 1000 * Ph0))

    # Calculate state error
    createSeries(("t", "xs_kalman_error"), binaryOperator(
        data[("t", "xs")], data[("t", "xs_kalman")], difference))
    createSeries(("t", "Ph_kalman_error"), binaryOperator(
        data[("t", "P")], data[("t", "Ph_kalman")], difference))

    plotSeries([("t", "xe"), ("t", "xs"), ("t", "xs_kalman"), ("t", "muxn_kalman")],
               args.input_dir, "kalman_displacement")
    plotSeries([("t", "P"), ("t", "Ph_kalman")],
               args.input_dir, "kalman_pressure")
    plotSeries([("t", "xs_kalman_error")],
               args.input_dir, "kalman_displacement_error")
    plotSeries([("t", "Ph_kalman_error")],
               args.input_dir, "kalman_pressure_error")
    plotSeries([("t", "Ph0_kalman")], args.input_dir, "kalman_Ph0")
    plotSeries([("t", "alphah_kalman")], args.input_dir, "kalman_alphah")
    plotSeries([("t", "alphas_kalman")], args.input_dir, "kalman_alphas")
    plotSeries([("t", "AhAs_kalman")], args.input_dir, "kalman_AhAs")
    plotSeries([("t", "FfdAs_kalman")], args.input_dir, "kalman_FfdAs")
    plotSeries([("t", "muPs_kalman")], args.input_dir, "kalman_muPs")
    plotSeries([("t", "rhoeta_kalman")], args.input_dir, "kalman_rhoeta")
