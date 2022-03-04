#!/bin/python3

# Copyright 2022 by Daniel Winkelman. All rights reserved.

from data_series import DataSeries, clusterExperimentDict, interp
from metadata import Metadata

import argparse
import json
import matplotlib.pyplot as plt
import numpy as np
import os
import scipy.optimize as opt
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--metadata-file", type=str, required=True)


def calculateShearThinningJointExp(t, xs, P, guess):
    minTime = min(t)
    xsRange = max(xs) - min(xs)
    PRange = max(P) - min(P)

    def eq(t, *params):
        #t, xs, P = independentVars
        A, B, T, K12, K3, n = params
        xsEst = A * (1 - np.exp(T * (t - minTime)) + B * (t - minTime))
        vsEst = A * (-T * np.exp(T * (t - minTime)) + B)
        #PEst = (K12 * (vsEst)) ** n
        #PEst -= PEst[0]
        # return (((xsEst - xs) / xsRange)**2 + ((PEst - P) / PRange)**2)**0.5
        return (xsEst - xs) / xsRange
    params = opt.curve_fit(eq, t, np.zeros(t.shape), guess, bounds=((0, 0, float(
        "-inf"), 0, 0, 1), (float("inf"), float("inf"), 0, float("inf"), float("inf"), 2)))
    return params


