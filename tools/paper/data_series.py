#!/bin/python3

# Copyright 2022 by Daniel Winkelman. All rights reserved.

import numpy as np
import re


class DataSeries:
    FILENAME_RE = re.compile("([a-zA-Z]+)-vs-[a-zA-Z]+\.npy")

    def __init__(self, data):
        self.data = data
        self.range = (None, None)

    def fromFile(fileName):
        data = np.array(np.load(fileName), dtype=np.float)
        data[:, 0] /= 1e6
        return DataSeries(data)

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
        if sum(mask) > 0:
            print("Removing {} outliers".format(sum(mask)))
        return DataSeries(self.data[~mask])

    def normalize(self):
        minValue = min(self.data[:, 1])
        maxValue = max(self.data[:, 1])
        assert minValue < maxValue
        newCol = (self.data[:, 1] - minValue) / (maxValue - minValue)
        return DataSeries(np.column_stack((self.data[:, 0], newCol)))

    def plot(self, ax, label):
        ax.plot(self.data[:, 0], self.data[:, 1], label=label)
        ax.set_xlabel("Time (s)")
