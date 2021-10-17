# Copyright 2021 by Daniel Winkelman. All rights reserved.

import datetime
import os
import re
import sys

import numpy as np
import matplotlib.pyplot as plt


DATA_DEFAULT_DIRNAME_RE = re.compile(
    "data-\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d\.\d\d\d\d\d\d")
DATA_FILENAME_RE = re.compile("[a-zA-Z]+-vs-[a-zA-Z]+\.npy")


DEFAULT_DATA_DIR = max(
    [name for name in os.listdir(".") if os.path.isdir(name) and DATA_DEFAULT_DIRNAME_RE.match(name)])
