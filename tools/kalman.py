# Copyright 2021 by Daniel Winkelman. All rights reserved.

import numpy as np


class KalmanState:
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


def f(x, u, deltat):
    (
        muxe, muxs, muxn, dmuxndt,  # displacement state
        muPh, muPs,  # pressure state
        alphah, alphas, rhoeta, AhAs, FfdAs,  # control parameters
        xs0, Ph0,  # adjustment parameters
    ) = x.T[0]
    (xein,) = u.T[0]  # TODO: redefine
    return np.column_stack((np.array([
        xein,  # muxe (control input)
        muxn + muPs / alphas,  # muxs (pressure-displacement ratio)
        muxn + deltat * dmuxndt,  # muxn (discrete-time integral)
        muPs / rhoeta,  # dmuxndt (Poiseuille's Law)
        alphah * (muxe - muxs),  # muPh (pressure-displacement ratio)
        AhAs * muPh - FfdAs,  # muPs (sum-of-forces)
        alphah,
        alphas,
        rhoeta,
        AhAs,
        FfdAs,
        xs0,
        Ph0,
    ]),))


def F(x, u, deltat):
    (
        muxe, muxs, muxn, dmuxndt,  # displacement state
        muPh, muPs,  # pressure state
        alphah, alphas, rhoeta, AhAs, FfdAs,  # control parameters
        xs0, Ph0,  # adjustment parameters
    ) = x.T[0]
    (xein,) = u.T[0]  # TODO: redefine
    return np.array([
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ],  # muxe
        [0, 0, 1, 0, 0, 1 / alphas, 0, -muPs / alphas**2, 0, 0, 0, 0, 0, ],  # muxs
        [0, 0, 1, deltat, 0, 0, 0, 0, 0, 0, 0, 0, 0, ],  # muxn
        [0, 0, 0, 0, 0, 1 / rhoeta, 0, 0, -muPs / \
            rhoeta**2, 0, 0, 0, 0, ],  # dmuxndt
        [alphah, -alphah, 0, 0, 0, 0, muxe - muxs, 0, 0, 0, 0, 0, 0, ],  # muPh
        [0, 0, 0, 0, AhAs, 0, 0, 0, 0, muPh, -1, 0, 0, ],  # muPs
        [0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ],
        [0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, ],
        [0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, ],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, ],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, ],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, ],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, ],
    ])


def h(x):
    (
        muxe, muxs, muxn, dmuxndt,  # displacement state
        muPh, muPs,  # pressure state
        alphah, alphas, rhoeta, AhAs, FfdAs,  # control parameters
        xs0, Ph0,  # adjustment parameters
    ) = x.T[0]
    return np.array([[muPh + Ph0 * 1000, muxs + xs0]]).T


def H(x):
    (
        muxe, muxs, muxn, dmuxndt,  # displacement state
        muPh, muPs,  # pressure state
        alphah, alphas, rhoeta, AhAs, FfdAs,  # control parameters
        xs0, Ph0,  # adjustment parameters
    ) = x.T[0]
    return np.array([
        [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1000, ],  # dP/dx
        [0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, ],  # dxs/dx
    ])


R = np.array([
    [3, 0, ],
    [0, 10, ],
])


def analyze(data):
    kalman = KalmanState(f, F, h, H, R,
                         np.array(
                             [[
                                 0, 0, 0, 0,  # displacement state
                                 0, 0,  # pressure state
                                 # control parameters
                                 3.4, 1.7, 20, 0.5, 0,
                                 0, 4,  # adjustment parameters
                             ]]).T,
                         np.diag([0, 10, 10, 10,  # displacement state
                                 3, 3,  # pressure state
                                 0.1, 0.1, 0.1, 0, 1,  # control parameters
                                 1, 4,  # adjustment parameters
                                  ]))
    rows = []
    lastT = 0
    for row in data:
        t, xe, P, xs = row
        kalman.evolve(np.array([[xe]]), np.array([[P], [xs]]), t - lastT)
        rows.append(kalman.getState().T)
        lastT = t
    return np.concatenate((data, np.row_stack(rows)), axis=1)
