#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import datetime
import re
import os
import sys
from importlib.machinery import SourceFileLoader

import numpy as np
import matplotlib.pyplot as plt

import utils

parser = argparse.ArgumentParser()
parser.add_argument("--filter-type", type=str,
                    choices=["ekf", "ukf"], required=True)
parser.add_argument("--model", type=str, required=True)
parser.add_argument("--batch", action="store_true", default=False)
parser.add_argument("--data-dir", type=str, default=utils.DEFAULT_DATA_DIR)
parser.add_argument("--data-dir-root", type=str, default=".")
parser.add_argument("--data-dir-recurse", action="store_true", default=False)
parser.add_argument("--no-plots", action="store_true", default=False)
parser.add_argument("--deltat-catchup", type=float, default=1)

subparsers = parser.add_subparsers(dest="command")
run_parser = subparsers.add_parser("run")
optimize_parser = subparsers.add_parser("optimize")
optimize_parser.add_argument("--step-size", type=float, default=0.2)

logFileName = "log-{}".format(datetime.datetime.now().isoformat())


class Expression:
    def __init__(self):
        pass

    def __add__(self, other):
        if type(self) is Constant and self.value == 0:
            return other
        if type(other) is Constant and other.value == 0:
            return self
        if type(self) is Constant and type(other) is Constant:
            return Constant(self.value + other.value)
        return Sum(self, other)

    def __sub__(self, other):
        if type(self) is Constant and self.value == 0:
            return Product(Constant(-1), other)
        if type(other) is Constant and other.value == 0:
            return self
        if type(self) is Constant and type(other) is Constant:
            return Constant(self.value - other.value)
        return Sum(self, Product(Constant(-1), other))

    def __mul__(self, other):
        if type(self) is Constant and self.value == 1:
            return other
        if type(other) is Constant and other.value == 1:
            return self
        if self.isZero() or other.isZero():
            return Constant(0)
        if type(self) is Constant and type(other) is Constant:
            return Constant(self.value * other.value)
        return Product(self, other)

    def __truediv__(self, other):
        if type(self) is Constant and self.value == 1:
            return Power(other, Constant(-1))
        if type(other) is Constant and other.value == 1:
            return self
        if type(self) is Constant and type(other) is Constant:
            return Constant(self.value / other.value)
        return Product(self, Power(other, Constant(-1)))

    def __pow__(self, other):
        return Power(self, other)

    def __neg__(self):
        return Product(Constant(-1), self)


class Sum(Expression):
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def isZero(self):
        return self.left.isZero() and self.right.isZero()

    def isSubtraction(self):
        return \
            type(self.right) is Product and \
            type(self.right.left) is Constant and \
            self.right.left.value == -1

    def partialDerivative(self, var):
        return \
            self.left.partialDerivative(var) + \
            self.right.partialDerivative(var)

    def __repr__(self):
        return "{} + {}".format(repr(self.left), repr(self.right))


class Product(Expression):
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def isZero(self):
        return self.left.isZero() or self.right.isZero()

    def isDivision(self):
        return \
            type(self.right) is Power and \
            type(self.right.power) is Constant and \
            self.right.power.value == -1

    def partialDerivative(self, var):
        return \
            self.left.partialDerivative(var) * self.right + \
            self.right.partialDerivative(var) * self.left

    def __repr__(self):
        return "{} * {}".format(
            "({})".format(repr(self.left))
            if type(self.left) is Sum
            else repr(self.left),
            "({})".format(repr(self.right))
            if type(self.right) is Sum
            else repr(self.right),
        )


class Power(Expression):
    def __init__(self, base, power):
        self.base = base
        self.power = power

    def isZero(self):
        return False

    def partialDerivative(self, var):
        baseDer = self.base.partialDerivative(var)
        powerDer = self.power.partialDerivative(var)
        if baseDer.isZero():
            if powerDer.isZero():
                return Constant(0)
            else:
                return powerDer * self.base ** self.power
        else:
            if powerDer.isZero() and type(baseDer) is Constant and baseDer.value == 1:
                return self.power * self.base ** (self.power - Constant(1))
            else:
                raise Exception()

    def __repr__(self):
        return "-pow(-{0}, {1}) if {0} < 0 else pow({0}, {1})".format(repr(self.base), repr(self.power))


