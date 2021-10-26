# Copyright 2021 by Daniel Winkelman. All rights reserved.

# Time step
deltat = ProcessVariable("deltat", 0, 0, 0, "sec")

# Position and pressure state
xe = ProcessVariable("xe", 0, 0, 0, "usteps")
xs = ProcessVariable("xs", 0, 20, 1, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 10, "usteps")
dxsdt = ProcessVariable("dxsdt", 0, 20, 0.5, "usteps/sec")
Ph = ProcessVariable("Ph", 1, 30, 0.1, "Pa")
Ph0 = ProcessVariable("Ph0", 2500, 100, 0.1, "Pa")
Ph_in = ProcessVariable("Ph_in", 0, 0, 3, "Pa")

# Model Variables
xvars = [xs, dxsdt, Ph, Ph0, ]
uvars = []
zvars = [xs_in, Ph_in, ]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)

# Model state equations
generator.addStateTransitionFunc(xs, xs + dxsdt * deltat)
generator.addObservationFunc(xs_in, xs)
generator.addObservationFunc(Ph_in, Ph + Ph0)
