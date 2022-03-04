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
        PEst = (K12 * (vsEst)) ** n
        PEst -= PEst[0]
        return (((xsEst - xs) / xsRange)**2 + ((PEst - P) / PRange)**2)**0.5
    params = opt.curve_fit(eq, t, np.zeros(t.shape), guess, bounds=((0, 0, float("-inf"), 0, 0, 1), (float("inf"), float("inf"), 0, float("inf"), float("inf"), 2)))
    return params

def calculateShearThinningExp(t, data, guess):
    minTime = min(t)
    def eq(t, *params):
        A, B, T = params
        return A * (1 - np.exp(T * (t - minTime)) + B * (t - minTime))
    params = opt.curve_fit(eq, t, data, guess)[0]
    return (*params, minTime), eq

if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])

    # Load metadata (if it already exists)
    if os.path.exists(args.metadata_file):
        rawMetadata = json.load(open(args.metadata_file, "r"))
        metadataDict = {key: Metadata(**value) for key, value in rawMetadata.items()}
    else:
        print("Cannot find metadata file '{}'".format(args.metadata_file))
        exit(0)

    n = {}
    for key, metadata in metadataDict.items():
        dataSeriesDict = metadata.loadData({"xe", "xs", "P", "m"})
        fig, axs = plt.subplots(2, 2)
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

        xsTail = xsNormalized.selectRange(metadata.extrusionEnd, metadata.extrusionEnd+30).normalizeToStart()
        pressureTail = P.selectRange(metadata.extrusionEnd, metadata.extrusionEnd+30).normalizeToStart()

        xsNormalized.plot(axs[0][0], "xs")
        xeNormalized.plot(axs[0][0], "xe")
        axs[0][0].set_xlabel("Time (s)")
        axs[0][0].set_ylabel("Displacement")
        axs[0][0].legend()

        try:
            xsTail.plot(axs[0][1], "diff")
            pressureTail.plot(axs[0][1], "pressure")
            #xsParams = xsTail.performExponentialRegression((600, -1e2))
            #xsParams, xsEq = calculateShearThinning(xsTail.data[:, 0], xsTail.data[:, 1], (200, 2e-2, -1e-1))
            
            #xsFit = xsTail.fromRegressionParameters(xsParams)
            #xsFit.plot(axs[0][1], "diff_fit", "--")
            #(xsTail - xsFit).plot(axs[0][1], "diff_residual", "--")
            #print(xsParams)
            #DataSeries(np.column_stack((xsTail.data[:, 0], xsEq(xsTail.data[:, 0], *xsParams[:3])))).plot(axs[0][1], "xs_fit", "--")
            #pressureParams = pressureTail.performExponentialRegression((-100, -1e2))
            #pressureParams, pressureEq = calculateShearThinning(pressureTail.data[:, 0], pressureTail.data[:, 1], (-100, 2e-2, -1e-1))
            #print(pressureParams)
            #DataSeries(np.column_stack((pressureTail.data[:, 0], pressureEq(pressureTail.data[:, 0], *pressureParams[:3])))).plot(axs[0][1], "pressure_fit", "--")
            #pressureFit = pressureTail.fromRegressionParameters(pressureParams)
            #pressureFit.plot(axs[0][1], "pressure_fit", "--")
            #(pressureTail - pressureFit).plot(axs[0][1], "pressure_residual", "--")
            A, B, T, K12, K3, n = calculateShearThinningJointExp(*interp(xsTail.data, pressureTail.data), (200, 2e-2, -1e-1, 1, 1, 1.2))[0]
            print(A, B, T, K12, K3, n)
            t = xsTail.data[:, 0] - min(xsTail.data[:, 0])
            xsFit = A * (1 - np.exp(T * t) + B * t)
            vsFit = A * (-T * np.exp(T * t) + B)
            pressureFit = K12 * abs(vsFit)**n 
            pressureFit -= pressureFit[0]
            xsFitDS = DataSeries(np.column_stack((xsTail.data[:, 0], xsFit)))
            vsFitDS = DataSeries(np.column_stack((xsTail.data[:, 0], vsFit)))
            pressureFitDS = DataSeries(np.column_stack((xsTail.data[:, 0], pressureFit)))
            xsFitDS.plot(axs[0][1], "xsFit", "--")
            vsFitDS.plot(axs[0][1], "vsFit", "--")
            pressureFitDS.plot(axs[0][1], "pressureFit", "--")
            axs[0][1].set_xlabel("Time (s)")

        except Exception as exception:
            print("EXCEPTION on \"{}\": {}".format(key, exception))

        massVsDisplacement.plot(axs[1][0], "mass vs. displacement")

        pressureVsDisplacement.plot(axs[1][1], "pressure vs. displacement")
        axs[1][1].set_xlabel("Displacement")
        axs[1][1].set_ylabel("Pressure")
        plt.show()

    for name, data in clusterExperimentDict(n).items():
        print("{}: {}".format(name, data))