class Conditional(Expression):
    def __init__(self, var, threshold, lessThan, greaterThan):
        self.var = var
        self.threshold = threshold
        self.lessThan = lessThan
        self.greaterThan = greaterThan

    def isZero(self):
        return self.lessThan.isZero() and self.greaterThan.isZero()

    def partialDerivative(self, var):
        return Conditional(
            self.var,
            self.threshold,
            self.lessThan.partialDerivative(var),
            self.greaterThan.partialDerivative(var))

    def __repr__(self):
        return "{} if {} > {} else {}".format(
            repr(self.greaterThan),
            repr(self.var), repr(self.threshold),
            repr(self.lessThan))


class Constant(Expression):
    def __init__(self, value):
        self.value = value

    def isZero(self):
        return self.value == 0

    def partialDerivative(self, var):
        return Constant(0)

    def __repr__(self):
        return str(self.value)


class ProcessVariable(Expression):
    def __init__(self, symbol, initialValue, initialCovariance, noise, units, updateWeight=1):
        self.symbol = symbol
        self.initialValue = initialValue
        self.initialCovariance = initialCovariance
        self.noise = noise
        self.units = units
        if updateWeight > 1:
            print(
                "Warning: process variable {} has an update weight greater than 1, setting to 1".format(symbol))
        self.updateWeight = updateWeight

    def isZero(self):
        return False

    def partialDerivative(self, var):
        if self.symbol == var.symbol:
            return Constant(1)
        else:
            return Constant(0)

    def __repr__(self):
        return self.symbol


def recurseSum(expr):
    if type(expr) is Sum:
        return recurseSum(expr.left) + recurseSum(expr.right)
    else:
        return [expr]


