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


def interp(*arrays):
    ts = np.concatenate([array[:, 0] for array in arrays])
    ts.sort()
    ts = np.unique(ts)
    return (ts,) + tuple((np.interp(ts, array[:, 0], array[:, 1]) for array in arrays))


def unaryOperator(array, op):
    return np.column_stack((array[:, 0], op(array[:, 1])))


def binaryOperator(array1, array2, op):
    ts, interp1, interp2 = interp(array1, array2)
    return np.column_stack((ts, op(interp1, interp2)))


def calculateDerivative(array):
    return np.row_stack((
        np.array([array[0, 0], 0]),
        np.column_stack((
            array[1:, 0],
            (array[1:, 1] - array[:-1, 1]) / (array[1:, 0] - array[:-1, 0])
        )),
    ))


def calculateLowpass(array, a):
    kernelSize = int(1 / a)
    output = np.zeros(array.shape)
    output[:, 0] = array[:, 0]
    for i in range(output.shape[0]):
        minRange = max(0, i - kernelSize // 2)
        maxRange = min(output.shape[0], i + kernelSize // 2)
        output[i, 1] = np.sum(
            array[minRange:maxRange, 1]) / (maxRange - minRange)
    return output


def calculateExpLowpass(array, a):
    output = np.zeros(array.shape)
    state = 0
    for i in range(array.shape[0]):
        state = state * (1 - a) + a * array[i, 1]
        output[i, 0] = array[i, 0]
        output[i, 1] = state
    return output


def calculateRms(array):
    return np.sqrt(
        np.sum(
            unaryOperator(array, lambda x: x**2)[:, 1]
        ) / array.shape[0]
    )


def isPD(B):
    """Returns true when input is positive-definite, via Cholesky"""
    try:
        _ = np.linalg.cholesky(B)
        return True
    except np.linalg.LinAlgError:
        return False


def nearestPD(A):
    """Find the nearest positive-definite matrix to input

    A Python/Numpy port of John D'Errico's `nearestSPD` MATLAB code [1], which
    credits [2].

    [1] https://www.mathworks.com/matlabcentral/fileexchange/42885-nearestspd

    [2] N.J. Higham, "Computing a nearest symmetric positive semidefinite
    matrix" (1988): https://doi.org/10.1016/0024-3795(88)90223-6
    """

    B = (A + A.T) / 2
    _, s, V = np.linalg.svd(B)

    H = np.dot(V.T, np.dot(np.diag(s), V))

    A2 = (B + H) / 2

    A3 = (A2 + A2.T) / 2

    if isPD(A3):
        return A3

    spacing = np.spacing(np.linalg.norm(A))
    # The above is different from [1]. It appears that MATLAB's `chol` Cholesky
    # decomposition will accept matrixes with exactly 0-eigenvalue, whereas
    # Numpy's will not. So where [1] uses `eps(mineig)` (where `eps` is Matlab
    # for `np.spacing`), we use the above definition. CAVEAT: our `spacing`
    # will be much larger than [1]'s `eps(mineig)`, since `mineig` is usually on
    # the order of 1e-16, and `eps(1e-16)` is on the order of 1e-34, whereas
    # `spacing` will, for Gaussian random matrixes of small dimension, be on
    # othe order of 1e-16. In practice, both ways converge, as the unit test
    # below suggests.
    I = np.eye(A.shape[0])
    k = 1
    while not isPD(A3):
        mineig = np.min(np.real(np.linalg.eigvals(A3)))
        A3 += I * (-mineig * k**2 + spacing)
        k += 1

    return A3


class Series:
    def __init__(self, ind, dep, array):
        self.ind = ind
        self.dep = dep
        self.array = array

    def asTuple(self):
        return (self.ind, self.dep)

    def __str__(self):
        return str(self.asTuple())

    def numRows(self):
        return self.array.shape[0]

    def applyUnary(self, func, newDep=None):
        return Series(self.ind, self.dep if newDep is None else newDep, unaryOperator(self.array, func))

    def applyBinary(self, other, func, newDep=None):
        return Series(self. ind, self.dep if newDep is None else newDep, binaryOperator(self.array, other.array, func))

    def lowpass(self, a, newDep=None):
        if newDep is None:
            newDep = "{}_filtered".format(self.dep)
        return Series(self.ind, newDep, calculateLowpass(self.array, a))

    def expLowpass(self, a, newDep=None):
        if newDep is None:
            newDep = "{}_expfiltered".format(self.dep)
        return Series(self.ind, newDep, calculateExpLowpass(self.array, a))

    def rms(self):
        return calculateRms(self.array)

    def averageAbs(self):
        return np.sum(np.abs(self.array[:, 1])) / self.numRows()

    def derivative(self, newDep=None):
        if newDep is None:
            newDep = "d{}d{}".format(self.dep, self.ind)
        return Series(self.ind, newDep, calculateDerivative(self.array))

    def join(self, other):
        # Join this series as independent variable with another series
        if self.ind != other.ind:
            raise TypeError("Independent variables must match; these are {} and {}".format(
                self.ind, other.ind))
        ts, interp1, interp2 = interp(self.array, other.array)
        return Series(self.dep, other.dep, np.column_stack((interp1, interp2)))

    def error(self, reference, newDep=None):
        # Calculate the error of this series against a reference
        if self.ind != reference.ind:
            raise TypeError("Independent variables must match; these are {} and {}".format(
                self.ind, reference.ind))
        if newDep is None:
            newDep = "{}_error".format(self.dep)
        ts, interp1, interp2 = interp(self.array, reference.array)
        return Series(self.ind, newDep, np.column_stack((ts, interp2 - interp1)))

    def smoothness(self, name, a=0.001):
        # Calculate "smoothness" of a function by taking the
        # RMS error of the series from its lowpass average
        return self.error(self.lowpass(a)).rms()

    def negativity(self):
        # Calculate the amount of signal that is negative
        data = self.array[:, 1]
        data[data > 0] = 0
        negative = Series(self.ind, self.dep,
                          np.column_stack((self.array[:, 0], data)))
        return negative.rms() / self.rms()


def importDataFiles(dirname):
    pairs = {}
    for name in os.listdir(dirname):
        filepath = "{}/{}".format(dirname, name)
        if os.path.isfile(filepath) and DATA_FILENAME_RE.match(name):
            pairs[name] = np.load(filepath)

    output = {}
    for name, array in pairs.items():
        print("{}: {} samples ({})".format(name, array.shape, array.dtype))
        components = name.split(".")[0].split("-")
        array = np.column_stack(
            ((array[:, 0] - array[0, 0]) * 1e-6, array[:, 1]))
        array = array[1:][array[:-1, 0] < array[1:, 0]]
        output[components[0]] = Series(components[2], components[0], array)
    return output


def plotSeries(series, outputDir, outputFileName=None, show=False, **kwargs):
    # Check that all independent variables are the same
    if not all((s.ind == series[0].ind for s in series)):
        print("Could not plot {} because the independent variables are different".format(
            (", ".join(map(str, series)))))
        return

    # Come up with a file name
    if outputFileName is None:
        outputFileName = "{}-vs-{}.png".format(
            ",".join((s.dep for s in series)),
            series[0].ind)
    outputDir = "{}/{}".format(outputDir, outputFileName)

    print("Plot ({}) -> \"{}\"".format(", ".join(map(str, series)), outputDir))

    plt.figure(dpi=200)
    for s in series:
        if "style" in kwargs:
            plt.plot(s.array[:, 0], s.array[:, 1],
                     kwargs["style"], label=s.dep)
        else:
            plt.plot(s.array[:, 0], s.array[:, 1], label=s.dep)
        plt.xlabel(s.ind)
    plt.legend()
    plt.title(outputDir)
    plt.savefig(outputDir)
    plt.clf()
