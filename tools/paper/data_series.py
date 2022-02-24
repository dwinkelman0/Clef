#!/bin/python3

# Copyright 2022 by Daniel Winkelman. All rights reserved.

import numpy as np
import re
import scipy.optimize as opt


def interp(*arrays):
    ts = np.concatenate([array[:, 0] for array in arrays])
    ts.sort()
    ts = np.unique(ts)
    return (ts,) + tuple((np.interp(ts, array[:, 0], array[:, 1]) for array in arrays))

class DataSeries:
    DATA_FOLDER_RE = re.compile("(.*/)?n\d+-[\w\.]+-\w+-\d+f")
    FILENAME_RE = re.compile("([a-zA-Z]+)-vs-[a-zA-Z]+\.npy")

    def __init__(self, data):
        self.data = data
        self.range = (None, None)

    def __sub__(self, other):
        assert type(other) == type(self)
        ts, thisData, otherData = interp(self.data, other.data)
        return DataSeries(np.column_stack((ts, thisData - otherData)))
    
    def join(self, other):
        assert type(other) == type(self)
        ts, thisData, otherData = interp(self.data, other.data)
        return DataSeries(np.column_stack((thisData, otherData)))

    def fromFile(fileName):
        data = np.array(np.load(fileName), dtype=np.float)
        data[:, 0] /= 1e6
        return DataSeries(data)

    def fromRegressionParameters(self, params):
        x0, y0, alpha = params
        print(params)
        ts = self.data[:, 0]
        return DataSeries(np.column_stack((ts, -y0 * np.exp(alpha * (ts - x0)) + y0)))

    def getStartToEndDifference(self):
        return self.data[-1, 1] - self.data[0, 1]

    def selectRange(self, start, end):
        if start is not None and end is not None:
            mask = (start <= self.data[:, 0]) & (self.data[:, 0] <= end)
            return DataSeries(self.data[mask])
        elif start is not None:
            mask = start <= self.data[:, 0]
            return DataSeries(self.data[mask])
        elif end is not None:
            mask = self.data[:, 0] <= end
            return DataSeries(self.data[mask])

    def removeOutliers(self):
        minValue = min(self.data[:, 1])
        maxValue = max(self.data[:, 1])
        diff = maxValue - minValue
        if diff == 0:
            return
        comparison = abs(self.data[1:, 1] - self.data[:-1, 1]) > (0.05 * diff)
        mask = np.concatenate(([False], comparison)) & np.concatenate((comparison, [False]))
        return DataSeries(self.data[~mask])

    def normalizeToUnit(self):
        minValue = min(self.data[:, 1])
        maxValue = max(self.data[:, 1])
        assert minValue < maxValue
        newCol = (self.data[:, 1] - minValue) / (maxValue - minValue)
        return DataSeries(np.column_stack((self.data[:, 0], newCol)))

    def normalizeToStart(self):
        startValue = self.data[0, 1]
        return DataSeries(np.column_stack((self.data[:, 0], self.data[:, 1] - startValue)))

    def normalizeToStartAndEnd(self, scale):
        startValue = self.data[0, 1]
        endValue = self.data[-1, 1]
        assert startValue != endValue
        newCol = (self.data[:, 1] - startValue) / (endValue - startValue) * abs(scale)
        return DataSeries(np.column_stack((self.data[:, 0], newCol)))

    def performExponentialRegression(self, guess):
        minTime = min(self.data[:, 0])
        def eq(x, *coefs):
            y0, alpha = coefs
            return -y0 * np.exp(alpha * (x - minTime)) + y0
        params = opt.curve_fit(eq, self.data[:, 0], self.data[:, 1], guess)[0]
        return minTime, *params

    def plot(self, ax, label, style="."):
        ax.plot(self.data[:, 0], self.data[:, 1], style, label=label)


def clusterExperimentDict(experimentDict):
    output = {}
    for key, value in experimentDict.items():
        if DataSeries.DATA_FOLDER_RE.match(key) is not None:
            name = (key.split("/")[-1]).split("-")[1]
            nameList = output.get(name, [])
            nameList.append(value)
            output[name] = nameList
        else:
            print("Could not cluster \"{}\"".format(key))
    return output