class KalmanFilterGenerator:
    def __init__(self, processVars, controlVars, measurementVars, timeStepVar):
        self.xvars = processVars
        self.uvars = controlVars
        self.zvars = measurementVars
        self.deltat = timeStepVar
        self.f = [var for var in self.xvars]
        self.h = [Constant(0) for var in self.zvars]
        self.dfdx = [
            [Constant(1)
                if j == i
                else Constant(0)
                for j in range(len(self.xvars))
             ] for i in range(len(self.xvars))
        ]
        self.dhdx = [
            [Constant(0)
                for j in range(len(self.xvars))
             ] for i in range(len(self.zvars))
        ]
        self.mevars = {}
        self.msvars = {}
        self.mnvars = {}
        self.params = {}

    def addStateTransitionFunc(self, processVar, expr):
        index = self.xvars.index(processVar)
        self.f[index] = expr
        self.dfdx[index] = [expr.partialDerivative(var) for var in self.xvars]

    def addObservationFunc(self, measurementVar, expr):
        index = self.zvars.index(measurementVar)
        self.h[index] = expr
        self.dhdx[index] = [expr.partialDerivative(var) for var in self.xvars]

    def addParameter(self, name, value):
        self.params[name] = value

    def addErrorMetric(self, referenceVar, actualVar, weight):
        self.mevars[actualVar] = {
            "referenceVar": referenceVar,
            "weight": weight
        }

    def addSmoothnessMetric(self, metricVar, weight, filterCoef=0.005):
        self.msvars[metricVar] = {"weight": weight, "filterCoef": filterCoef}

    def addNegativityMetric(self, metricVar, weight):
        self.mnvars[metricVar] = {"weight": weight}

    def generatePython3Functions(self, outputPath):
        def generateUnpackingConditionals(vars, varsArrayName):
            return "\n\t\t".join([
                "if key.symbol == \"{}\": {}[{}] = value".format(
                    var.symbol, varsArrayName, i)
                for i, var in enumerate(vars)
            ])

        def generateUnpacking(varName, varList):
            return "({}) = {}.T[0]".format(
                " ".join([var.symbol + "," for var in varList]),
                varName
            )

        def generateExpression(expr, label=None):
            return "{},{}".format(
                repr(expr),
                "  # " + label if label is not None else "",
            )

        def generateExpressions(exprs, rowVars):
            return "\n\t\t".join([
                generateExpression(exprs[i], var.symbol)
                for i, var in enumerate(rowVars)
            ])

        def generateArrayRow(row, label=None):
            return "[{}],{}".format(
                " ".join([repr(item) + "," for item in row]),
                "  # " + label if label is not None else "",
            )

        def generateMatrix(matrix, rowVars):
            return "\n\t\t".join([
                generateArrayRow(matrix[i], var.symbol)
                for i, var in enumerate(rowVars)
            ])

        source = "\n\n".join([
            "\n".join([
                "# Packing Function",
                "def packingFunc(data):",
                "\t" +
                "uk, zk, deltat = [None] * {}, [None] * {}, None".format(
                    len(self.uvars), len(self.zvars)),
                "\t" + "for key, value in data.items():",
                "\t\t" + generateUnpackingConditionals(self.uvars, "uk"),
                "\t\t" + generateUnpackingConditionals(self.zvars, "zk"),
                "\t\t" +
                "if key.symbol == \"{}\": deltat = value".format(
                    self.deltat.symbol),
                "\t" + "if all((item is not None for item in uk)) and \\",
                "\t\t\t" + "all((item is not None for item in zk)) and \\",
                "\t\t\t" + "deltat is not None:",
                "\t\t" +
                "return np.column_stack((uk,)), np.column_stack((zk,)), deltat",
                "\t" + "else:",
                "\t\t" +
                "print(\"Missing data: {} {} {}\".format(uk, zk, deltat))",
            ]),
            "\n".join([
                "# State Transition Function",
                "def f(x, u, deltat):",
                "\t" + generateUnpacking("x", self.xvars),
                "\t" + generateUnpacking("u", self.uvars),
                "\t" +
                ("#" if self.deltat.symbol == "deltat" else "") +
                self.deltat.symbol + " = deltat",
                "\t" + "return np.array([[",
                "\t\t" + generateExpressions(self.f, self.xvars),
                "\t" + "]]).T",
            ]),
            "\n".join([
                "# State Transition Function Gradient",
                "def dfdx(x, u, deltat):",
                "\t" + generateUnpacking("x", self.xvars),
                "\t" + generateUnpacking("u", self.uvars),
                "\t" +
                ("#" if self.deltat.symbol == "deltat" else "") +
                self.deltat.symbol + " = deltat",
                "\t" + "return np.array([",
                "\t\t" + generateMatrix(self.dfdx, self.xvars),
                "\t" + "])",
            ]),
            "\n".join([
                "# Observation Function",
                "def h(x):",
                "\t" + generateUnpacking("x", self.xvars),
                "\t" + "return np.array([[",
                "\t\t" + generateExpressions(self.h, self.zvars),
                "\t" + "]]).T",
            ]),
            "\n".join([
                "# Observation Function Gradient",
                "def dhdx(x):",
                "\t" + generateUnpacking("x", self.xvars),
                "\t" + "return np.array([",
                "\t\t" + generateMatrix(self.dhdx, self.zvars),
                "\t" + "])",
            ]),
            "\n".join([
                "# Initial State",
                "x0 = np.array([[",
                "\t\t" + generateExpressions(
                    [var.initialValue for var in self.xvars],
                    self.xvars),
                "]]).T",
            ]),
            "\n".join([
                "# Initial State Covariance",
                "P0 = np.diag([",
                "\t\t" + generateExpressions(
                    [var.initialCovariance for var in self.xvars],
                    self.xvars),
                "])",
            ]),
            "\n".join([
                "# State Noise",
                "Q = np.diag([",
                "\t\t" + generateExpressions(
                    [var.noise for var in self.xvars],
                    self.xvars),
                "])",
            ]),
            "\n".join([
                "# Observation Noise",
                "R = np.diag([",
                "\t\t" + generateExpressions(
                    [var.noise for var in self.zvars],
                    self.zvars),
                "])",
            ]),
            "\n".join([
                "# Update Weights",
                "Wx = np.diag([",
                "\t\t" + generateExpressions(
                    [var.updateWeight for var in self.xvars],
                    self.xvars),
                "])",
            ]),
            "\n".join([
                "# Other Parameters",
                "\n".join(("{} = {}".format(key, value)
                          for key, value in self.params.items())),
            ]),
            "\n".join([
                "# Model Configurations",
                "extendedModelConfig = (packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R, Wx)",
                "unscentedModelConfig = (packingFunc, f, h, x0, P0, Q, R, Wx, alpha, beta, kappa)",
            ]),
        ])

        # Dump source to file
        with open(outputPath, "w") as outputFile:
            outputFile.write(
                "# Copyright 2021 by Daniel Winkelman. All rights reserved.\n")
            outputFile.write(
                "# This file is autogenerated by kalman.py.\n\n")
            outputFile.write("import numpy as np\n\n")
            outputFile.write(source)
            outputFile.write("\n")


