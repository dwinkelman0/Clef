# Copyright 2021 by Daniel Winkelman. All rights reserved.

# Time step
deltat = ProcessVariable("deltat", 0, 0, 0, "sec")

# Position and pressure state
xe = ProcessVariable("xe", 0, 0, 0, "usteps")
xs = ProcessVariable("xs", 0, 0, 0.1, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 20, "usteps")
dxsdt = ProcessVariable("dxsdt", 0, 0, 0.2, "usteps/sec")
Ph = ProcessVariable("Ph", 0, 0, 0.1, "Pa")
Ph0 = ProcessVariable("Ph0", 2500, 100, 0.01, "Pa")
Ph_in = ProcessVariable("Ph_in", 0, 0, 3, "Pa")
Ps = ProcessVariable("Ps", 0, 0, 0.1, "Pa")
Pfric = ProcessVariable("Pfric", 0, 0, 10, "Pa")

# Level 1 Capacitance
Chl = ProcessVariable("Chl", 1.2, 0.4, 0.01, "Pa/ustep")

# Level 1 Shear Thinning
m = ProcessVariable("m", 0.56, 0.05, 0.00001,
                    "dimensionless", updateWeight=0.05)
gamma = ProcessVariable("gamma", 6, 0.5, 0.00001, "idk", updateWeight=0.05)

# Model Variables
xvars = [xs, dxsdt, Ph, Ph0, Ps, Pfric, Chl, m, gamma, ]
uvars = [xe, ]
zvars = [xs_in, Ph_in, ]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)

# Model state equations
generator.addStateTransitionFunc(xs, xs + dxsdt * deltat)
generator.addStateTransitionFunc(Ps, Ph - Pfric)
generator.addStateTransitionFunc(dxsdt, Conditional(
    dxsdt, Constant(25), dxsdt, gamma * Ps ** m))
generator.addStateTransitionFunc(Ph, Chl * (xe - xs))
generator.addObservationFunc(xs_in, xs)
generator.addObservationFunc(Ph_in, Ph + Ph0)
