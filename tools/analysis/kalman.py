#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import os
import sys

import numpy as np

import utils

parser = argparse.ArgumentParser()
parser.add_argument("--model", type=str, default=None)
parser.add_argument("--data-dir", type=str, default=utils.DEFAULT_DATA_DIR)
parser.add_argument("--output-dir", type=str, default=None)


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
        return "pow({}, {})".format(repr(self.base), repr(self.power))


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
    def __init__(self, symbol, initialValue, initialCovariance, noise, units):
        self.symbol = symbol
        self.initialValue = initialValue
        self.initialCovariance = initialCovariance
        self.noise = noise
        self.units = units

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

    def addStateTransitionFunc(self, processVar, expr):
        index = self.xvars.index(processVar)
        self.f[index] = expr
        self.dfdx[index] = [expr.partialDerivative(var) for var in self.xvars]

    def addObservationFunc(self, measurementVar, expr):
        index = self.zvars.index(measurementVar)
        self.h[index] = expr
        self.dhdx[index] = [expr.partialDerivative(var) for var in self.xvars]

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
                "uk, zk, deltat = [None] * {}, [None] * {}, 0".format(
                    len(self.uvars), len(self.zvars)),
                "\t" + "for key, value in data.items():",
                "\t\t" + generateUnpackingConditionals(self.uvars, "uk"),
                "\t\t" + generateUnpackingConditionals(self.zvars, "zk"),
                "\t\t" +
                "if key == \"{}\": deltat = value".format(
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

        # Interpret source into active memory
        exec(source, globals())
        return packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R


class KalmanFilterState:
    def __init__(self, packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R):
        self.packingFunc = packingFunc
        self.f = f
        self.dfdx = dfdx
        self.h = h
        self.dhdx = dhdx
        self.x = x0
        self.P = P0
        self.Q = Q
        self.R = R

    def evolve(self, data):
        uk, zk, deltat = packingFunc(data)
        xint = self.f(self.x, uk, deltat)
        Fk = self.dfdx(self.x, uk, deltat)
        Hk = self.dhdx(xint)
        Pint = Fk @ self.P @ Fk.T + self.Q
        yk = zk - self.h(xint)
        Sk = Hk @ Pint @ Hk.T + self.R
        Kk = Pint @ Hk.T @ np.linalg.inv(Sk)
        self.x = xint + Kk @ yk
        self.P = (np.eye(Kk.shape[0]) - Kk @ Hk) @ Pint

    def getState(self):
        return self.x, self.P


def kalmanProcess(dataMap, state):
    # Check that all data series are the same length
    dataDims = [data.shape[0] for data in dataMap.values()]
    if not all((dataDims[0] == dim for dim in dataDims)):
        print("Invalid data because the series are different sizes")

    output = []
    timeCol = []
    for i in range(dataDims[0]):
        times = [array[i, 0] for array in dataMap.values()]
        if not all((time == times[0] for time in times)):
            print("Row has different time values")
        timeCol.append(times[0])
        state.evolve({key: array[i, 1] for key, array in dataMap.items()})
        x, P = state.getState()
        output.append(x.T)
    return np.column_stack((timeCol, np.row_stack(output)))


if __name__ == "__main__":
    # Parse command line args and set up directories
    args = parser.parse_args((sys.argv[1:]))
    outputDir = args.output_dir \
        if not args.output_dir is None \
        else "kalman-{}".format(args.data_dir)
    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
    print("Saving output to {}".format(outputDir))

    # Import and compile Kalman Filter
    print("Opening and compiling Kalman filter {}...".format(args.model))
    with open(args.model, "r") as inputFile:
        # TODO: this is extremely dangerous
        exec(inputFile.read(), globals())
    packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R = generator.generatePython3Functions(
        "{}/model.py".format(outputDir))

    # Plotting utils
    def plotSeries(names, outputFileName=None, show=False, **kwargs):
        utils.plotSeries(names, outputDir=outputDir,
                         outputFileName=outputFileName, show=show, **kwargs)

    # Import data
    print("Opening data directory {}...".format(args.data_dir))
    utils.importDataFiles(args.data_dir)

    # Normalize data
    utils.data[("t", "xe")][:, 1] = utils.data[("t", "xe")
                                               ][:, 1] - utils.data[("t", "xe")][0, 1]
    utils.data[("t", "xs")][:, 1] = utils.data[("t", "xs")
                                               ][:, 1] - utils.data[("t", "xs")][0, 1]

    # Generate derivative data
    utils.createSeries(("t", "deltax"), utils.binaryOperator(
        utils.data[("t", "xe")], utils.data[("t", "xs")], lambda a1, a2: a1 - a2))
    utils.joinSeries(("t", "P"), ("t", "deltax"))
    utils.lowpassFilter(("t", "xe"), 0.1)
    utils.derivative(("t", "xe_filtered"))
    utils.data[("t", "dxedt")] = utils.data[("t", "dxe_filtereddt")]
    utils.lowpassFilter(("t", "xs"), 0.1)
    utils.derivative(("t", "xs_filtered"))
    utils.data[("t", "dxsdt")] = utils.data[("t", "dxs_filtereddt")]
    utils.joinSeries(("t", "P"), ("t", "dxsdt"))
    utils.joinSeries(("t", "dxsdt"), ("t", "deltax"))

    # Plot original data
    plotSeries([("t", "xe"), ("t", "xs")])
    plotSeries([("t", "dxedt"), ("t", "dxsdt")])
    plotSeries([("t", "P")])
    plotSeries([("t", "deltax")])
    plotSeries([("P", "deltax")])
    plotSeries([("P", "dxsdt")])
    plotSeries([("dxsdt", "deltax")])

    # Calculate deltat
    deltats = np.zeros(utils.data[("t", "xe")].shape)
    deltats[:, 0] = utils.data[("t", "xe")][:, 0]
    deltats[0, 1] = deltats[0, 0]
    deltats[1:, 1] = deltats[1:, 0] - deltats[:-1, 0]

    # Run Kalman filter
    state = KalmanFilterState(packingFunc, f, dfdx, h, dhdx, x0, P0, Q, R)
    kalmanData = kalmanProcess(
        {
            Ph_in: utils.data[("t", "P")],
            xs_in: utils.data[("t", "xs")],
            xe: utils.data[("t", "xe")]
        }, state)

    # Export all Kalman parameters to their own data series and create plots
    for i, xvar in enumerate(generator.xvars):
        seriesName = ("t", "kalman_{}".format(xvar.symbol))
        utils.createSeries(
            seriesName,
            np.column_stack((kalmanData[:, 0], kalmanData[:, i + 1])))
        plotSeries([seriesName])
