# Copyright 2021 by Daniel Winkelman. All rights reserved.

import numpy as np


class KalmanState:
    def __init__(self, f, F, h, H, Rk, x0):
        self.f = f
        self.F = F
        self.h = h
        self.H = H
        self.Rk = Rk
        self.x = x0
        self.P = np.eye(x0.size)

    def evolve(self, uk, zk, deltat):
        xint = self.f(self.x, uk, deltat)
        Fk = self.F(self.x, uk, deltat)
        Hk = self.H(xint)
        Pint = Fk @ self.P @ Fk.T
        yk = zk - self.h(xint)
        Sk = Hk @ Pint @ Hk.T + self.Rk
        Kk = Pint @ Hk.T @ np.linalg.inv(Sk)
        self.x = xint + Kk @ yk
        self.P = (np.eye(Kk.shape[0]) - Kk @ Hk) @ Pint

    def getState(self):
        return self.x


def f(x, u, deltat):
    (mux, muxprime, muP, alpha, beta, P0, deltax0, xe) = x.T[0]
    (xein,) = u.T[0]  # TODO: redefine
    return np.column_stack((np.array([
        mux + deltat * muxprime,
        beta * (xe - mux - deltax0),
        P0 + alpha * (xe - mux - deltax0),
        alpha,
        beta,
        P0,
        deltax0,
        xein,
    ]),))


def F(x, u, deltat):
    (mux, muxprime, muP, alpha, beta, P0, deltax0, xe) = x.T[0]
    (xein,) = u.T[0]  # TODO: redefine
    return np.array([
        [1, deltat, 0, 0, 0, 0, 0, 0, ],  # dmux/dx
        [-beta, 0, 0, 0, xe - mux - deltax0, 0, -beta, beta, ],  # dmuxprime/dx
        [-alpha, 0, 0, xe - mux - deltax0, 0, 1, -alpha, alpha, ],  # dP/dx
        [0, 0, 0, 1, 0, 0, 0, 0, ],  # dalpha/dx
        [0, 0, 0, 0, 1, 0, 0, 0, ],  # dbeta/dx
        [0, 0, 0, 0, 0, 1, 0, 0, ],  # dP0/dx
        [0, 0, 0, 0, 0, 0, 1, 0, ],  # ddeltax/dx
        [0, 0, 0, 0, 0, 0, 0, 0, ],  # dxe/dx
    ])


def h(x):
    (mux, muxprime, muP, alpha, beta, P0, deltax0, xe) = x.T[0]
    return np.array([[muP + P0, mux + deltax0]]).T


def H(x):
    (mux, muxprime, muP, alpha, beta, P0, deltax0, xe) = x.T[0]
    return np.array([
        [0, 0, 1, 0, 0, 1, 0, 0, ],  # dP/dx
        [1, 0, 0, 0, 0, 0, 1, 0, ],  # dxs/dx
    ])


R = np.array([
    [3, 0, ],
    [0, 10, ],
])


def analyze(data):
    kalman = KalmanState(f, F, h, H, R,
                         np.array([[0, 0, 0, 0.3, 0.05, 100, 1, 0]]).T)
    rows = []
    lastT = 0
    for row in data:
        t, xe, P, xs = row
        kalman.evolve(np.array([[xe]]), np.array([[P], [xs]]), t - lastT)
        rows.append(kalman.getState().T)
        lastT = t
    return np.concatenate((data, np.row_stack(rows)), axis=1)