def calculateShearThinningExp(t, data, guess, addLine):
    minTime = min(t)

    def eq(t, *params):
        A, B, T = params
        B = 0 if not addLine else B
        return A * (1 - np.exp(T * (t - minTime)) + B * (t - minTime))
    params = opt.curve_fit(eq, t, data, guess)[0]
    return (*params, minTime), eq


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])

    # Load metadata (if it already exists)
    if os.path.exists(args.metadata_file):
        rawMetadata = json.load(open(args.metadata_file, "r"))
        metadataDict = {key: Metadata(**value)
                        for key, value in rawMetadata.items()}
    else:
        print("Cannot find metadata file '{}'".format(args.metadata_file))
        exit(0)

    nLine = {}
    nNoLine = {}
    bLine = {}
    for key, metadata in metadataDict.items():
        dataSeriesDict = metadata.loadData({"xe", "xs", "P", "m"})
        # fig, axs = plt.subplots(2, 2)
        xe = dataSeriesDict["xe"]
        xs = dataSeriesDict["xs"]
        P = dataSeriesDict["P"]
        m = dataSeriesDict["m"]
        xsNormalized = xs.normalizeToStart()
        xeNormalized = xe.normalizeToStartAndEnd(xs.getStartToEndDifference())
        mNormalized = m.normalizeToStart()
        displacementDifference = xeNormalized - xsNormalized
        pressureVsDisplacement = displacementDifference.join(P)
        massVsDisplacement = xsNormalized.join(mNormalized)

        xsTail = xsNormalized.selectRange(
            metadata.extrusionEnd, metadata.extrusionEnd+30).normalizeToStart()
        pressureTail = P.selectRange(
            metadata.extrusionEnd, metadata.extrusionEnd+30).normalizeToStart()

        # xsNormalized.plot(axs[0][0], "xs")
        # xeNormalized.plot(axs[0][0], "xe")
        # axs[0][0].set_xlabel("Time (s)")
        # axs[0][0].set_ylabel("Displacement")
        # axs[0][0].legend()

        try:
            # xsTail.plot(axs[0][1], "diff")
            # pressureTail.plot(axs[0][1], "pressure")
            xsParamsNoLine, xsEq = calculateShearThinningExp(
                xsTail.data[:, 0], xsTail.data[:, 1], (200, 0.03, -0.2), False)
            xsParamsLine, xsEq = calculateShearThinningExp(
                xsTail.data[:, 0], xsTail.data[:, 1], (200, 0.03, -0.2), True)
            # DataSeries(np.column_stack(
            #     (xsTail.data[:, 0], xsEq(xsTail.data[:, 0], *xsParams[:3])))).plot(axs[0][1], "diff_fit", "--")
            pressureParamsNoLine, pressureEq = calculateShearThinningExp(
                pressureTail.data[:, 0], pressureTail.data[:, 1], (-50, 0.03, -0.2), False)
            pressureParamsLine, pressureEq = calculateShearThinningExp(
                pressureTail.data[:, 0], pressureTail.data[:, 1], (-50, 0.03, -0.2), True)
            # DataSeries(np.column_stack(
            #     (pressureTail.data[:, 0], pressureEq(pressureTail.data[:, 0], *pressureParams[:3])))).plot(axs[0][1], "pressure_fit", "--")
            print(pressureParamsLine)
            nNoLine[key] = pressureParamsNoLine[2] / xsParamsNoLine[2]
            nLine[key] = pressureParamsLine[2] / xsParamsLine[2]
            bLine[key] = pressureParamsLine[1] / xsParamsLine[1]
            # axs[0][1].set_xlabel("Time (s)")

        except Exception as exception:
            print("EXCEPTION on \"{}\": {}".format(key, exception))

        # massVsDisplacement.plot(axs[1][0], "mass vs. displacement")

        # pressureVsDisplacement.plot(axs[1][1], "pressure vs. displacement")
        # axs[1][1].set_xlabel("Displacement")
        # axs[1][1].set_ylabel("Pressure")
        # plt.show()

    for name, data in clusterExperimentDict(nNoLine).items():
        print("{}: {}".format(name, data))
    for name, data in clusterExperimentDict(nLine).items():
        print("{}: {}".format(name, data))
    for name, data in clusterExperimentDict(bLine).items():
        print("{}: {}".format(name, data))

    # Plot n data
    fig, axs = plt.subplots(3, 1)
    for col, (data, label, color) in enumerate([(nNoLine, "Simple Exponential", "blue"), (nLine, "Linear Exponential", "orange")]):
        for row, (name, fancy) in enumerate([("alg2.5", "Alginate 2.5%"), ("alg5", "Alginate 5%"), ("gelma10", "Gelma 10%")]):
            x = clusterExperimentDict(data)[name]
            meanX = [xi for xi in x if 0.8 <= xi and xi <= 1.6]
            mean = sum(meanX) / len(meanX)
            stddev = (sum([(xi - mean)**2 for xi in meanX]) / len(meanX))**0.5
            axs[row].plot([mean], [1.1 - col * 0.2], color=color, marker="o")
            axs[row].plot([mean - stddev, mean + stddev],
                          [1.1 - col * 0.2] * 2, color=color, linestyle="-")
            axs[row].plot(x, [1.1 - col * 0.2] * len(x),
                          color=color, marker="x", linestyle='', label=label)
            axs[row].get_yaxis().set_visible(False)
            axs[row].set_xlim([0.8, 1.6])
            axs[row].set_ylim([0.7, 1.3])
            axs[row].set_title(fancy)
            if row == 2:
                axs[row].legend()
                axs[row].set_xlabel("Shear Thinning Constant ($n$)")
    plt.tight_layout()
    plt.savefig("figs/n_plot_v1.png")

    # Plot the two extrusion profiles
    fig, ax = plt.subplots(1, 1)
    f100 = metadataDict["exp-data-7/n1-alg7.5-10-100f"].loadData({"xe"})["xe"]
    f100 = f100.selectRange(None, 100).normalizeToStartAndEnd(4.4)
    f300 = metadataDict["exp-data-7/n2-alg7.5-10-300f"].loadData({"xe"})["xe"]
    f300 = f300.selectRange(None, 100).normalizeToStartAndEnd(4.4)
    ax.plot(f100.data[:, 0], f100.data[:, 1],
            color="blue", label="F = 100", linestyle=":")
    ax.plot(f300.data[:, 0], f300.data[:, 1],
            color="green", label="F = 300", linestyle="--")
    ax.set_xlabel("Time (seconds)")
    ax.set_ylabel("Displacement (mL)")
    ax.set_title("Extrusion Profiles")
    ax.legend()
    plt.tight_layout()
    plt.savefig("figs/xe_plot_v1.png")