class ExtendedKalmanFilterState:
    def __init__(self, packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R, Wx):
        self.packingFunc = packingFunc
        self.f = f
        self.dfdx = dfdx
        self.h = h
        self.dhdx = dhdx
        self.x = x0
        self.P = P0
        self.Q = Q
        self.R = R
        self.Wx = Wx

    def evolve(self, data):
        uk, zk, deltat = self.packingFunc(data)
        xint = self.f(self.x, uk, deltat)
        Fk = self.dfdx(self.x, uk, deltat)
        Hk = self.dhdx(xint)
        Pminus = Fk @ self.P @ Fk.T + self.Q
        yk = zk - self.h(xint)
        Sk = Hk @ Pminus @ Hk.T + self.R
        Kk = Pminus @ Hk.T @ np.linalg.inv(Sk)
        self.x = xint + self.Wx @ Kk @ yk
        Pplus = (np.eye(Kk.shape[0]) - Kk @ Hk) @ Pminus
        self.P = self.Wx @ (Pminus - Pplus) @ self.Wx + Pplus

    def getState(self):
        return self.x, self.P, self.h(self.x)


class UnscentedKalmanFilterState:
    def __init__(self, packingFunc, f, h, x0, P0, Q, R, Wx, alpha, beta, kappa):
        self.packingFunc = packingFunc
        self.f = f
        self.h = h
        self.x = x0
        self.P = P0
        self.Q = Q
        self.R = R
        self.Wx = Wx
        self.Wxi = np.eye(self.Wx.shape[0]) - self.Wx
        self.L = x0.size
        self.N = 2 * self.L + 1
        self.Wa = np.zeros((self.N,))
        self.Wc = np.zeros((self.N,))
        self.Wa[0] = (alpha**2 * kappa - self.L) / (alpha**2 * kappa)
        self.Wc[0] = self.Wa[0] + 1 - alpha**2 + beta
        self.Wa[1:] = 1 / (2 * alpha**2 * kappa)
        self.Wc[1:] = 1 / (2 * alpha**2 * kappa)
        self.alpha = alpha
        self.kappa = kappa

    def generateSigmaPoints(self, x, P):
        while True:
            try:
                A = np.linalg.cholesky(P)
                break
            except Exception as e:
                P = utils.nearestPD(P)
        s = [None] * self.N
        s[0] = np.copy(x)
        for j in range(self.L):
            diff = self.alpha * np.sqrt(self.kappa) * \
                np.column_stack((A[:, j],))
            s[j + 1] = x + diff
            s[self.L + j + 1] = x - diff
        return s

    def evolve(self, data):
        uk, zk, deltat = self.packingFunc(data)
        spred = self.generateSigmaPoints(self.x, self.P)
        x = [self.f(s, uk, deltat) for s in spred]
        xminus = sum((Wj * x[j] for j, Wj in enumerate(self.Wa)))
        Pminus = sum((Wj * (x[j] - xminus) @ (x[j] - xminus).T
                      for j, Wj in enumerate(self.Wc))) + self.Q
        supdate = self.generateSigmaPoints(xminus, Pminus)
        z = [self.h(s) for s in supdate]
        zhat = sum((Wj * z[j] for j, Wj in enumerate(self.Wa)))
        Sk = sum((Wj * (z[j] - zhat) @ (z[j] - zhat).T
                 for j, Wj in enumerate(self.Wc))) + self.R
        Csz = sum((Wj * (supdate[j] - xminus) @ (z[j] - zhat).T
                  for j, Wj in enumerate(self.Wc)))
        Kk = Csz @ np.linalg.inv(Sk)
        self.x = xminus + self.Wx @ Kk @ (zk - zhat)
        Pplus = Pminus - Kk @ Sk @ Kk.T
        self.P = self.Wxi @ Pminus @ self.Wxi + self.Wx @ Pplus @ self.Wx

    def getState(self):
        return self.x, self.P, self.h(self.x)


