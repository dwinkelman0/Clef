# Copyright 2021 by Daniel Winkelman. All rights reserved.

# Time step
deltat = ProcessVariable("deltat", 0, 0, 0, "sec")

# Position and pressure state
xe = ProcessVariable("xe", 0, 0, 0, "usteps")
xs = ProcessVariable("xs", 0, 20, 10, "usteps")
xs0 = ProcessVariable("xs", 0, 20, 10, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 10, "usteps")
dxsdt = ProcessVariable("dxsdt", 0, 1000, 400, "usteps/sec")
Ph = ProcessVariable("Ph", 0, 500, 50, "Pa")
Ph0 = ProcessVariable("Ph", 0, 500, 50, "Pa")
Ph_in = ProcessVariable("Ph_in", 0, 0, 50, "Pa")

# Level 1 Capacitance
Chl = ProcessVariable("Chl", 1, 0.4, 0.1, "Pa/ustep")

# Level 1 Friction
a0 = ProcessVariable("a0", 0, 1000, 50, "Pa")
a1 = ProcessVariable("a1", 3, 1, 0.1, "Pa/(ustep/sec)")

# Model Variables
xvars = [xs, xs0, dxsdt, Ph, Ph0, Chl, a0, a1, ]
uvars = [xe, ]
zvars = [xs_in, Ph_in, ]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)

# Model state equations
generator.addStateTransitionFunc(xs, xs + dxsdt * deltat)
generator.addStateTransitionFunc(Ph, a0 + a1 * dxsdt)
generator.addStateTransitionFunc(Chl, -Ph / (xs - xe))
generator.addObservationFunc(xs_in, xs + xs0)
generator.addObservationFunc(Ph_in, Ph + Ph0)
