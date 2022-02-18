#!/bin/python3

# Copyright 2022 by Daniel Winkelman. All rights reserved.

from data_series import DataSeries
from metadata import Metadata

import argparse
import json
import matplotlib.pyplot as plt
import os
import pathlib
import sys
import threading

parser = argparse.ArgumentParser()
parser.add_argument("--root-dir", type=str, required=True)
parser.add_argument("--metadata-file", type=str, required=True)
parser.add_argument("--revise", action="store_true", default=False)

def parseOptionalFloat(s, existing):
    return float(s) if len(s) > 0 else existing

def populateMetadata(metadata):
    metadata.start = parseOptionalFloat(input("Start: "), metadata.start)
    metadata.end = parseOptionalFloat(input("End: "), metadata.end)
    metadata.extrusionStart = parseOptionalFloat(input("Extrusion Start: "), metadata.extrusionStart)
    metadata.extrusionEnd = parseOptionalFloat(input("Extrusion End: "), metadata.extrusionEnd)


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])

    # Load metadata (if it already exists)
    if os.path.exists(args.metadata_file):
        rawMetadata = json.load(open(args.metadata_file, "r"))
        metadataDict = {key: Metadata(**value) for key, value in rawMetadata.items()}
    else:
        metadataDict = {}

    # Recurse through all folders
    for root, subfolders, files in os.walk(args.root_dir):
        if len(subfolders) == 0:
            key = os.path.relpath(root, args.root_dir)
            dataDir = str(pathlib.Path(root).resolve())
            if key in metadataDict:
                metadataDict[key].dataDir = dataDir
            else:
                metadataDict[key] = Metadata(dataDir=dataDir)

    # Display graphs and edit metadata
    for key, metadata in sorted(metadataDict.items()):
        if not metadata.isComplete() or args.revise:
            dataSeriesDict = metadata.loadData({"xe", "xs", "P", "m"})

            # Prompt input for metadata
            if metadata.isComplete() and input("Revise? [y/N]: ") != "y":
               continue
            thread = threading.Thread(target=populateMetadata, args=(metadata,))
            thread.start()
            
            xe = dataSeriesDict["xe"]
            xs = dataSeriesDict["xs"]
            P = dataSeriesDict["P"]
            m = dataSeriesDict["m"]
            fig, ax = plt.subplots(1, 1)
            xe.normalize().plot(ax, "xe")
            xs.normalize().plot(ax, "xs")
            P.normalize().plot(ax, "P")
            m.normalize().plot(ax, "m")
            ax.legend()
            plt.show()

            thread.join()
            json.dump(
                {key: metadata.asJson() for key, metadata in metadataDict.items()},
                open(args.metadata_file, "w"),
                indent=4)