def kalmanProcess(dataMap, state):
    # Check that all data series are the same length
    dataDims = [series.numRows() for series in dataMap.values()]
    if not all((dataDims[0] == dim for dim in dataDims)):
        print("Invalid data because the series are different sizes")

    output = []
    covariance = []
    observation = []
    timeCol = []
    try:
        for i in range(dataDims[0]):
            times = [series.array[i, 0] for series in dataMap.values()]
            if not all((time == times[0] for time in times)):
                print("Row has different time values")
            state.evolve({key: series.array[i, 1]
                          for key, series in dataMap.items()})
            timeCol.append(times[0])
            x, P, z = state.getState()
            output.append(x.T)
            covariance.append(np.diag(P))
            observation.append(z.T)
    except Exception as exception:
        print("Kalman evolution error: {}".format(str(exception)))
        return None
    return np.column_stack((timeCol, np.row_stack(output), np.row_stack(covariance), np.row_stack(observation)))


def calculateErrorCosts(generator, inputSeries, outputSeries, plotFunc):
    costs = {}
    for var in generator.xvars + generator.zvars:
        if var in generator.mevars:
            referenceSeries = \
                inputSeries[generator.mevars[var]["referenceVar"]]
            errorSeries = outputSeries[var].error(referenceSeries)
            error = errorSeries.rms() / referenceSeries.averageAbs()
            costFactor = generator.mevars[var]["weight"] * error
            costs[var] = costFactor
            plotFunc([errorSeries])
            plotFunc([referenceSeries, outputSeries[var]])
    return costs


def calculateSmoothnessCosts(generator, outputSeries):
    costs = {}
    for var in generator.xvars + generator.zvars:
        if var in generator.msvars:
            smoothness = \
                outputSeries[var].smoothness(generator.msvars[var]["filterCoef"]) / \
                outputSeries[var].rms()
            costFactor = generator.msvars[var]["weight"] * smoothness
            costs[var] = costFactor
    return costs


def calculateExtrapolation(xsSeries, dxsdtSeries, deltatSeries, deltatCatchup):
    output = np.zeros((xsSeries.numRows(), 2))
    output[:, 0] = xsSeries.array[:, 0]
    output[0, 1] = xsSeries.array[0, 1]
    for i, (xs, dxsdt, deltat) in enumerate(
            zip(xsSeries.array[:-1, 1],
                dxsdtSeries.array[:-1, 1],
                deltatSeries.array[:-1, 1])):
        dxsdtPred = (xs - output[i, 1]) / deltatCatchup + dxsdt
        output[i + 1, 1] = output[i, 1] + deltat * dxsdtPred
    return output


def openKalmanSpec(modelDir):
    print("Opening Kalman filter spec {}...".format(modelDir))
    spec = SourceFileLoader("spec", modelDir).load_module()
    modelName = os.path.splitext(os.path.basename(modelDir))[0]
    setattr(spec, "modelName", modelName)
    return spec


def restoreKalmanSpec(spec, varGroup, index, param, value):
    setattr(getattr(spec, varGroup)[index], param, value)


