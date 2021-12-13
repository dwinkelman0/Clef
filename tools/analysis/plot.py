#!/bin/python3

# Copyright 2021 by Daniel Winkelman. All rights reserved.

import argparse
import utils
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--input-dir", type=str, default=utils.DEFAULT_DATA_DIR)
parser.add_argument("--output-dir", type=str, default=None)


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])
    dataDir = args.input_dir
    outputDir = args.input_dir if args.output_dir is None else args.output_dir
    print("Opening data directory {}...".format(dataDir))
    inputData = utils.importDataFiles(dataDir)

    def plotSeries(series, outputFileName=None, show=False, **kwargs):
        utils.plotSeries([series], outputDir=outputDir,
                         outputFileName=outputFileName, show=show, **kwargs)

    # Normalize data
    inputData["xe"] = inputData["xe"].applyUnary(
        lambda x: x - inputData["xe"].array[0, 1])
    inputData["xs"] = inputData["xs"].applyUnary(
        lambda x: x - inputData["xs"].array[0, 1])

    for key, series in inputData.items():
        plotSeries(series)
