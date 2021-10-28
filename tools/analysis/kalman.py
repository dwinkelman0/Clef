#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import os
import sys
from importlib.machinery import SourceFileLoader
from matplotlib.colors import get_named_colors_mapping

import numpy as np

import utils

parser = argparse.ArgumentParser()
parser.add_argument("--model", type=str, required=True)
parser.add_argument("--data-dir", type=str, default=utils.DEFAULT_DATA_DIR)
parser.add_argument("--output-dir", type=str, default=None)
parser.add_argument("--no-plots", action="store_true", default=False)

subparsers = parser.add_subparsers(dest="command")
optimize_parser = subparsers.add_parser("optimize")
optimize_parser.add_argument("--step-size", type=float, default=0.02)


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

    def addStateTransitionFunc(self, processVar, expr):
        index = self.xvars.index(processVar)
        self.f[index] = expr
        self.dfdx[index] = [expr.partialDerivative(var) for var in self.xvars]

    def addObservationFunc(self, measurementVar, expr):
        index = self.zvars.index(measurementVar)
        self.h[index] = expr
        self.dhdx[index] = [expr.partialDerivative(var) for var in self.xvars]

    def addErrorMetric(self, referenceVar, actualVar, weight):
        self.mevars[actualVar] = {
            "referenceVar": referenceVar,
            "weight": weight
        }

    def addSmoothnessMetric(self, metricVar, weight, filterCoef=0.005):
        self.msvars[metricVar] = {"weight": weight, "filterCoef": filterCoef}

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
                "modelConfig = (packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R, Wx)",
            ])
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


class KalmanFilterState:
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
        Pint = Fk @ self.P @ Fk.T + self.Q
        yk = zk - self.h(xint)
        Sk = Hk @ Pint @ Hk.T + self.R
        Kk = Pint @ Hk.T @ np.linalg.inv(Sk)
        self.x = xint + self.Wx @ Kk @ yk
        self.P = (np.eye(Kk.shape[0]) - Kk @ Hk) @ Pint

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
            error = errorSeries.rms() / referenceSeries.rms()
            costFactor = generator.mevars[var]["weight"] * error
            costs[var] = costFactor
            plotFunc([referenceSeries, outputSeries[var]])
    return costs


def calculateSmoothnessCosts(generator, inputSeries, outputSeries):
    costs = {}
    for var in generator.xvars + generator.zvars:
        if var in generator.msvars:
            smoothness = \
                outputSeries[var].smoothness(generator.msvars[var]["filterCoef"]) / \
                outputSeries[var].rms()
            costFactor = generator.msvars[var]["weight"] * smoothness
            costs[var] = costFactor
    return costs


def processDataSet(dataDir, modelDir, outputDir, initialConditionAdjustments, generatePlots=False):
    modelName = os.path.splitext(os.path.basename(modelDir))[0]
    outputDir = outputDir \
        if not outputDir is None \
        else "kalman-{}-{}".format(dataDir, modelName)
    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
    print("Saving output to {}".format(outputDir))

    # Import and compile Kalman Filter
    print("Opening and compiling Kalman filter {}...".format(args.model))
    spec = SourceFileLoader("spec", args.model).load_module()
    modelFileName = "{}/model.py".format(outputDir)
    spec.generator.generatePython3Functions(modelFileName)
    model = SourceFileLoader("model", modelFileName).load_module()

    # Plotting utils
    def plotSeries(series, outputFileName=None, show=False, **kwargs):
        if args.no_plots:
            return
        utils.plotSeries(series, outputDir=outputDir,
                         outputFileName=outputFileName, show=show, **kwargs)

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
    if not args.no_plots:
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
    state = KalmanFilterState(*model.modelConfig)
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
        print("Exiting...")
        exit(0)

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

    # Calculate costs
    errorCosts = calculateErrorCosts(
        spec.generator, kalmanInputSeries, kalmanOutputSeries, plotSeries)
    smoothnessCosts = calculateSmoothnessCosts(
        spec.generator, kalmanInputSeries, kalmanOutputSeries)
    totalCost = sum(errorCosts.values()) + sum(smoothnessCosts.values())
    print("Total Cost: {}".format(totalCost))
    summaries = [("error", var.symbol, cost) for var, cost in errorCosts.items()] + \
        [("smoothness", var.symbol, cost)
         for var, cost in smoothnessCosts.items()]
    summaries.sort(key=lambda row: row[2], reverse=True)
    for category, symbol, cost in summaries:
        print(" * {}: {:.3f}% ({:.3e})".format(
            "{}-{}:".format(category, symbol).ljust(24, "."),
            100 * cost / totalCost, cost))


if __name__ == "__main__":
    # Parse command line args and set up directories
    args = parser.parse_args((sys.argv[1:]))
    processDataSet(args.data_dir, args.model, args.output_dir, None)
