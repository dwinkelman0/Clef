# Copyright 2021 by Daniel Winkelman. All rights reserved.

# Time step
deltat = ProcessVariable("deltat", 0, 0, 0, "sec")

# Position and pressure state
xe = ProcessVariable("xe", 0, 0, 0, "usteps")
xs = ProcessVariable("xs", 0, 20, 0, "usteps")
xs0 = ProcessVariable("xs0", 0, 20, 0, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 0, "usteps")
dxsdt = ProcessVariable("dxsdt", 0, 20, 0, "usteps/sec")
Ph = ProcessVariable("Ph", 0, 30, 0, "Pa")
Ph0 = ProcessVariable("Ph0", 2500, 100, 0, "Pa")
Ph_in = ProcessVariable("Ph_in", 0, 0, 0, "Pa")

# Level 1 Capacitance
Chl = ProcessVariable("Chl", 3, 0.4, 0, "Pa/ustep")

# Level 1 Friction
a0 = ProcessVariable("a0", 300, 1000, 0, "Pa")
a1 = ProcessVariable("a1", 3.0, 1, 0, "Pa/(ustep/sec)")

# Model Variables
xvars = [xs, dxsdt, Ph, Ph0, Chl, a0, a1, ]
uvars = [xe, ]
zvars = [xs_in, Ph_in, ]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)

# Model state equations
generator.addStateTransitionFunc(xs, xs + dxsdt * deltat)
generator.addStateTransitionFunc(dxsdt, (Ph - a0) / a1)
generator.addStateTransitionFunc(Ph, Chl * (xe - xs))
generator.addObservationFunc(xs_in, xs)
generator.addObservationFunc(Ph_in, Ph + Ph0)