def manipulateKalmanSpec(spec, varGroup, index, param, change):
    if varGroup == "zvars" and (param == "initialValue" or param == "initialCovariance" or param == "updateWeight"):
        return None
    var = getattr(spec, varGroup)[index]
    origValue = getattr(var, param)
    newValue = origValue * (1 + change)
    if origValue == 0 or param == "updateWeight" and (newValue > 1 or newValue < 0):
        return None
    setattr(var, param, origValue * (1 + change))
    return origValue


def assembleOutputDirName(dataDir, filterType, modelName, subdirs):
    dataDirPath = os.path.split(dataDir)
    return os.path.join(
        *dataDirPath[:-1],
        "kalman-{}-{}-{}".format(filterType, dataDirPath[-1], modelName),
        *subdirs)


def kalmanAnalysis(
        dataDir, spec, filterType, outputSubdirs=[],
        generatePlots=False, completeSummary=False, deltatCatchup=None):
    # Figure out the output directory
    outputDir = assembleOutputDirName(
        dataDir, filterType, spec.modelName, outputSubdirs)

    # Plotting utils
    def plotSeries(series, outputFileName=None, show=False, **kwargs):
        if not generatePlots:
            return
        utils.plotSeries(series, outputDir=outputDir,
                         outputFileName=outputFileName, show=show, **kwargs)

    # Make sure output directory exists
    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
    print("Saving output to {}".format(outputDir))

    # Compile Kalman filter
    modelFileName = "{}/model.py".format(outputDir)
    spec.generator.generatePython3Functions(modelFileName)
    model = SourceFileLoader("model", modelFileName).load_module()

    # Import data
    print("Opening data directory {}...".format(dataDir))
    inputData = utils.importDataFiles(dataDir)

    # Normalize data
    inputData["xe"] = inputData["xe"].applyUnary(
        lambda x: x - inputData["xe"].array[0, 1])
    inputData["xs"] = inputData["xs"].applyUnary(
        lambda x: x - inputData["xs"].array[0, 1])

    # Generate derived data
    inputData["dxsdt"] = inputData["xs"].lowpass(0.1).derivative()
    deltats = np.zeros(inputData["xe"].array.shape)
    deltats[:, 0] = inputData["xe"].array[:, 0]
    deltats[0, 1] = deltats[0, 0]
    deltats[1:, 1] = deltats[1:, 0] - deltats[:-1, 0]
    inputData["deltat"] = utils.Series("t", "deltat", deltats)

    # Optional data for plotting
    if generatePlots:
        deltax = inputData["xe"].applyBinary(
            inputData["xs"], lambda xe, xs: xe - xs, newDep="deltax")
        dxedt = inputData["xe"].lowpass(0.1).derivative(newDep="dxedt")
        deltax_vs_P = inputData["P"].join(deltax)
        dxsdt_vs_P = inputData["dxsdt"].join(inputData["P"])
        deltax_vs_dxsdt = inputData["dxsdt"].join(deltax)

        plotSeries([inputData["xe"], inputData["xs"]])
        plotSeries([dxedt, inputData["dxsdt"]])
        plotSeries([inputData["P"]])
        plotSeries([deltax])
        plotSeries([deltax_vs_P])
        plotSeries([dxsdt_vs_P])
        plotSeries([deltax_vs_dxsdt])

    # Run Kalman filter
    if filterType == "ekf":
        state = ExtendedKalmanFilterState(*model.extendedModelConfig)
    elif filterType == "ukf":
        state = UnscentedKalmanFilterState(*model.unscentedModelConfig)
    kalmanInputSeries = {
        spec.deltat: inputData["deltat"],
        spec.Ph_in: inputData["P"],
        spec.xs_in: inputData["xs"],
        spec.dxsdt: inputData["dxsdt"],
        spec.xe: inputData["xe"],
    }
    kalmanOutputSeries = {}
    kalmanData = kalmanProcess(kalmanInputSeries, state)
    if kalmanData is None:
        return None

    # Export all Kalman parameters to their own data series and create plots
    for i, xvar in enumerate(spec.xvars):
        seriesName = "kalman_{}".format(xvar)
        series = utils.Series("t", seriesName, np.column_stack(
            (kalmanData[:, 0], kalmanData[:, i + 1])))
        kalmanOutputSeries[xvar] = series

        if xvar in spec.generator.msvars:
            a = spec.generator.msvars[xvar]["filterCoef"]
            filtered = series.lowpass(a)
            plotSeries([series, filtered])
        else:
            plotSeries([series])

        seriesName = "kalman_cov_{}".format(xvar)
        series = utils.Series("t", seriesName, np.column_stack(
            (kalmanData[:, 0], kalmanData[:, len(spec.generator.xvars) + i + 1])))
        plotSeries([series])

    for i, zvar in enumerate(spec.generator.zvars):
        seriesName = "kalman_{}".format(zvar)
        series = utils.Series("t", seriesName, np.column_stack(
            (kalmanData[:, 0], kalmanData[:, len(spec.generator.xvars) * 2 + i + 1])))
        kalmanOutputSeries[zvar] = series
        plotSeries([series])

    # Simulate the extrapolation algorithm
    if deltatCatchup is not None:
        xsExtArray = calculateExtrapolation(
            inputData["xs"], kalmanOutputSeries[spec.dxsdt], inputData["deltat"], deltatCatchup)
        xsExt = utils.Series("t", "xs_extrapolation", xsExtArray)
        xsExtError = xsExt.error(inputData["xs"])
        dxsdtLowpass = inputData["xs"].derivative().expLowpass(0.05)
        xsExtLowpassArray = calculateExtrapolation(
            inputData["xs"], dxsdtLowpass, inputData["deltat"], deltatCatchup)
        xsExtLowpass = utils.Series("t", "xs_lowpass", xsExtLowpassArray)
        xsExtLowpassError = xsExtLowpass.error(inputData["xs"])
        plotSeries([inputData["xs"], xsExt, xsExtLowpass])
        plotSeries([xsExtError])
        plotSeries([xsExtLowpassError])
        plotSeries([inputData["dxsdt"], dxsdtLowpass])

    # Special plots
    if hasattr(spec, "Ps") and spec.Ps in kalmanOutputSeries:
        viscocityCurve = kalmanOutputSeries[spec.dxsdt].join(
            kalmanOutputSeries[spec.Ps])
        plotSeries([viscocityCurve])

    # Calculate costs
    errorCosts = calculateErrorCosts(
        spec.generator, kalmanInputSeries, kalmanOutputSeries, plotSeries)
    smoothnessCosts = calculateSmoothnessCosts(
        spec.generator, kalmanOutputSeries)
    totalCost = sum(errorCosts.values()) + sum(smoothnessCosts.values())
    if completeSummary:
        print("Total Cost: {}".format(totalCost))
        summaries = [("error", var.symbol, cost) for var, cost in errorCosts.items()] + \
            [("smoothness", var.symbol, cost)
             for var, cost in smoothnessCosts.items()]
        summaries.sort(key=lambda row: row[2], reverse=True)
        for category, symbol, cost in summaries:
            print(" * {}: {:.3f}% ({:.3e})".format(
                "{}-{}:".format(category, symbol).ljust(24, "."),
                100 * cost / totalCost, cost))
    plt.close("all")
    if deltatCatchup is not None:
        xsExtErrorTotal = xsExtError.rms()
        print("True extrusion RMS error: {}".format(xsExtErrorTotal))
        return xsExtErrorTotal, inputData["xe"].numRows()
    else:
        return totalCost, inputData["xe"].numRows()


