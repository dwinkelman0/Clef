# Copyright 2021 by Daniel Winkelman. All rights reserved.

# Time step
deltat = ProcessVariable("deltat", 0, 0, 0, "sec")

# Position and pressure state
xe = ProcessVariable("xe", 0, 0, 0, "usteps")
xs = ProcessVariable("xs", 0, 20, 1, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 10, "usteps")
dxsdt = ProcessVariable("dxsdt", 0, 20, 0.1, "usteps/sec")
xn = ProcessVariable("xn", 0, 20, 0, "usteps")
dxndt = ProcessVariable("dxndt", 0, 20, 0, "usteps/sec")
Ph = ProcessVariable("Ph", 1, 30, 0.1, "Pa")
Ph0 = ProcessVariable("Ph0", 2500, 100, 0, "Pa")
Ph_in = ProcessVariable("Ph_in", 0, 0, 3, "Pa")
Ps = ProcessVariable("Ps", 1, 30, 0, "Pa")

# Level 1 Capacitance
Chl = ProcessVariable("Chl", 1.2, 0.4, 0, "Pa/ustep")
Csl = ProcessVariable("Csl", 2.5, 0.4, 0.1, "Pa/ustep")
AhAs = ProcessVariable("AhAs", (0.5 / 0.7)**2, 0.1, 0, "dimensionless")

# Level 1 Friction
a0 = ProcessVariable("a0", 300, 1000, 0, "Pa")
a1 = ProcessVariable("a1", 3.0, 1, 0, "Pa/(ustep/sec)")

# Level 1 Shear Thinning
m = ProcessVariable("m", 0.56, 0.05, 0, "dimensionless")
gamma = ProcessVariable("gamma", 6, 0.5, 0, "idk")

# Model Variables
xvars = [xs, dxsdt, xn, dxndt, Ph, Ph0, Ps, Chl, Csl, AhAs, a0, a1, m, gamma, ]
uvars = [xe, ]
zvars = [xs_in, Ph_in, ]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)

# Model state equations
generator.addStateTransitionFunc(xs, xs + dxsdt * deltat)
generator.addStateTransitionFunc(xn, xn + dxndt * deltat)
generator.addStateTransitionFunc(a0, AhAs * Ph - Ps - a1 * dxsdt)
generator.addStateTransitionFunc(dxndt, gamma * Ps ** m)
generator.addStateTransitionFunc(Ph, Chl * (xe - xs))
generator.addStateTransitionFunc(Ps, Csl * (xn - xs))
generator.addObservationFunc(xs_in, xs)
generator.addObservationFunc(Ph_in, Ph + Ph0)
