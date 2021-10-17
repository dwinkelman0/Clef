# Copyright 2021 by Daniel Winkelman. All rights reserved.

deltat = ProcessVariable("deltat", 0, 0, 0, "sec")
xe = ProcessVariable("xe", 0, 10, 10, "usteps")
xs = ProcessVariable("xs", 0, 10, 10, "usteps")
xs_in = ProcessVariable("xs_in", 0, 0, 10, "usteps")
xn = ProcessVariable("xn", 0, 10, 10, "usteps")
dxndt = ProcessVariable("dxndt", 0, 10, 10, "usteps/sec")
Ph = ProcessVariable("Ph", 0, 3, 100, "Pa")
Ph_in = ProcessVariable("Ph_in", 0, 0, 100, "Pa")
Ps = ProcessVariable("Ps", 0, 3, 100, "Pa")
Chl = ProcessVariable("Chl", 3.4, 0.1, 0.1, "Pa/ustep")
Csl = ProcessVariable("Csl", 1.7, 0.1, 0.1, "Pa/ustep")
rhoeta = ProcessVariable("rhoeta", 20, 0.1, 0.1, "Pa/ustep")
AhAs = ProcessVariable("AhAs", 0.5, 0.1, 0.1, "m^2/m^2")
FfdAs = ProcessVariable("FfdAs", 0, 1, 1, "Pa")
xs0 = ProcessVariable("xs0", 0, 1, 10, "usteps")
Ph0 = ProcessVariable("Ph0", 5000, 1000, 50, "Pa")
xvars = [xs, xn, dxndt, Ph, Ps, Chl, Csl, rhoeta, AhAs, FfdAs, xs0, Ph0]
uvars = [xe]
zvars = [xs_in, Ph_in]
generator = KalmanFilterGenerator(xvars, uvars, zvars, deltat)
generator.addStateTransitionFunc(xs, xn + Ps / Csl)
generator.addStateTransitionFunc(xn, xn + deltat * dxndt)
generator.addStateTransitionFunc(dxndt, Ps / rhoeta)
generator.addStateTransitionFunc(Ph, Chl * (xe - xs))
generator.addStateTransitionFunc(Ps, AhAs * Ph - FfdAs)
generator.addObservationFunc(xs_in, xs + xs0)
generator.addObservationFunc(Ph_in, Ph + Ph0)