def getDataDirs(args):
    output = []
    if args.batch:
        dataDirRe = re.compile(args.data_dir)
        for current, folders, files in os.walk(args.data_dir_root):
            for folder in folders:
                if dataDirRe.match(folder):
                    output.append(os.path.join(current, folder))
            if not args.data_dir_recurse:
                break
        output.sort()
        return output
    else:
        if os.path.exists(args.data_dir):
            return [args.data_dir]
        else:
            raise("Data directory \"{}\" does not exist".format(args.data_dir))


def analyzeBatch(
        dataDirs, spec, filterType, outputSubdirs=[],
        generatePlots=False, completeSummary=False, deltatCatchup=None):
    # Collect costs and weights for each dataset
    costs = {}
    print("Preparing to process:")
    for dataDir in dataDirs:
        print(" - {}".format(dataDir))

    # Redirect output to a log file
    print("Logging to {}...".format(logFileName))
    oldStdout = sys.stdout
    oldStderr = sys.stderr

    def printOverride(s):
        swappedStdout = sys.stdout
        sys.stdout = oldStdout
        print(s)
        sys.stdout = swappedStdout

    with open(logFileName, "a") as logFile:
        sys.stdout = logFile
        sys.stderr = logFile
        for dataDir in dataDirs:
            cost, weight = kalmanAnalysis(
                dataDir, spec, filterType, outputSubdirs=outputSubdirs,
                generatePlots=generatePlots, completeSummary=completeSummary, deltatCatchup=deltatCatchup)
            printOverride("{} (weight {}): cost is {}".format(
                dataDir, weight, cost))
            costs[dataDir] = (cost, weight)
            if cost is None or np.isnan(cost):
                break

    sys.stdout = oldStdout
    sys.stderr = oldStderr

    # Calculate average cost
    if any((cost is None or np.isnan(cost) for cost, weight in costs.values())):
        output = np.nan
    else:
        weightedCost = sum((cost * weight for cost, weight in costs.values()))
        totalWeight = sum((weight for cost, weight in costs.values()))
        output = weightedCost / totalWeight
    print("Total (average) cost is {}".format(output))
    return output


