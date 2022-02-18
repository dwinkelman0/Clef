#!/bin/python3

# Copyright 2022 by Daniel Winkelman. All rights reserved.

from data_series import DataSeries

import os

class Metadata:
    def __init__(self, **kwargs):
        assert "dataDir" in kwargs
        self.dataDir = kwargs.get("dataDir")
        self.start = kwargs.get("start", None)
        self.end = kwargs.get("end", None)
        self.extrusionStart = kwargs.get("extrusionStart", None)
        self.extrusionEnd = kwargs.get("extrusionEnd", None)

    def asJson(self):
        output = {}
        for key in ["dataDir", "start", "end", "extrusionStart", "extrusionEnd"]:
            if getattr(self, key) is not None:
                output[key] = getattr(self, key)
        return output

    def isComplete(self):
        params = [self.start, self.end, self.extrusionStart, self.extrusionEnd]
        return not all((x is None for x in params))

    def loadData(self, allowedSeries):
        output = {}
        for filename in os.listdir(self.dataDir):
            if DataSeries.FILENAME_RE.match(filename):
                seriesName = filename.split("-")[0]
                if seriesName in allowedSeries:
                    dataSeries = DataSeries.fromFile(os.path.join(self.dataDir, filename)).removeOutliers().selectRange(self.start, self.end)
                    output[seriesName] = dataSeries
        return output
