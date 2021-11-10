# Copyright 2021 by Daniel Winkelman. All rights reserved.

from kalman import *

# Time step
deltat = ProcessVariable("deltat", 0, 0, 0, "sec")

# Position and pressure state
xe = ProcessVariable("xe", 0, 0, 0, "usteps")
xs = ProcessVariable("xs", 0, 10, 0.5, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 5, "usteps")
dxsdt = ProcessVariable("dxsdt", 0, 30, 0.15, "usteps/sec")
Ph = ProcessVariable("Ph", 0, 10, 0.9, "Pa", updateWeight=0.5)
Ph0 = ProcessVariable("Ph0", 4000, 40, 0.2, "Pa", updateWeight=1)
Ph_in = ProcessVariable("Ph_in", 0, 0, 2, "Pa")
Ps = ProcessVariable("Ps", 0, 10, 0.1, "Pa", updateWeight=0.35)
a1 = ProcessVariable("a1", 3, 1, 0.14, "Pa/(ustep/sec)", updateWeight=0.1)
a0 = ProcessVariable("a0", 0, 100, 10, "Pa", updateWeight=0.1)

# Level 1 Capacitance
Chl = ProcessVariable("Chl", 12, 0.2, 0.03, "Pa/ustep", updateWeight=0.03)

# Level 1 Shear Thinning
#m = ProcessVariable("m", 0.6, 0.1, 0.006, "dimensionless", updateWeight=0.01)
gamma = ProcessVariable("gamma", 0.5, 0.1, 0.001, "idk", updateWeight=0.002)

# Model Variables
xvars = [xs, dxsdt, Ph, Ph0, Ps, a1, a0, Chl, gamma, ]
uvars = [xe, ]
zvars = [xs_in, Ph_in, ]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)

# Model state equations
generator.addStateTransitionFunc(xs, xs + dxsdt * deltat)
generator.addStateTransitionFunc(Ps, Ph - a0 - a1 * dxsdt)
# generator.addStateTransitionFunc(dxsdt, Conditional(
#    dxsdt, Constant(25), dxsdt, gamma * Ps ** m))
generator.addStateTransitionFunc(dxsdt, gamma * Ps)
generator.addStateTransitionFunc(Ph, Chl * (xe - xs))
generator.addObservationFunc(xs_in, xs)
generator.addObservationFunc(Ph_in, Ph + Ph0)
generator.addParameter("alpha", 2.5)
generator.addParameter("beta", 5.25)
generator.addParameter("kappa", 2)

# Model optimization
generator.addErrorMetric(xs_in, xs, 0.1)
generator.addErrorMetric(dxsdt, dxsdt, 5)
generator.addErrorMetric(Ph_in, Ph_in, 0.1)
generator.addSmoothnessMetric(xs, 0.1, 0.01)
generator.addSmoothnessMetric(Ph, 0.5, 0.01)
generator.addSmoothnessMetric(Ph0, 1, 0.001)
generator.addSmoothnessMetric(Ps, 0.5, 0.01)
generator.addSmoothnessMetric(a1, 1, 0.0005)
generator.addSmoothnessMetric(a0, 1, 0.0005)
generator.addSmoothnessMetric(Chl, 5, 0.0005)
#generator.addSmoothnessMetric(m, 10, 0.0005)
generator.addSmoothnessMetric(gamma, 5, 0.0005)
generator.addNegativityMetric(a1, 4)
generator.addNegativityMetric(a0, 2)
generator.addNegativityMetric(Chl, 1)
#generator.addNegativityMetric(m, 10)
generator.addNegativityMetric(gamma, 10)
