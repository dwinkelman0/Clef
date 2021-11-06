// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Memory.h>
#include <string.h>
#include <util/Matrix.h>

namespace Clef::Fw {
template <uint16_t Xsize, uint16_t Usize, uint16_t Zsize>
class KalmanFilter {
 public:
  using XVector = Clef::Util::RamMatrix<Xsize, 1>;
  using UVector = Clef::Util::RamMatrix<Usize, 1>;
  using ZVector = Clef::Util::RamMatrix<Zsize, 1>;

  virtual void evolve(const UVector &uk, const ZVector &zk,
                      const float deltat) = 0;
  virtual const XVector &getState() const = 0;
};

template <uint16_t Xsize, uint16_t Usize, uint16_t Zsize>
class ExtendedKalmanFilter : public KalmanFilter<Xsize, Usize, Zsize> {
 public:
  using XVector = Clef::Util::RamMatrix<Xsize, 1>;
  using UVector = Clef::Util::RamMatrix<Usize, 1>;
  using ZVector = Clef::Util::RamMatrix<Zsize, 1>;
  using PMatrix = Clef::Util::RamMatrix<Xsize, Xsize>;
  using FMatrix = Clef::Util::RamMatrix<Xsize, Xsize>;
  using QMatrix = Clef::If::RomDiagonalMatrix<Xsize>;
  using WxMatrix = Clef::If::RomDiagonalMatrix<Xsize>;
  using HMatrix = Clef::Util::RamMatrix<Zsize, Xsize>;
  using RMatrix = Clef::If::RomDiagonalMatrix<Zsize>;

  ExtendedKalmanFilter(QMatrix &Q, RMatrix &R, WxMatrix &Wx)
      : x_(memX_), P_(memP_), Q_(Q), R_(R), Wx_(Wx) {
    memset(memX_, 0, sizeof(memX_));
    memset(memP_, 0, sizeof(memP_));
  }

  virtual void init() = 0;

  void evolve(const UVector &uk, const ZVector &zk,
              const float deltat) override {
    float scratch1[Xsize * Xsize], scratch2[Xsize * Xsize];

    float xintMem[Xsize];
    XVector xint(xintMem);
    calculateStateTrans(this->x_, uk, deltat, xint);

    float hxintMem[Zsize], ykMem[Zsize];
    ZVector hxint(hxintMem);
    ZVector yk(ykMem);
    calculateObservationTrans(xint, hxint);
    Clef::Util::Matrix::sub(zk, hxint, yk);

    float HkMem[Zsize * Xsize];
    HMatrix Hk(HkMem);
    calculateObserationTransGradient(xint, Hk);

    float PminusMem[Xsize * Xsize];
    FMatrix Fk(PminusMem);
    typename FMatrix::Transpose FkT = Fk.transpose();
    PMatrix FkP(scratch1);
    PMatrix FkPFkT(scratch2);
    PMatrix Pminus(PminusMem);
    calculateStateTransGradient(this->x_, uk, deltat, Fk);
    Clef::Util::Matrix::dot(Fk, this->P_, FkP);
    Clef::Util::Matrix::dot(FkP, FkT, FkPFkT);
    Clef::Util::Matrix::add(FkPFkT, this->Q_, Pminus);

    float KkMem[Xsize * Zsize];
    typename HMatrix::Transpose HkT = Hk.transpose();
    Clef::Util::RamMatrix<Xsize, Zsize> PminusHkT(scratch1);
    Clef::Util::RamMatrix<Zsize, Zsize> HkPminusHkT(scratch2);
    Clef::Util::RamMatrix<Zsize, Zsize> Sk(KkMem);
    Clef::Util::RamMatrix<Zsize, Zsize> Skinv(scratch2);
    Clef::Util::RamMatrix<Xsize, Zsize> Kk(KkMem);
    Clef::Util::Matrix::dot(Pminus, HkT, PminusHkT);
    Clef::Util::Matrix::dot(Hk, PminusHkT, HkPminusHkT);
    Clef::Util::Matrix::add(HkPminusHkT, this->R_, Sk);
    Clef::Util::Matrix::inverse(Sk, Skinv);
    Clef::Util::Matrix::dot(PminusHkT, Skinv, Kk);

    Clef::Util::RamMatrix<Xsize, Zsize> WxKk(scratch1);
    XVector WxKkyk(scratch2);
    Clef::Util::Matrix::dot(this->Wx_, Kk, WxKk);
    Clef::Util::Matrix::dot(WxKk, yk, WxKkyk);
    Clef::Util::Matrix::add(xint, WxKkyk, this->x_);

    PMatrix KkHk(scratch1);
    PMatrix eyeKkHk(scratch2);
    PMatrix Pplus(scratch1);
    PMatrix deltaP(scratch2);
    PMatrix deltaPWx(scratch1);
    PMatrix WxdeltaPWx(scratch2);
    Clef::Util::Matrix::dot(Kk, Hk, KkHk);
    Clef::Util::Matrix::sub(Clef::Util::IdentityMatrix<Xsize>(), KkHk, eyeKkHk);
    Clef::Util::Matrix::dot(eyeKkHk, Pminus, Pplus);
    Clef::Util::Matrix::sub(Pminus, Pplus, deltaP);
    Clef::Util::Matrix::dot(deltaP, this->Wx_, deltaPWx);
    Clef::Util::Matrix::dot(this->Wx_, deltaPWx, WxdeltaPWx);
    Clef::Util::Matrix::add(WxdeltaPWx, Pplus, this->P_);
  }

  const XVector &getState() const override { return x_; }

 protected:
  virtual void calculateStateTrans(const XVector &xk, const UVector &uk,
                                   const float deltat,
                                   XVector &output) const = 0;
  virtual void calculateStateTransGradient(const XVector &xk, const UVector &uk,
                                           const float deltat,
                                           FMatrix &output) const = 0;
  virtual void calculateObservationTrans(const XVector &xk,
                                         ZVector &output) const = 0;
  virtual void calculateObserationTransGradient(const XVector &xk,
                                                HMatrix &output) const = 0;

  float memX_[Xsize];
  float memP_[Xsize * Xsize];

  XVector x_;
  PMatrix P_;
  QMatrix Q_;
  RMatrix R_;
  WxMatrix Wx_;
};
}  // namespace Clef::Fw
