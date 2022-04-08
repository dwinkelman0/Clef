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
        # t, xs, P = independentVars
        A, B, T, K12, K3, n = params
        xsEst = A * (1 - np.exp(T * (t - minTime)) + B * (t - minTime))
        vsEst = A * (-T * np.exp(T * (t - minTime)) + B)
        # PEst = (K12 * (vsEst)) ** n
        # PEst -= PEst[0]
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


def calculateShearThinningParams(n, alpha, guess):
    def eq(n, *params):
        K12, K3 = params
        return K12 * K3**(n-1)
    params = opt.curve_fit(eq, n, alpha, guess)[0]
    return params, eq


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

    fig, axs = plt.subplots(2, 2)
    fig.set_figwidth(fig.get_figwidth() * 1.2)
    axs[0][0].set_title("Gelma 10%, F = 100")
    axs[0][1].set_title("Gelma 10%, F = 300")
    axs[0][0].set_ylabel("Displacement (mL)")
    axs[1][0].set_ylabel("Pressure (psi)")

    nLine = {}
    nNoLine = {}
    alphaNoLine = {}
    nAndAlphaNoLine = {}
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
            pressureParamsNoLine, pressureEq = calculateShearThinningExp(
                pressureTail.data[:, 0], pressureTail.data[:, 1], (-50, 0.03, -0.2), False)
            pressureParamsLine, pressureEq = calculateShearThinningExp(
                pressureTail.data[:, 0], pressureTail.data[:, 1], (-50, 0.03, -0.2), True)
            A, B, T, Tmin = pressureParamsNoLine
            n = pressureParamsNoLine[2] / xsParamsNoLine[2]
            Kp = pressureParamsNoLine[0]
            alpha = Kp / (A * T)**n
            nNoLine[key] = n
            alphaNoLine[key] = -alpha
            nAndAlphaNoLine[key] = (n, -alpha)
            nLine[key] = pressureParamsLine[2] / xsParamsLine[2]
            bLine[key] = pressureParamsLine[1] / xsParamsLine[1]
            # axs[0][1].set_xlabel("Time (s)")

        except Exception as exception:
            print("EXCEPTION on \"{}\": {}".format(key, exception))

        if key == "exp-data-6/n11-gelma10-10-100f":
            col = 0
        elif key == "exp-data-6/n12-gelma10-10-300f":
            col = 1
        else:
            col = None
        if col is not None:
            axs[0][col].plot(xsTail.normalizeToStart().data[:, 0] - Tmin, xsTail.data[:, 1] /
                             (5640 / 4.4), color="blue", label="Experiment")
            axs[0][col].plot(xsTail.data[:, 0] - Tmin, xsEq(xsTail.data[:, 0], xsParamsNoLine[0],
                             0, xsParamsNoLine[2]) / (5640 / 4.4), color="orange", linestyle="--", label="Fit (${:.3f}(1 - e^{{{:.3f}t}})$)".format(xsParamsNoLine[0] / (5640 / 4.4), xsParamsNoLine[2]))
            axs[1][col].plot(pressureTail.data[:, 0] - Tmin, (-pressureParamsNoLine[0] + pressureTail.data[:,
                             1]) / 1024 * 150, color="blue", label="Experiment")
            axs[1][col].plot(xsTail.data[:, 0] - Tmin, (-pressureParamsNoLine[0] + pressureEq(xsTail.data[:, 0], pressureParamsNoLine[0],
                             0, pressureParamsNoLine[2])) / 1024 * 150, color="orange", linestyle="--", label="Fit (${:.1f}e^{{{:.3f}t}}$)".format(-pressureParamsNoLine[0] / 1024 * 150, pressureParamsNoLine[2]))
            axs[0][col].axhline(y=xsParamsNoLine[0] / (5640 / 4.4),
                                color="black", linestyle="--")
            axs[0][col].set_xlim(
                [0, max(xsTail.data[:, 0]) - Tmin])
            axs[0][col].set_ylim([0, 0.4])
            axs[1][col].set_xlim(
                [0, max(xsTail.data[:, 0]) - Tmin])
            axs[1][col].set_ylim([0, 20])
            axs[0][col].legend()
            axs[1][col].legend()
            axs[1][col].set_xlabel("Time (seconds)")

    fig.tight_layout()
    plt.savefig("figs/relaxation_fit.png")

    # Plot hydraulic capacitance example
    fig, axs = plt.subplots(1, 2)
    fig.set_figwidth(fig.get_figwidth() * 1.2)
    for col, (name, feedrate) in enumerate([("exp-data-6/n11-gelma10-10-100f", 100), ("exp-data-6/n12-gelma10-10-300f", 300)]):
        metadata = metadataDict[name]
        data = metadataDict[name].loadData({"xe", "xs", "P"})
        normalization = (max(data["xs"].data[:, 1]) -
                         min(data["xs"].data[:, 1])) / 5640 * 4.4
        seriesStart = (data["xe"].normalizeToStartAndEnd(normalization).selectRange(metadata.extrusionStart, metadata.extrusionEnd) - data["xs"].normalizeToStartAndEnd(
            normalization).selectRange(metadata.extrusionStart, metadata.extrusionEnd)).join(data["P"].selectRange(metadata.extrusionStart, metadata.extrusionEnd))
        seriesEnd = (data["xe"].normalizeToStartAndEnd(normalization).selectRange(metadata.extrusionEnd, None) - data["xs"].normalizeToStartAndEnd(
            normalization).selectRange(metadata.extrusionEnd, None)).join(data["P"].selectRange(metadata.extrusionEnd, None))
        m, b = seriesStart.linearFit((200, 400))
        axs[col].plot(seriesStart.data[:, 0], seriesStart.data[:, 1] /
                      1024 * 150, color="blue", label="Extrusion")
        axs[col].plot(seriesEnd.data[:, 0], seriesEnd.data[:, 1] /
                      1024 * 150, color="green", linestyle=":", label="Relaxation")
        bestFitPoints = np.array([0, 7])
        axs[col].plot(bestFitPoints, (m * bestFitPoints + b) /
                      1024 * 150, color="orange", linestyle="--", label="Fit (${:.1f}(x_e - x_s) + {:.1f}$)".format(m / 1024 * 150, b / 1024 * 150))
        axs[col].set_xlim([0, 0.7])
        axs[col].set_ylim([28, 65])
        axs[col].legend()
        axs[col].set_xlabel("Compression Displacement (mL)")
    axs[0].set_ylabel("Pressure (psi)")
    axs[0].set_title("Gelma 10%, F = 100")
    axs[1].set_title("Gelma 10%, F = 300")
    fig.tight_layout()
    fig.savefig("figs/pressure_vs_disp_v1.png")

    # Plot hydraulic capacitance trend
    fig, axs = plt.subplots(1, 1)
    KH = {}
    KH_average = {}
    for key, metadata in metadataDict.items():
        data = metadata.loadData({"xe", "xs", "P"})
        normalization = (max(data["xs"].data[:, 1]) -
                         min(data["xs"].data[:, 1])) / 5640 * 4.4
        seriesStart = (data["xe"].normalizeToStartAndEnd(normalization).selectRange(metadata.extrusionStart, metadata.extrusionEnd) - data["xs"].normalizeToStartAndEnd(
            normalization).selectRange(metadata.extrusionStart, metadata.extrusionEnd)).join(data["P"].selectRange(metadata.extrusionStart, metadata.extrusionEnd))
        m, b = seriesStart.linearFit((200, 400))
        KH[key] = (m / 1024 * 150, b / 1024 * 150)
    overall = []
    for row, (name, fancy, color) in enumerate([("alg2.5", "Alginate 2.5%", "blue"), ("alg5", "Alginate 5%", "green"), ("gelma10", "Gelma 10%", "orange")]):
        m = np.array(
            [i[0] for i in clusterExperimentDict(KH)[name]])
        b = np.array(
            [i[1] for i in clusterExperimentDict(KH)[name]])
        axs.plot(m, b, linestyle="", marker="o", color=color, label=fancy)
        axs.plot([sum(m)/len(m)], [sum(b)/len(b)],
                 linestyle="", marker="x", color=color, label="{} Avg.".format(fancy))
        overall.append([sum(m)/len(m), sum(b)/len(b)])
        KH_average[name] = (sum(m)/len(m), sum(b)/len(b))
    bounds = np.array([30, 70])
    overall = DataSeries(np.array(overall))
    print(overall.data)
    mOverall, bOverall = overall.linearFit((1, 15))
    axs.plot(bounds, bounds * mOverall + bOverall, linestyle="--",
             color="black", label="Overall Trend")
    axs.legend()
    axs.set_xlim([30, 70])
    axs.set_title("Hydraulic Linear Regression")
    axs.set_xlabel("Fit Slope or $K_H$ (psi/mL)")
    axs.set_ylabel("Fit Intercept (psi)")
    fig.tight_layout()
    fig.savefig("figs/hydraulic_coef.png")

    for name, data in clusterExperimentDict(nNoLine).items():
        print("{}: {}".format(name, data))
    for name, data in clusterExperimentDict(nLine).items():
        print("{}: {}".format(name, data))
    for name, data in clusterExperimentDict(bLine).items():
        print("{}: {}".format(name, data))

    # Plot n data
    nFinal = {}
    fig, axs = plt.subplots(3, 1)
    for col, (data, label, color) in enumerate([(nNoLine, "Simple Exponential", "blue"), (nLine, "Linear Exponential", "orange")]):
        for row, (name, fancy) in enumerate([("alg2.5", "Alginate 2.5%"), ("alg5", "Alginate 5%"), ("gelma10", "Gelma 10%")]):
            x = clusterExperimentDict(data)[name]
            meanX = [xi for xi in x if 0.8 <= xi and xi <= 1.6]
            mean = sum(meanX) / len(meanX)
            stddev = (sum([(xi - mean)**2 for xi in meanX]) / len(meanX))**0.5
            if col == 0:
                nFinal[name] = (mean, stddev)
            axs[row].plot([mean], [1.15 - col * 0.3], color=color, marker="o")
            axs[row].plot([mean - stddev, mean + stddev],
                          [1.15 - col * 0.3] * 2, color=color, linestyle="-")
            axs[row].plot(x, [1.15 - col * 0.3] * len(x),
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

    print(nFinal)

    # Plot alpha data
    fig, ax = plt.subplots(1, 1)
    for row, (name, fancy, color, marker, linestyle) in enumerate([("alg2.5", "Alginate 2.5%", "blue", "o", "-"), ("alg5", "Alginate 5%", "green", "s", "--"), ("gelma10", "Gelma 10%", "orange", "x", ":"), ]):
        x = clusterExperimentDict(nAndAlphaNoLine, False)[name]
        n = [a for a, b in x]
        alpha = [b for a, b in x]
        params, eq = calculateShearThinningParams(n, alpha, (10, 0.8))
        ax.plot(n, alpha, color=color, marker=marker,
                linestyle='', label=fancy)
        ax.plot(np.linspace(0.7, 1.5), eq(np.linspace(0.7, 1.5),
                *params), color=color, linestyle=linestyle)
        ax.set_xlim([0.7, 1.5])
        print(fancy, params)
    ax.set_xlabel("Shear Thinning Constant ($n$)")
    ax.set_ylabel("Alpha")
    ax.set_title("Material Parameter Extraction")
    ax.legend()
    plt.tight_layout()
    plt.savefig("figs/alpha_plot_v1.png")

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

    # Plot example extrusion experiments
    fig, axs = plt.subplots(3, 2)
    fig.set_figwidth(fig.get_figwidth() * 1.2)
    fig.set_figheight(fig.get_figheight() * 1.5)
    for col, (name, feedrate) in enumerate([("exp-data-6/n11-gelma10-10-100f", 100), ("exp-data-6/n12-gelma10-10-300f", 300)]):
        data = metadataDict[name].loadData({"xe", "xs", "P"})
        axs[0][col].plot(data["xe"].data[:, 0],
                         data["xe"].normalizeToStartAndEnd(4.4).data[:, 1], color="blue", linestyle="--", label="Stepper Disp. ($x_e$)")
        axs[0][col].plot(data["xs"].data[:, 0],
                         data["xs"].normalizeToStartAndEnd(4.4).data[:, 1], color="orange", linestyle="-", label="Plunger Disp. ($x_s$)")
        axs[0][col].set_xlim([6, 150])
        axs[1][col].plot(data["P"].data[:, 0],
                         data["P"].data[:, 1] / 1024 * 150, color="blue", label="Pressure ($P$)")
        axs[1][col].set_xlim([6, 150])
        axs[1][col].set_ylim([25, 65])
        deltaX = data["xe"].normalizeToStartAndEnd(
            4.4) - data["xs"].normalizeToStartAndEnd(4.4)
        axs[2][col].plot(deltaX.data[:, 0], deltaX.data[:, 1],
                         color="blue", label="Compression ($x_e - x_s$)")
        axs[2][col].set_xlim([6, 150])
        axs[2][col].set_ylim([0, 0.7])
        axs[2][col].set_xlabel("Time (seconds)")
        axs[0][col].axvline(x=metadataDict[name].extrusionEnd,
                            color="black", linestyle="--")
        axs[1][col].axvline(x=metadataDict[name].extrusionEnd,
                            color="black", linestyle="--")
        axs[2][col].axvline(x=metadataDict[name].extrusionEnd,
                            color="black", linestyle="--")
    axs[0][0].set_title("Gelma 10%, F = 100")
    axs[0][1].set_title("Gelma 10%, F = 300")
    axs[0][1].legend()
    axs[1][1].legend()
    axs[2][1].legend()
    axs[0][0].set_ylabel("Displacement (mL)")
    axs[1][0].set_ylabel("Pressure (psi)")
    axs[2][0].set_ylabel("Displacement (mL)")
    plt.tight_layout()
    plt.savefig("figs/exs_v1.png")
