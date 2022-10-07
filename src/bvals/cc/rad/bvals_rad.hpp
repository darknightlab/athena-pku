#ifndef BVALS_CC_RAD_BVALS_RAD_HPP_
#define BVALS_CC_RAD_BVALS_RAD_HPP_
//========================================================================================
// Athena++ astrophysical MHD code
// Copyright(C) 2014 James M. Stone <jmstone@princeton.edu> and other code contributors
// Licensed under the 3-clause BSD License, see LICENSE file for details
//========================================================================================
//! \file bvals_rad.hpp
//  \brief definition of RadBoundaryVariable class

// Athena++ headers
#include "../bvals_cc.hpp"             // CellCenteredBoundaryVariable
#include "../../../athena.hpp"         // Real
#include "../../../athena_arrays.hpp"  // AthenaArray

//----------------------------------------------------------------------------------------
// Radiation boundary variable class

class RadBoundaryVariable : public CellCenteredBoundaryVariable {

public:

  // Constructor and destructor
  RadBoundaryVariable(MeshBlock *pmb, AthenaArray<Real> *p_var,
      AthenaArray<Real> *p_coarse_var, AthenaArray<Real> *flux_x, int num_zeta,
      int num_psi);
  virtual ~RadBoundaryVariable() = default;

  // Parameters (manually mirroring Radiation)
  int nzeta;   // number of polar radiation angles in active zone
  int npsi;    // number of azimuthal radiation angles in active zone
  int nang;    // total number of radiation angles, including ghost zones
  int zs, ze;  // start and end zeta-indices
  int ps, pe;  // start and end psi-indices
  int is, ie;  // start and end x1-indices
  int js, je;  // start and end x2-indices
  int ks, ke;  // start and end x3-indices

  // Data arrays (manually mirroring Radiation)
  AthenaArray<Real> zetaf;   // face-centered polar radiation angles
  AthenaArray<Real> zetav;   // volume-centered polar radiation angles
  AthenaArray<Real> dzetaf;  // face-to-face polar radiation angle differences
  AthenaArray<Real> psif;    // face-centered azimuthal radiation angles
  AthenaArray<Real> psiv;    // volume-centered azimuthal radiation angles
  AthenaArray<Real> dpsif;   // face-to-face azimuthal radiation angle differences

  // Reflecting boundary function overrides
  void ReflectInnerX1(Real time, Real dt, int il, int jl, int ju, int kl, int ku, int ngh)
      override;
  void ReflectOuterX1(Real time, Real dt, int iu, int jl, int ju, int kl, int ku, int ngh)
      override;
  void ReflectInnerX2(Real time, Real dt, int il, int iu, int jl, int kl, int ku, int ngh)
      override;
  void ReflectOuterX2(Real time, Real dt, int il, int iu, int ju, int kl, int ku, int ngh)
      override;
  void ReflectInnerX3(Real time, Real dt, int il, int iu, int jl, int ju, int kl, int ngh)
      override;
  void ReflectOuterX3(Real time, Real dt, int il, int iu, int jl, int ju, int ku, int ngh)
      override;

  // Indexing function for angles
  // Inputs:
  //   l: zeta-index
  //   m: psi-index
  // Outputs:
  //   returned value: 1D index for both zeta and psi
  // Notes:
  //   Less general version of Radiation::AngleInd().
  int AngleInd(int l, int m) {
    return l * (npsi + 2*NGHOST_RAD) + m;
  }

private:

  // Reflecting boundary remapping arrays
  AthenaArray<int> reflect_ind_ix1_;
  AthenaArray<int> reflect_ind_ox1_;
  AthenaArray<int> reflect_ind_ix2_;
  AthenaArray<int> reflect_ind_ox2_;
  AthenaArray<int> reflect_ind_ix3_;
  AthenaArray<int> reflect_ind_ox3_;
  AthenaArray<Real> reflect_frac_ix1_;
  AthenaArray<Real> reflect_frac_ox1_;
  AthenaArray<Real> reflect_frac_ix2_;
  AthenaArray<Real> reflect_frac_ox2_;
  AthenaArray<Real> reflect_frac_ix3_;
  AthenaArray<Real> reflect_frac_ox3_;

  // Polar boundary remapping arrays
  AthenaArray<Real> polar_vals_;
  AthenaArray<int> polar_ind_north_;
  AthenaArray<int> polar_ind_south_;
  AthenaArray<Real> polar_frac_north_;
  AthenaArray<Real> polar_frac_south_;

  // Boundary setting function overrides
  void SetBoundarySameLevel(Real *buf, const NeighborBlock& nb) override;
  void SetBoundaryFromCoarser(Real *buf, const NeighborBlock& nb) override;
  void SetBoundaryFromFiner(Real *buf, const NeighborBlock& nb) override;
};

#endif  // BVALS_CC_RAD_BVALS_RAD_HPP_