if __name__ == "__main__":
    # Parse command line args and set up directories
    args = parser.parse_args((sys.argv[1:]))
    dataDirs = getDataDirs(args)
    spec = openKalmanSpec(args.model)
    if args.command == "run":
        cost = analyzeBatch(
            dataDirs, spec, args.filter_type, outputSubdirs=["run"],
            generatePlots=not args.no_plots, completeSummary=True, deltatCatchup=args.deltat_catchup)
    elif args.command == "optimize":
        for n in range(100):
            costs = []
            print("==== Round {} ====".format(n))
            print("Running reference ({})...".format(n))
            referenceCost = analyzeBatch(
                dataDirs, spec, args.filter_type,
                outputSubdirs=["opt{}".format(n), "reference"],
                generatePlots=not args.no_plots, deltatCatchup=args.deltat_catchup)
            for group in ["xvars", "zvars"]:
                for index, var in enumerate(getattr(spec, group)):
                    for param in ["initialValue", "initialCovariance", "noise", "updateWeight"]:
                        for direction, change in [("dec", -args.step_size * np.exp(-n / 20)), ("inc", args.step_size * np.exp(-n / 20))]:
                            address = (group, index, param)
                            origValue = manipulateKalmanSpec(
                                spec, *address, change)
                            if origValue is None:
                                continue
                            label = "{}-{}-{}".format(
                                str(var), param, direction)
                            print("Running {} ({})...".format(label, n))
                            cost = analyzeBatch(
                                dataDirs, spec, args.filter_type,
                                outputSubdirs=[
                                    "opt{}".format(n), "trial", label],
                                deltatCatchup=args.deltat_catchup)
                            costs.append((address, cost, change, origValue))
                            restoreKalmanSpec(spec, *address, origValue)
            costs = list(
                filter(lambda row: row[1] is not None and not np.isnan(row[1]), costs))
            costs.sort(key=lambda row: row[1])
            bestCost = referenceCost
            for index, (address, cost, change, origValue) in enumerate(costs):
                manipulateKalmanSpec(spec, *address, change)
                newCost = analyzeBatch(
                    dataDirs, spec, args.filter_type,
                    ["opt{}".format(n), "optimize", str(index)],
                    deltatCatchup=args.deltat_catchup)
                if newCost is None or newCost >= bestCost or np.isnan(newCost):
                    restoreKalmanSpec(spec, *address, origValue)
                    break
                else:
                    bestCost = newCost
                    print("Applying {} optimizes down to {}".format(
                        address, newCost))
            if bestCost == referenceCost:
                break
    else:
        print("Please choose a command")
