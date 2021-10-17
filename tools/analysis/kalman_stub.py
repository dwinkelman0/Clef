# Copyright 2021 by Daniel Winkelman. All rights reserved.

import numpy as np

class ExtendedKalmanFilterState:
    def __init__(self, f, F, h, H, Rk, x0, P0):
        self.f = f
        self.F = F
        self.h = h
        self.H = H
        self.Rk = Rk
        self.x = x0
        self.P = P0

    def evolve(self, uk, zk, deltat):
        xint = self.f(self.x, uk, deltat)
        Fk = self.F(self.x, uk, deltat)
        Hk = self.H(xint)
        Pint = Fk @ self.P @ Fk.T + \
            np.diag([0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0, 1, 10, 3])
        yk = zk - self.h(xint)
        Sk = Hk @ Pint @ Hk.T + self.Rk
        Kk = Pint @ Hk.T @ np.linalg.inv(Sk)
        self.x = xint + Kk @ yk
        self.P = (np.eye(Kk.shape[0]) - Kk @ Hk) @ Pint

    def getState(self):
        return self.x

