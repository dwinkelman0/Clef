#!/bin/python3

# Copyright 2022 by Daniel Winkelman. All rights reserved.

from data_series import DataSeries
from metadata import Metadata

import argparse
import json
import matplotlib.pyplot as plt
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--metadata-file", type=str, required=True)

if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])

    # Load metadata (if it already exists)
    if os.path.exists(args.metadata_file):
        rawMetadata = json.load(open(args.metadata_file, "r"))
        metadataDict = {key: Metadata(**value) for key, value in rawMetadata.items()}
    else:
        print("Cannot find metadata file '{}'".format(args.metadata_file))
        exit(0)

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

        xsNormalized.plot(axs[0][0], "xs")
        xeNormalized.plot(axs[0][0], "xe")
        axs[0][0].set_xlabel("Time (s)")
        axs[0][0].set_ylabel("Displacement")
        axs[0][0].legend()

        displacementDifference.plot(axs[0][1], "diff")
        P.plot(axs[0][1], "P")
        axs[0][1].set_xlabel("Time (s)")

        massVsDisplacement.plot(axs[1][0], "mass vs. displacement")

        pressureVsDisplacement.plot(axs[1][1], "pressure vs. displacement")
        axs[1][1].set_xlabel("Displacement")
        axs[1][1].set_ylabel("Pressure")
        plt.show()
