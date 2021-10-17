#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import sys

import numpy as np

from utils import DEFAULT_DATA_DIR

parser = argparse.ArgumentParser()
parser.add_argument("--model", type=str, default=None)
parser.add_argument("--data-dir", type=str, default=DEFAULT_DATA_DIR)
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
                "# State Transition Function",
                "def f(x, u, deltat):",
                "\t" + generateUnpacking("x", self.xvars),
                "\t" + generateUnpacking("u", self.uvars),
                "\t" + \
                    ("#" if self.deltat.symbol == "deltat" else "") + \
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
                "\t" + \
                    ("#" if self.deltat.symbol == "deltat" else "") + \
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
            outputFile.write("# Copyright 2021 by Daniel Winkelman. All rights reserved.\n")
            outputFile.write("# This file is autogenerated by kalman_gen.py.\n\n")
            outputFile.write("import numpy as np\n\n")
            outputFile.write(source)
            outputFile.write("\n")

        # Interpret source into active memory
        exec(source, globals())
        return f, dfdx, h, dhdx, x0, P0, Q, R

class KalmanFilterState:
    def __init__(self, f, F, h, H, Rk, x0, P0):
        self.f = f
        self.F = F
        self.h = h
        self.H = H
        self.Rk = Rk
        self.x = x0
        self.P = P0

    def evolve(self, uk, zk, deltat):
        xint = self.f(self.x, uk, deltat)
        Fk = self.F(self.x, uk, deltat)
        Hk = self.H(xint)
        Pint = Fk @ self.P @ Fk.T + \
            np.diag([0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0, 1, 10, 3])
        yk = zk - self.h(xint)
        Sk = Hk @ Pint @ Hk.T + self.Rk
        Kk = Pint @ Hk.T @ np.linalg.inv(Sk)
        self.x = xint + Kk @ yk
        self.P = (np.eye(Kk.shape[0]) - Kk @ Hk) @ Pint

    def getState(self):
        return self.x


if __name__ == "__main__":
    args = parser.parse_args((sys.argv[1:]))
    print("Opening data directory {}...".format(parser.data_dir))

